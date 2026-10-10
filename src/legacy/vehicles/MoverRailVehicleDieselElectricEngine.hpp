#pragma once
#include "MoverDieselEngineUnit.hpp"
#include "MoverDriveUnit.hpp"
#include "MoverTractionMotorsUnit.hpp"
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleDieselElectricEngine.hpp"

namespace godot {
    /* RailVehicleDieselElectricEngine on the vendored Mover. It owns the units the engine is
     * composed of, and writes the diesel-electric engine's own
     * configuration into the Mover, which none of the units covers. */
    class MoverRailVehicleDieselElectricEngine : public RailVehicleDieselElectricEngine, public MoverComponent {
            GDCLASS(MoverRailVehicleDieselElectricEngine, RailVehicleDieselElectricEngine);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();
            MoverDriveUnit drive_unit_impl{*this};
            MoverDieselEngineUnit diesel_engine_unit_impl{*this};
            MoverTractionMotorsUnit traction_motors_unit_impl{*this};


        public:
            MoverRailVehicleDieselElectricEngine() {
                traction_motors_unit = &traction_motors_unit_impl;
                drive_unit = &drive_unit_impl;
                diesel_engine_unit = &diesel_engine_unit_impl;
            }


        protected:
            /// Mover.cpp:8551-8552 (readWWList) - the shunting power bounds of a WWList row
            static constexpr double WWLIST_SHUNT_POWER_DIVISOR = 47.6;

            void _apply_configuration() override;
    };
} // namespace godot
