#include "MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverRailVehicleController.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <map>

#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/worker_thread_pool.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /* Reports physics inconsistencies with push_error (see _check_velocity_jumps) */
    constexpr const char *DIAGNOSTICS_SETTING = "maszyna/debug/physics_diagnostics";

    /* A freed vehicle's last velocity goes with it. No explicit disconnect: callable_mp reports
     * this instance as the callable's object, so the engine drops the connection when it dies. */
    MaszynaMoverVehicleServer *MaszynaMoverVehicleServer::singleton = nullptr;

    MaszynaMoverVehicleServer::MaszynaMoverVehicleServer() {
        singleton = this;
        diagnostics = ProjectSettings::get_singleton()->get_setting(DIAGNOSTICS_SETTING, false);
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &MaszynaMoverVehicleServer::_on_project_settings_changed));
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &MaszynaMoverVehicleServer::_on_vehicle_freed));
    }

    MaszynaMoverVehicleServer::~MaszynaMoverVehicleServer() {
        singleton = nullptr;
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
    /* p_controllers are this implementation's own - MoverRailVehicleController is the one class
     * that names it (MoverRailVehicleController.cpp:45) - hence the unchecked cast; every vehicle
     * is attached to the rail before its controller is bound (RailVehiclePhysicsNode::
     * _prepare_vehicle()) */
    void MaszynaMoverVehicleServer::stepping_advance(
            const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> &p_controllers, const double p_delta) {
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail_vehicles);
        if (p_delta <= 0.0) {
            return;
        }

        for (const RID &vehicle_rid: p_vehicles) {
            rail_vehicles->vehicle_report_position(vehicle_rid);
        }

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
            for (const RID &vehicle_rid: p_vehicles) {
                rail_vehicles->vehicle_update_location(vehicle_rid);
            }
            for (const RID &vehicle_rid: p_vehicles) {
                rail_vehicles->vehicle_update_neighbours(vehicle_rid);
            }
            // the groups follow the neighbours just found
            _group_vehicles(p_controllers);
            StepPhase phase;
            phase.groups = &step_groups;
            phase.controllers = &p_controllers;
            phase.sub_step = sub_step;
            // the cheap integration in every sub-iteration but the last (DynObj.cpp:4086)
            phase.full_movement = iteration == iterations - 1;
            // the original computes the forces of every vehicle before moving any of them, so
            // coupled vehicles see a consistent state (DynObj.cpp:8199-8205)
            _run_phase(&MaszynaMoverVehicleServer::_compute_forces_task, phase, "Mover forces");
            _run_phase(&MaszynaMoverVehicleServer::_compute_movement_task, phase, "Mover movement");
            // the track is the server's, its events and signals the main thread's: each vehicle
            // takes its movement along it after all have computed theirs, in their order
            for (int index = 0; index < p_vehicles.size(); ++index) {
                // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): see stepping_advance()
                if (static_cast<RailVehicleController *>(p_controllers[index].ptr())->is_physics_active()) {
                    rail_vehicles->vehicle_process_movement(p_vehicles[index], sub_step);
                }
            }
        }

        for (int index = 0; index < p_vehicles.size(); ++index) {
            VehicleController *controller = p_controllers[index].ptr();
            if (controller->is_physics_active()) {
                controller->update_state();
            }
            rail_vehicles->vehicle_collect_current(p_vehicles[index], p_delta);
            controller->process_components(p_delta);
            rail_vehicles->vehicle_report_track_standing(p_vehicles[index]);
        }
        if (diagnostics) {
            _check_velocity_jumps(p_vehicles, p_controllers, p_delta);
        }

        for (const RID &vehicle_rid: p_vehicles) {
            rail_vehicles->vehicle_report_placement(vehicle_rid);
        }
    }

    /* Union-find over the Movers' ends: a vehicle and whatever stands at its ends - coupled or
     * not, stepped here or not - end up in one group */
    void MaszynaMoverVehicleServer::_group_vehicles(const Vector<Ref<VehicleController>> &p_controllers) {
        StepGroups &groups = step_groups;
        const uint32_t count = p_controllers.size();
        groups.indices.clear();
        groups.parents.clear();
        for (uint32_t index = 0; index < count; ++index) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): see stepping_advance()
            const auto *controller = static_cast<MoverRailVehicleController *>(p_controllers[index].ptr());
            const TMoverParameters *mover = controller->get_mover();
            if (mover != nullptr) {
                groups.indices.insert(mover, index);
            }
            groups.parents.push_back(index);
        }
        const auto find = [&groups](uint32_t p_node) {
            while (groups.parents[p_node] != p_node) {
                groups.parents[p_node] = groups.parents[groups.parents[p_node]];
                p_node = groups.parents[p_node];
            }
            return p_node;
        };
        for (uint32_t index = 0; index < count; ++index) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): see stepping_advance()
            const auto *controller = static_cast<MoverRailVehicleController *>(p_controllers[index].ptr());
            const TMoverParameters *mover = controller->get_mover();
            if (mover == nullptr) {
                continue;
            }
            for (const auto &neighbour: mover->Neighbours) {
                if (neighbour.vehicle == nullptr) {
                    continue;
                }
                const uint32_t *other = groups.indices.getptr(neighbour.vehicle);
                uint32_t other_index = 0;
                if (other != nullptr) {
                    other_index = *other;
                } else {
                    // a Mover not stepped here is still written to by both its neighbours
                    other_index = groups.parents.size();
                    groups.parents.push_back(other_index);
                    groups.indices.insert(neighbour.vehicle, other_index);
                }
                const uint32_t root = find(index);
                const uint32_t other_root = find(other_index);
                if (root != other_root) {
                    groups.parents[MAX(root, other_root)] = MIN(root, other_root);
                }
            }
        }
        // a group per root, its vehicles in their order
        groups.counts.resize(groups.parents.size());
        for (uint32_t &group_count: groups.counts) {
            group_count = 0;
        }
        for (uint32_t index = 0; index < count; ++index) {
            ++groups.counts[find(index)];
        }
        groups.starts.clear();
        uint32_t start = 0;
        for (uint32_t root = 0; root < groups.counts.size(); ++root) {
            const uint32_t group_count = groups.counts[root];
            if (group_count == 0) {
                continue;
            }
            groups.starts.push_back(start);
            // from here on counts[root] is where the root's next vehicle goes
            groups.counts[root] = start;
            start += group_count;
        }
        groups.starts.push_back(start);
        groups.members.resize(count);
        for (uint32_t index = 0; index < count; ++index) {
            groups.members[groups.counts[find(index)]++] = index;
        }
    }

    void MaszynaMoverVehicleServer::_run_phase(
            void (*p_task)(void *, uint32_t), StepPhase &p_phase, const char *p_description) {
        const uint32_t group_count = step_groups.starts.size() - 1;
        WorkerThreadPool *pool = WorkerThreadPool::get_singleton();
        if (pool == nullptr || group_count < PARALLEL_MIN_GROUPS) {
            for (uint32_t group = 0; group < group_count; ++group) {
                p_task(&p_phase, group);
            }
            return;
        }
        const int64_t task = pool->add_native_group_task(
                p_task, &p_phase, static_cast<int>(group_count), -1, true, String(p_description));
        pool->wait_for_group_task_completion(task);
    }

    void MaszynaMoverVehicleServer::_compute_forces_task(void *p_phase, const uint32_t p_group) {
        const StepPhase *phase = static_cast<const StepPhase *>(p_phase);
        const StepGroups &groups = *phase->groups;
        for (uint32_t member = groups.starts[p_group]; member < groups.starts[p_group + 1]; ++member) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): see stepping_advance()
            static_cast<RailVehicleController *>((*phase->controllers)[groups.members[member]].ptr())
                    ->compute_forces(phase->sub_step);
        }
    }

    void MaszynaMoverVehicleServer::_compute_movement_task(void *p_phase, const uint32_t p_group) {
        const StepPhase *phase = static_cast<const StepPhase *>(p_phase);
        const StepGroups &groups = *phase->groups;
        for (uint32_t member = groups.starts[p_group]; member < groups.starts[p_group + 1]; ++member) {
            const Ref<VehicleController> &stepped = (*phase->controllers)[groups.members[member]];
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast): see stepping_advance()
            auto *controller = static_cast<RailVehicleController *>(stepped.ptr());
            if (!controller->is_physics_active()) {
                continue;
            }
            if (phase->full_movement) {
                controller->compute_movement(phase->sub_step);
            } else {
                controller->compute_fast_movement(phase->sub_step);
            }
        }
    }

    void MaszynaMoverVehicleServer::_check_velocity_jumps(
            const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> &p_controllers, const double p_delta) {
        for (int index = 0; index < p_vehicles.size(); ++index) {
            const VehicleController *controller = p_controllers[index].ptr();
            const double velocity = controller->get_velocity();
            const double *previous = diagnostics_velocity.getptr(p_vehicles[index]);
            const double acceleration = (velocity - (previous != nullptr ? *previous : velocity)) / p_delta;
            diagnostics_velocity[p_vehicles[index]] = velocity;
            if (Math::abs(acceleration) > DIAGNOSTICS_MAX_ACCELERATION) {
                UtilityFunctions::push_error(
                        vformat("MaszynaMoverVehicleServer: %s kicked, dV/dt=%.2f m/s^2 at V=%.2f m/s",
                                controller->get_vehicle_id(), acceleration, velocity));
            }
        }
    }

    Maszyna::start_t MaszynaMoverVehicleServer::start_mode_to_mover(const RailVehicleController::StartMode p_mode) {
        static const std::map<RailVehicleController::StartMode, Maszyna::start_t> map = {
                {RailVehicleController::START_MODE_DISABLED, Maszyna::start_t::disabled},
                {RailVehicleController::START_MODE_MANUAL, Maszyna::start_t::manual},
                {RailVehicleController::START_MODE_AUTOMATIC, Maszyna::start_t::automatic},
                {RailVehicleController::START_MODE_MANUAL_WITH_AUTO_FALLBACK, Maszyna::start_t::manualwithautofallback},
                {RailVehicleController::START_MODE_CONVERTER, Maszyna::start_t::converter},
                {RailVehicleController::START_MODE_BATTERY, Maszyna::start_t::battery},
                {RailVehicleController::START_MODE_DIRECTION, Maszyna::start_t::direction},
        };
        return map.at(p_mode);
    }

    Maszyna::TEngineType MaszynaMoverVehicleServer::engine_type_to_mover(const RailVehicleEngine::EngineType p_type) {
        static const std::map<RailVehicleEngine::EngineType, Maszyna::TEngineType> map = {
                {RailVehicleEngine::NONE, Maszyna::TEngineType::None},
                {RailVehicleEngine::DUMB, Maszyna::TEngineType::Dumb},
                {RailVehicleEngine::WHEELS_DRIVEN, Maszyna::TEngineType::WheelsDriven},
                {RailVehicleEngine::ELECTRIC_SERIES_MOTOR, Maszyna::TEngineType::ElectricSeriesMotor},
                {RailVehicleEngine::ELECTRIC_INDUCTION_MOTOR, Maszyna::TEngineType::ElectricInductionMotor},
                {RailVehicleEngine::DIESEL, Maszyna::TEngineType::DieselEngine},
                {RailVehicleEngine::STEAM, Maszyna::TEngineType::SteamEngine},
                {RailVehicleEngine::DIESEL_ELECTRIC, Maszyna::TEngineType::DieselElectric},
                {RailVehicleEngine::MAIN, Maszyna::TEngineType::Main},
        };
        return map.at(p_type);
    }

    Maszyna::TPowerSource
    MaszynaMoverVehicleServer::power_source_to_mover(const RailVehicleController::TrainPowerSource p_source) {
        static const std::map<RailVehicleController::TrainPowerSource, Maszyna::TPowerSource> map = {
                {RailVehicleController::POWER_SOURCE_NOT_DEFINED, Maszyna::TPowerSource::NotDefined},
                {RailVehicleController::POWER_SOURCE_INTERNAL, Maszyna::TPowerSource::InternalSource},
                {RailVehicleController::POWER_SOURCE_TRANSDUCER, Maszyna::TPowerSource::Transducer},
                {RailVehicleController::POWER_SOURCE_GENERATOR, Maszyna::TPowerSource::Generator},
                {RailVehicleController::POWER_SOURCE_ACCUMULATOR, Maszyna::TPowerSource::Accumulator},
                {RailVehicleController::POWER_SOURCE_CURRENTCOLLECTOR, Maszyna::TPowerSource::CurrentCollector},
                {RailVehicleController::POWER_SOURCE_POWERCABLE, Maszyna::TPowerSource::PowerCable},
                {RailVehicleController::POWER_SOURCE_HEATER, Maszyna::TPowerSource::Heater},
                {RailVehicleController::POWER_SOURCE_MAIN, Maszyna::TPowerSource::Main},
        };
        return map.at(p_source);
    }

    RailVehicleController::TrainPowerSource
    MaszynaMoverVehicleServer::power_source_from_mover(const Maszyna::TPowerSource p_source) {
        static const std::map<Maszyna::TPowerSource, RailVehicleController::TrainPowerSource> map = {
                {Maszyna::TPowerSource::NotDefined, RailVehicleController::POWER_SOURCE_NOT_DEFINED},
                {Maszyna::TPowerSource::InternalSource, RailVehicleController::POWER_SOURCE_INTERNAL},
                {Maszyna::TPowerSource::Transducer, RailVehicleController::POWER_SOURCE_TRANSDUCER},
                {Maszyna::TPowerSource::Generator, RailVehicleController::POWER_SOURCE_GENERATOR},
                {Maszyna::TPowerSource::Accumulator, RailVehicleController::POWER_SOURCE_ACCUMULATOR},
                {Maszyna::TPowerSource::CurrentCollector, RailVehicleController::POWER_SOURCE_CURRENTCOLLECTOR},
                {Maszyna::TPowerSource::PowerCable, RailVehicleController::POWER_SOURCE_POWERCABLE},
                {Maszyna::TPowerSource::Heater, RailVehicleController::POWER_SOURCE_HEATER},
                {Maszyna::TPowerSource::Main, RailVehicleController::POWER_SOURCE_MAIN},
        };
        return map.at(p_source);
    }

    Maszyna::TPowerType
    MaszynaMoverVehicleServer::power_type_to_mover(const RailVehicleController::TrainPowerType p_type) {
        static const std::map<RailVehicleController::TrainPowerType, Maszyna::TPowerType> map = {
                {RailVehicleController::POWER_TYPE_NONE, Maszyna::TPowerType::NoPower},
                {RailVehicleController::POWER_TYPE_BIO, Maszyna::TPowerType::BioPower},
                {RailVehicleController::POWER_TYPE_MECH, Maszyna::TPowerType::MechPower},
                {RailVehicleController::POWER_TYPE_ELECTRIC, Maszyna::TPowerType::ElectricPower},
                {RailVehicleController::POWER_TYPE_STEAM, Maszyna::TPowerType::SteamPower},
        };
        return map.at(p_type);
    }
} // namespace godot
