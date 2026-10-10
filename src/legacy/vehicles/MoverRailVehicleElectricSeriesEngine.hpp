#pragma once
#include "MoverCircuitUnit.hpp"
#include "MoverDriveUnit.hpp"
#include "MoverTractionMotorsUnit.hpp"
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleElectricSeriesEngine.hpp"

namespace godot {
    /* RailVehicleElectricSeriesEngine on the vendored Mover. It owns the units the engine is
     * composed of, and writes the series motor's own
     * configuration into the Mover, which none of the units covers. */
    class MoverRailVehicleElectricSeriesEngine : public RailVehicleElectricSeriesEngine, public MoverComponent {
            GDCLASS(MoverRailVehicleElectricSeriesEngine, RailVehicleElectricSeriesEngine);

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
            MoverCircuitUnit circuit_unit_impl{*this};
            MoverTractionMotorsUnit traction_motors_unit_impl{*this};


        public:
            MoverRailVehicleElectricSeriesEngine() {
                traction_motors_unit = &traction_motors_unit_impl;
                drive_unit = &drive_unit_impl;
                circuit_unit = &circuit_unit_impl;
            }

            double get_resistor_fan_rotation() const override;
            double get_circuit_imin() const override;
            bool get_circuit_imin_high_enabled() const override;
            double get_next_position_velocity(bool p_main_controller) const override;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
    };
} // namespace godot
