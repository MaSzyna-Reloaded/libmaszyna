#include "StationServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    const char *StationServer::dispatch_step_changed_signal = "dispatch_step_changed";
    const char *StationServer::dispatch_finished_signal = "dispatch_finished";

    void StationServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("dispatch_start", "vehicle", "cars"), &StationServer::dispatch_start);
        ClassDB::bind_method(D_METHOD("dispatch_depart", "vehicle"), &StationServer::dispatch_depart);
        ClassDB::bind_method(D_METHOD("dispatch_cancel", "vehicle"), &StationServer::dispatch_cancel);
        ClassDB::bind_method(D_METHOD("dispatch_get_step", "vehicle"), &StationServer::dispatch_get_step);
        ClassDB::bind_method(
                D_METHOD("dispatch_get_exchange_time", "vehicle"), &StationServer::dispatch_get_exchange_time);

        BIND_ENUM_CONSTANT(DISPATCH_STEP_NONE);
        BIND_ENUM_CONSTANT(DISPATCH_STEP_EXCHANGE);
        BIND_ENUM_CONSTANT(DISPATCH_STEP_WAIT_DEPARTURE);
        BIND_ENUM_CONSTANT(DISPATCH_STEP_CLOSE_DOORS);

        ADD_SIGNAL(MethodInfo(
                dispatch_step_changed_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::INT, "step", PROPERTY_HINT_ENUM, "None,Exchange,Wait departure,Close doors")));
        ADD_SIGNAL(MethodInfo(dispatch_finished_signal, PropertyInfo(Variant::RID, "vehicle")));
    }

    /// A freed vehicle takes its train's dispatch with it. No explicit disconnect: callable_mp
    /// reports this instance as the callable's object, so the engine drops the connection when it
    /// dies.
    StationServer::StationServer() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(VehicleServer::vehicle_freed_signal, callable_mp(this, &StationServer::_on_vehicle_freed));
    }

    void StationServer::dispatch_start(const RID &p_vehicle, const TypedArray<RID> &p_cars) {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        dispatch_cancel(p_vehicle);
        Dispatch &dispatch = dispatches[p_vehicle];
        for (int index = 0; index < p_cars.size(); ++index) {
            Car car;
            car.vehicle = p_cars[index];
            car.load = vehicles->vehicle_component_get(car.vehicle, VehicleComponentType::COMPONENT_LOAD);
            car.doors = vehicles->vehicle_component_get(car.vehicle, VehicleComponentType::COMPONENT_DOORS);
            if (car.load.is_valid()) {
                car.load->connect(
                        RailVehicleLoad::load_exchange_finished_signal,
                        callable_mp(this, &StationServer::_on_load_exchange_finished).bind(p_vehicle, car.vehicle));
                if (car.load->get_load_exchange_time() > 0.0) {
                    dispatch.exchanging.insert(car.vehicle);
                }
            }
            if (car.doors.is_valid()) {
                car.doors->connect(
                        RailVehicleDoors::doors_closed_signal,
                        callable_mp(this, &StationServer::_on_doors_closed).bind(p_vehicle, car.vehicle));
            }
            dispatch.cars.push_back(car);
        }
        _set_step(
                p_vehicle, dispatch,
                dispatch.exchanging.is_empty() ? DISPATCH_STEP_WAIT_DEPARTURE : DISPATCH_STEP_EXCHANGE);
    }

    void StationServer::dispatch_depart(const RID &p_vehicle) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        if (dispatch == nullptr) {
            return;
        }
        dispatch->departure_allowed = true;
        if (dispatch->step == DISPATCH_STEP_WAIT_DEPARTURE) {
            _close_doors(p_vehicle, *dispatch);
        }
    }

    void StationServer::dispatch_cancel(const RID &p_vehicle) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        if (dispatch == nullptr) {
            return;
        }
        for (const Car &car: dispatch->cars) {
            if (car.load.is_valid()) {
                car.load->disconnect(
                        RailVehicleLoad::load_exchange_finished_signal,
                        callable_mp(this, &StationServer::_on_load_exchange_finished).bind(p_vehicle, car.vehicle));
            }
            if (car.doors.is_valid()) {
                car.doors->disconnect(
                        RailVehicleDoors::doors_closed_signal,
                        callable_mp(this, &StationServer::_on_doors_closed).bind(p_vehicle, car.vehicle));
            }
        }
        dispatches.erase(p_vehicle);
        emit_signal(dispatch_step_changed_signal, p_vehicle, DISPATCH_STEP_NONE);
    }

    StationServer::DispatchStep StationServer::dispatch_get_step(const RID &p_vehicle) const {
        const Dispatch *dispatch = dispatches.getptr(p_vehicle);
        return dispatch != nullptr ? dispatch->step : DISPATCH_STEP_NONE;
    }

    double StationServer::dispatch_get_exchange_time(const RID &p_vehicle) const {
        const Dispatch *dispatch = dispatches.getptr(p_vehicle);
        double exchange_time = 0.0;
        if (dispatch == nullptr) {
            return exchange_time;
        }
        for (const Car &car: dispatch->cars) {
            if (car.load.is_valid()) {
                exchange_time = MAX(exchange_time, car.load->get_load_exchange_time());
            }
        }
        return exchange_time;
    }

    void StationServer::_set_step(const RID &p_vehicle, Dispatch &p_dispatch, const DispatchStep p_step) {
        p_dispatch.step = p_step;
        emit_signal(dispatch_step_changed_signal, p_vehicle, p_step);
    }

    /// Every car whose doors are not closed yet is waited for; with none, the dispatch is over
    void StationServer::_close_doors(const RID &p_vehicle, Dispatch &p_dispatch) {
        for (const Car &car: p_dispatch.cars) {
            if (car.doors.is_valid() && !(car.doors->get_left_closed() && car.doors->get_right_closed())) {
                p_dispatch.doors_not_closed.insert(car.vehicle);
            }
        }
        if (p_dispatch.doors_not_closed.is_empty()) {
            _finish(p_vehicle);
            return;
        }
        _set_step(p_vehicle, p_dispatch, DISPATCH_STEP_CLOSE_DOORS);
    }

    void StationServer::_finish(const RID &p_vehicle) {
        dispatch_cancel(p_vehicle);
        emit_signal(dispatch_finished_signal, p_vehicle);
    }

    void StationServer::_on_load_exchange_finished(const RID &p_vehicle, const RID &p_car) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        ERR_FAIL_NULL(dispatch);
        dispatch->exchanging.erase(p_car);
        if (dispatch->step != DISPATCH_STEP_EXCHANGE || !dispatch->exchanging.is_empty()) {
            return;
        }
        if (dispatch->departure_allowed) {
            _close_doors(p_vehicle, *dispatch);
            return;
        }
        _set_step(p_vehicle, *dispatch, DISPATCH_STEP_WAIT_DEPARTURE);
    }

    void StationServer::_on_doors_closed(const int p_side, const RID &p_vehicle, const RID &p_car) {
        Dispatch *dispatch = dispatches.getptr(p_vehicle);
        ERR_FAIL_NULL(dispatch);
        if (dispatch->step != DISPATCH_STEP_CLOSE_DOORS) {
            return;
        }
        for (const Car &car: dispatch->cars) {
            if (car.vehicle == p_car && car.doors->get_left_closed() && car.doors->get_right_closed()) {
                dispatch->doors_not_closed.erase(p_car);
            }
        }
        if (dispatch->doors_not_closed.is_empty()) {
            _finish(p_vehicle);
        }
    }

    void StationServer::_on_vehicle_freed(const RID &p_vehicle) {
        Vector<RID> dropped;
        for (const KeyValue<RID, Dispatch> &entry: dispatches) {
            if (entry.key == p_vehicle) {
                dropped.push_back(entry.key);
                continue;
            }
            for (const Car &car: entry.value.cars) {
                if (car.vehicle == p_vehicle) {
                    dropped.push_back(entry.key);
                }
            }
        }
        for (const RID &vehicle: dropped) {
            dispatch_cancel(vehicle);
        }
    }
} // namespace godot
