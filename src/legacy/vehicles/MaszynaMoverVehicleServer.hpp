#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "vehicles/base/VehicleImplementationServer.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>

namespace godot {
    /* The vehicles simulated by the vendored Mover (TMoverParameters), registered with
     * VehicleServer as IMPLEMENTATION_NAME. It knows how its vehicles are stepped: all of them
     * together, in the original's phases (vehicle_table::update(), DynObj.cpp:8686-8724), with
     * RailVehicleServer doing what the rail does for each of them. */
    class MaszynaMoverVehicleServer : public VehicleImplementationServer {
            GDCLASS(MaszynaMoverVehicleServer, VehicleImplementationServer)

        private:
            /* Original engine: the primary physics update rate - a frame is integrated whole, in as
             * many steps as keep each at or below it (drivermode.cpp:193-206) */
            static constexpr double PHYSICS_STEP = 0.01;
            /* Velocity change of a vehicle within one frame reported as a kick (m/s^2) */
            static constexpr double DIAGNOSTICS_MAX_ACCELERATION = 3.0;

            /* The Mover of each vehicle on it, created and deleted here (mover_create(),
             * mover_free()); coupled Movers only know each other (TCoupling::Connected), hence the
             * way back to the vehicle */
            HashMap<RID, TMoverParameters *> movers;
            HashMap<const TMoverParameters *, RID> vehicles_by_mover;
            bool diagnostics = false;
            /* The velocity each vehicle had after the last step, for the kick diagnostics */
            HashMap<RID, double> diagnostics_velocity;

            /* Diagnostics: a velocity jump within one frame is a kick - with consistent track
             * movement it comes from the forces, typically a coupler reacting to an inconsistent
             * vehicle position. */
            void _check_velocity_jumps(
                    const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> &p_controllers, double p_delta);
            void _on_vehicle_freed(const RID &p_vehicle);
            /// The diagnostics follow their setting; switched on, they start from the next step
            void _on_project_settings_changed();

        protected:
            static void _bind_methods() {}

        public:
            static constexpr const char *IMPLEMENTATION_NAME = "maszyna_mover";

            static MaszynaMoverVehicleServer *get_instance() {
                return Object::cast_to<MaszynaMoverVehicleServer>(
                        Engine::get_singleton()->get_singleton("MaszynaMoverVehicleServer"));
            }

            MaszynaMoverVehicleServer();
            ~MaszynaMoverVehicleServer() override;

            /* C++ only and unbound: the Mover is this implementation's own business, and the
             * pointers stay in it - the vehicle's Mover controller and components take it from
             * here when its simulation starts and let go of it before it is freed. */
            TMoverParameters *mover_create(
                    const RID &p_vehicle, double p_velocity, const String &p_type_name, const String &p_name,
                    int p_cab);
            void mover_free(const RID &p_vehicle);
            TMoverParameters *mover_get(const RID &p_vehicle) const;
            /* The vehicle a Mover belongs to - what a coupler's Connected is traced back with */
            RID mover_get_vehicle(const TMoverParameters *p_mover) const;

            void stepping_advance(
                    const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> &p_controllers,
                    double p_delta) override;

            /* The bits of TMoverParameters::WarningSignal and EmergencyBrakeWarningSignal: low horn,
             * high horn, whistle (MOVER.h:2101; Train.cpp:7946-8037, DynObj.cpp:4893-4905) */
            static constexpr int WARNING_SIGNAL_HORN_LOW = 1;
            static constexpr int WARNING_SIGNAL_HORN_HIGH = 2;
            static constexpr int WARNING_SIGNAL_WHISTLE = 4;

            /* The vehicle interfaces' own enums as the vendored Mover spells them, for this
             * implementation's controller and components; the interfaces name no Mover type. */
            static Maszyna::start_t start_mode_to_mover(RailVehicleController::StartMode p_mode);
            static Maszyna::TEngineType engine_type_to_mover(RailVehicleEngine::EngineType p_type);
            static Maszyna::TPowerSource power_source_to_mover(RailVehicleController::TrainPowerSource p_source);
            static RailVehicleController::TrainPowerSource power_source_from_mover(Maszyna::TPowerSource p_source);
            static Maszyna::TPowerType power_type_to_mover(RailVehicleController::TrainPowerType p_type);
    };
} // namespace godot
