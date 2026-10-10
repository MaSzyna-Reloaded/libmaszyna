#pragma once
#include "MoverCircuitUnit.hpp"
#include "MoverDriveUnit.hpp"
#include "MoverTractionMotorsUnit.hpp"
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleElectricInductionEngine.hpp"

namespace godot {
    /* RailVehicleElectricInductionEngine on the vendored Mover. It owns the units the engine is
     * composed of, and writes the induction motor's own
     * configuration into the Mover, which none of the units covers. */
    class MoverRailVehicleElectricInductionEngine : public RailVehicleElectricInductionEngine, public MoverComponent {
            GDCLASS(MoverRailVehicleElectricInductionEngine, RailVehicleElectricInductionEngine);

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
            MoverRailVehicleElectricInductionEngine() {
                traction_motors_unit = &traction_motors_unit_impl;
                drive_unit = &drive_unit_impl;
                circuit_unit = &circuit_unit_impl;
            }

            TypedArray<RailVehicleInverter> get_inverters() const override;
            double get_force_max() const override;
            double get_force_full() const override;
            double get_field_current() const override;
            double get_motor_voltage() const override;

        protected:
            void _apply_configuration() override;
    };
} // namespace godot
