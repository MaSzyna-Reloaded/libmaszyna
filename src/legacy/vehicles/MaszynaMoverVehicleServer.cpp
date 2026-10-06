#include "MaszynaMoverVehicleServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /* Reports physics inconsistencies with push_error (see _check_velocity_jumps) */
    constexpr const char *DIAGNOSTICS_SETTING = "maszyna/debug/physics_diagnostics";

    /* A freed vehicle's last velocity goes with it. No explicit disconnect: callable_mp reports
     * this instance as the callable's object, so the engine drops the connection when it dies. */
    MaszynaMoverVehicleServer::MaszynaMoverVehicleServer() {
        diagnostics = ProjectSettings::get_singleton()->get_setting(DIAGNOSTICS_SETTING, false);
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &MaszynaMoverVehicleServer::_on_project_settings_changed));
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &MaszynaMoverVehicleServer::_on_vehicle_freed));
    }

    MaszynaMoverVehicleServer::~MaszynaMoverVehicleServer() {
        for (const KeyValue<RID, TMoverParameters *> &entry: movers) {
            delete entry.value;
        }
    }

    TMoverParameters *MaszynaMoverVehicleServer::mover_create(
            const RID &p_vehicle, const double p_velocity, const String &p_type_name, const String &p_name,
            const int p_cab) {
        ERR_FAIL_COND_V_MSG(!p_vehicle.is_valid(), nullptr, "A Mover belongs to a vehicle");
        ERR_FAIL_COND_V_MSG(movers.has(p_vehicle), movers[p_vehicle], "The vehicle has its Mover already");
        TMoverParameters *mover = new TMoverParameters(
                p_velocity, std::string(p_type_name.utf8().get_data()), std::string(p_name.utf8().get_data()), p_cab);
        movers.insert(p_vehicle, mover);
        vehicles_by_mover.insert(mover, p_vehicle);
        return mover;
    }

    void MaszynaMoverVehicleServer::mover_free(const RID &p_vehicle) {
        TMoverParameters **mover = movers.getptr(p_vehicle);
        if (mover == nullptr) {
            return;
        }
        vehicles_by_mover.erase(*mover);
        delete *mover;
        movers.erase(p_vehicle);
    }

    TMoverParameters *MaszynaMoverVehicleServer::mover_get(const RID &p_vehicle) const {
        TMoverParameters *const *mover = movers.getptr(p_vehicle);
        return mover != nullptr ? *mover : nullptr;
    }

    RID MaszynaMoverVehicleServer::mover_get_vehicle(const TMoverParameters *p_mover) const {
        const RID *vehicle = vehicles_by_mover.getptr(p_mover);
        return vehicle != nullptr ? *vehicle : RID();
    }

    /// The velocities kept from before the diagnostics went off are no previous step to compare with
    void MaszynaMoverVehicleServer::_on_project_settings_changed() {
        const bool enabled = ProjectSettings::get_singleton()->get_setting(DIAGNOSTICS_SETTING, false);
        if (enabled && !diagnostics) {
            diagnostics_velocity.clear();
        }
        diagnostics = enabled;
    }

    void MaszynaMoverVehicleServer::_on_vehicle_freed(const RID &p_vehicle) {
        diagnostics_velocity.erase(p_vehicle);
    }

    /* The whole step of the vehicles on the Mover, in the phase order of the original's
     * vehicle_table::update() (DynObj.cpp:8686-8724): locations and neighbours, then the forces
     * of all before the movement of all in each sub-iteration.
     *
     * It runs on the rendered frame, not on Godot's fixed tick, exactly like the original - on the
     * fixed tick the same step ran several times per frame to catch up and the vehicles juddered.
     * The vehicles are handed their new placement at the end of this rather than pulling it
     * themselves on their own beat. */
    void MaszynaMoverVehicleServer::stepping_advance(const Vector<RID> &p_vehicles, const double p_delta) {
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        ERR_FAIL_NULL(vehicle_server);
        if (p_delta <= 0.0) {
            return;
        }
        stepped_vehicles.clear();
        stepped_controllers.clear();
        for (const RID &vehicle_rid: p_vehicles) {
            // only a vehicle on the rail is stepped - one not attached there has nowhere to move
            const Ref<RailVehicleController> controller = Object::cast_to<RailVehicleController>(
                    ObjectDB::get_instance(ObjectID(vehicle_server->vehicle_get_controller_instance_id(vehicle_rid))));
            if (controller.is_null() || !rail_vehicles->vehicle_is_attached(vehicle_rid)) {
                continue;
            }
            stepped_vehicles.push_back(vehicle_rid);
            stepped_controllers.push_back(controller);
        }
        if (stepped_vehicles.is_empty()) {
            return;
        }

        for (const RID &vehicle_rid: stepped_vehicles) {
            rail_vehicles->vehicle_report_position(vehicle_rid);
        }
        rail_vehicles->neighbour_index_rebuild();

        // the whole frame, in steps no longer than PHYSICS_STEP; the clock caps the frame
        // (drivermode.cpp:193-206)
        const int iterations = MAX(static_cast<int>(Math::ceil(p_delta / PHYSICS_STEP)), 1);
        const double sub_step = p_delta / iterations;
        for (int iteration = 0; iteration < iterations; ++iteration) {
            // DELIBERATE DEPARTURE FROM THE ORIGINAL - do not move this back out of the loop.
            //
            // The original refreshes the vehicles' locations and neighbour distances once a frame
            // (vehicle_table::update(), DynObj.cpp:8691-8699), before all its sub-steps. The Mover
            // does not measure a coupler from the positions, though: CouplerForce()
            // (Mover.cpp:4779-4784) takes the distance set by that refresh and adds TEN TIMES what
            // the two vehicles moved relative to each other since (dMoveLen, reset with the
            // location). The error of that term grows with the time since the refresh, so the
            // longer the frame, the stiffer and more wrongly loaded every coupler is. At 60 fps
            // (1-2 sub-steps) it does not show; at 0.17 s a frame (17 sub-steps - a slow machine,
            // or any simulation speed above 1) the eszelon's 21 vehicles locked up: 391 kN at
            // the wheels, 0.18 m/s, for minutes. Measured with the same start stepped at 0.03 s
            // and at 0.17 s a frame (FINDINGS.md, 2026-09-27 "couplers stiffened by a long
            // frame").
            //
            // Refreshed here, before every sub-step, the ten-fold term only ever spans one
            // PHYSICS_STEP whatever the frame length, which is what the original does at 100 fps;
            // the two frame lengths then give the same run to within 0.1 m/s. The Mover itself
            // (vendored) is left as it is. The cost: the locations and the neighbour scan run per
            // sub-step, not per frame - at 60 fps the same as before, on a slow frame up to
            // MAX_FRAME_TIME / PHYSICS_STEP times.
            for (const RID &vehicle_rid: stepped_vehicles) {
                rail_vehicles->vehicle_update_location(vehicle_rid);
            }
            for (const RID &vehicle_rid: stepped_vehicles) {
                rail_vehicles->vehicle_update_neighbours(vehicle_rid);
            }
            // the original computes the forces of every vehicle before moving any of them, so
            // coupled vehicles see a consistent state (DynObj.cpp:8199-8205)
            for (const Ref<RailVehicleController> &controller: stepped_controllers) {
                controller->compute_forces(sub_step);
            }
            const bool full_movement = iteration == iterations - 1;
            for (int index = 0; index < stepped_vehicles.size(); ++index) {
                const Ref<RailVehicleController> &controller = stepped_controllers[index];
                if (!controller->is_physics_active()) {
                    continue;
                }
                // the cheap integration in every sub-iteration but the last (DynObj.cpp:4086)
                if (full_movement) {
                    controller->compute_movement(sub_step);
                } else {
                    controller->compute_fast_movement(sub_step);
                }
                rail_vehicles->vehicle_process_movement(stepped_vehicles[index], sub_step);
            }
        }

        for (int index = 0; index < stepped_vehicles.size(); ++index) {
            const Ref<RailVehicleController> &controller = stepped_controllers[index];
            if (controller->is_physics_active()) {
                controller->update_state();
            }
            rail_vehicles->vehicle_collect_current(stepped_vehicles[index], p_delta);
            controller->process_components(p_delta);
            rail_vehicles->vehicle_report_track_heading(stepped_vehicles[index]);
        }
        if (diagnostics) {
            _check_velocity_jumps(p_delta);
        }

        for (const RID &vehicle_rid: stepped_vehicles) {
            rail_vehicles->vehicle_report_placement(vehicle_rid);
        }
    }

    void MaszynaMoverVehicleServer::_check_velocity_jumps(const double p_delta) {
        for (int index = 0; index < stepped_vehicles.size(); ++index) {
            const Ref<RailVehicleController> &controller = stepped_controllers[index];
            const double velocity = controller->get_velocity();
            const double *previous = diagnostics_velocity.getptr(stepped_vehicles[index]);
            const double acceleration = (velocity - (previous != nullptr ? *previous : velocity)) / p_delta;
            diagnostics_velocity[stepped_vehicles[index]] = velocity;
            if (Math::abs(acceleration) > DIAGNOSTICS_MAX_ACCELERATION) {
                UtilityFunctions::push_error(
                        vformat("MaszynaMoverVehicleServer: %s kicked, dV/dt=%.2f m/s^2 at V=%.2f m/s",
                                controller->get_vehicle_id(), acceleration, velocity));
            }
        }
    }
} // namespace godot
