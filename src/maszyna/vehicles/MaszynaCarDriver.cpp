#include "MaszynaCarDriver.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "maszyna/scenario/MaszynaVehicleCommandAction.hpp"
#include "roads/RoadServer.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/road/RoadVehicleController.hpp"
#include "vehicles/road/RoadVehicleServer.hpp"

#include <godot_cpp/classes/curve3d.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
    void MaszynaCarDriver::_bind_methods() {
        ClassDB::bind_method(D_METHOD("vehicle_attach", "vehicle", "velocity"), &MaszynaCarDriver::vehicle_attach);
        ClassDB::bind_method(D_METHOD("vehicle_detach", "vehicle"), &MaszynaCarDriver::vehicle_detach);
        ClassDB::bind_method(D_METHOD("vehicle_is_attached", "vehicle"), &MaszynaCarDriver::vehicle_is_attached);
        ClassDB::bind_method(
                D_METHOD("vehicle_handle_command", "vehicle", "command", "value1", "value2"),
                &MaszynaCarDriver::vehicle_handle_command);
    }

    MaszynaCarDriver::MaszynaCarDriver() {
        singleton = this;
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &MaszynaCarDriver::vehicle_detach));
    }

    MaszynaCarDriver::~MaszynaCarDriver() {
        _set_processing(false);
        singleton = nullptr;
    }

    void MaszynaCarDriver::_set_processing(const bool p_processing) {
        SimulationServer *simulation = SimulationServer::get_instance();
        if (p_processing == processing || simulation == nullptr) {
            return;
        }
        processing = p_processing;
        const Callable advanced = callable_mp(this, &MaszynaCarDriver::_on_simulation_advanced);
        if (processing) {
            simulation->connect(SimulationServer::simulation_advanced_signal, advanced);
            return;
        }
        simulation->disconnect(SimulationServer::simulation_advanced_signal, advanced);
    }

    /* The orders of a driver without a timetable (OrdersInit(), Driver.cpp:5243-5249, 5316-5319):
     * it starts the engine (Prepare_engine), then shunts at the scenery's velocity within its
     * shunting one, or stands; a car has its head lights and red markers on (Driver.cpp:6519-6522) */
    void MaszynaCarDriver::vehicle_attach(const RID &p_vehicle, const double p_velocity) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        if (cars.has(p_vehicle)) {
            return;
        }
        Car &car = cars.insert(p_vehicle, Car())->value;
        car.ordered = Math::abs(p_velocity) >= READY_VELOCITY ? MIN(Math::abs(p_velocity), SHUNT_VELOCITY) : 0.0;
        car.wanted = car.ordered;
        car.stopped = Math::is_zero_approx(p_velocity);
        order.push_back(p_vehicle);
        vehicle_server->vehicle_send_command(p_vehicle, "engine_start");
        vehicle_server->vehicle_send_command(
                p_vehicle, "lights_set",
                static_cast<int64_t>(RoadVehicleController::LIGHTS_HEAD | RoadVehicleController::LIGHTS_TAIL));
        vehicle_server->vehicle_send_command(p_vehicle, "wanted_velocity_set", car.wanted);
        _set_processing(true);
    }

    void MaszynaCarDriver::vehicle_detach(const RID &p_vehicle) {
        if (!cars.erase(p_vehicle)) {
            return;
        }
        const int64_t index = order.find(p_vehicle);
        order.remove_at(static_cast<uint32_t>(index));
        if (next > static_cast<uint32_t>(index)) {
            --next;
        }
        if (order.is_empty()) {
            _set_processing(false);
        }
    }

    bool MaszynaCarDriver::vehicle_is_attached(const RID &p_vehicle) const {
        return cars.has(p_vehicle);
    }

    void MaszynaCarDriver::vehicle_handle_command(
            const RID &p_vehicle, const String &p_command, const double p_value1, const double p_value2) {
        Car *car = cars.getptr(p_vehicle);
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (car == nullptr || vehicle_server == nullptr || !(p_command == WARNING_SIGNAL) || p_value1 <= 0.0 ||
            p_value2 <= 0.0) {
            return;
        }
        car->horn_off_time = time + p_value1;
        vehicle_server->vehicle_send_command(p_vehicle, "horns_set", static_cast<int64_t>(p_value2));
    }

    /* Every car thinks once a reaction time, the cars spread over its frames */
    void MaszynaCarDriver::_on_simulation_advanced(const double p_seconds) {
        time += p_seconds;
        carried += static_cast<double>(order.size()) * p_seconds / REACTION_TIME;
        while (carried >= 1.0 && !order.is_empty()) {
            carried -= 1.0;
            if (next >= order.size()) {
                next = 0;
            }
            const RID vehicle = order[next++];
            _think(vehicle, *cars.getptr(vehicle));
        }
    }

    /* The memories the road's passive events read ahead of it, the way it drives (TableTraceRoute(),
     * CheckTrackEvent(), Driver.cpp:459-470, 589-779): a SetVelocity it slows down for within its
     * braking distance (fBrakeDist, Driver.cpp:6601), stopping within its proximity of one at 0 */
    double MaszynaCarDriver::_road_velocity(const RID &p_vehicle, const double p_ordered) const {
        RoadVehicleServer *road_vehicles = RoadVehicleServer::get_instance();
        const MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance();
        const ScenarioEventServer *scenario = ScenarioEventServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        double velocity = p_ordered;
        if (road_vehicles == nullptr || events == nullptr || scenario == nullptr || vehicle_server == nullptr) {
            return velocity;
        }
        // the route starts at its middle, its front is half its length on
        const double front = 0.5 * vehicle_server->vehicle_get_dimensions(p_vehicle).z;
        const double speed = vehicle_server->vehicle_get_speed(p_vehicle);
        const double min_proximity = MAX(MIN_PROXIMITY, speed * MIN_PROXIMITY_PER_SPEED);
        const double max_proximity = MAX(MAX_PROXIMITY, speed * MAX_PROXIMITY_PER_SPEED);
        const RoadServer *roads = RoadServer::get_instance();
        if (roads == nullptr) {
            return velocity;
        }
        const TypedArray<RoadRouteSegment> route = road_vehicles->vehicle_trace_route(p_vehicle, AWARENESS_RANGE);
        for (int64_t index = 0; index < route.size(); ++index) {
            const Ref<RoadRouteSegment> segment = route[index];
            // the events are its track's, of the road laid along it; a road of its own has none
            const RID track = roads->road_get_track(segment->get_road());
            if (!track.is_valid()) {
                continue;
            }
            const bool toward_end = segment->get_toward() == RoadServer::ROAD_END_END;
            const MaszynaScenarioEventServer::TrackEvent slot =
                    toward_end ? MaszynaScenarioEventServer::TRACK_EVENT2 : MaszynaScenarioEventServer::TRACK_EVENT1;
            const TypedArray<RID> track_events = events->track_get_events(track, slot);
            for (int64_t event_index = 0; event_index < track_events.size(); ++event_index) {
                const RID event = track_events[event_index];
                const Ref<MaszynaVehicleCommandAction> action = scenario->event_get_action(event);
                if (!events->event_is_passive(event) || action.is_null() || !action->get_source().is_valid() ||
                    !(events->memory_get_text(action->get_source()) == SET_VELOCITY)) {
                    continue;
                }
                const double limit = events->memory_get_value1(action->get_source());
                if (limit == NO_LIMIT) {
                    continue;
                }
                // where its position meets the track (GetDistanceToEvent())
                const Ref<Curve3D> curve = roads->road_get_curve(segment->get_road());
                const double offset = curve.is_valid() ? curve->get_closest_offset(action->get_position()) : 0.0;
                const double ahead =
                        segment->get_distance() + (toward_end ? offset : segment->get_length() - offset) - front;
                if (ahead <= 0.0) {
                    continue; // passed
                }
                // a stop within its proximity is where it stands (the range it stops in, Driver.cpp:6785-6797)
                if (limit <= 0.0 && ahead <= max_proximity) {
                    return 0.0;
                }
                const double distance = MAX(ahead - (limit > 0.0 ? 0.0 : min_proximity), 0.0);
                // the velocity whose braking distance less the limit's is the distance there: v of
                // v * (v + 40) = limit * (limit + 40) + distance / braking
                const double half_offset = 0.5 * BRAKING_VELOCITY_OFFSET;
                const double reach = (limit * (limit + BRAKING_VELOCITY_OFFSET)) + (distance / DRIVER_BRAKING);
                velocity = MIN(velocity, Math::sqrt((half_offset * half_offset) + reach) - half_offset);
            }
        }
        return velocity;
    }

    void MaszynaCarDriver::_think(const RID &p_vehicle, Car &p_car) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        if (p_car.horn_off_time > 0.0 && time >= p_car.horn_off_time) {
            p_car.horn_off_time = 0.0;
            vehicle_server->vehicle_send_command(
                    p_vehicle, "horns_set", static_cast<int64_t>(RoadVehicleController::HORNS_NONE));
        }
        const double allowed = _road_velocity(p_vehicle, p_car.ordered);
        double wanted = allowed;
        if (Math::is_zero_approx(allowed) && Math::is_zero_approx(vehicle_server->vehicle_get_velocity(p_vehicle))) {
            p_car.stopped = true;
            p_car.move_off_time = 0.0;
        }
        // standing, it moves off a while after the way cleared (WaitingExpireTime, Driver.cpp:2737-2755)
        if (allowed > 0.0 && p_car.stopped) {
            if (p_car.move_off_time <= 0.0) {
                p_car.move_off_time = time + WAITING_EXPIRE_TIME;
            }
            if (time < p_car.move_off_time) {
                wanted = 0.0;
            } else {
                p_car.stopped = false;
            }
        }
        if (wanted == p_car.wanted) {
            return;
        }
        p_car.wanted = wanted;
        vehicle_server->vehicle_send_command(p_vehicle, "wanted_velocity_set", wanted);
    }
} // namespace godot
