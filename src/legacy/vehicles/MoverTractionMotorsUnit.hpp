#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleTractionMotorsUnit.hpp"

namespace godot {
    /* The traction motors on the vendored Mover. */
    class MoverTractionMotorsUnit : public RailVehicleTractionMotorsUnit {
        private:
            /* The Mover* component that owns this unit - it reaches the Mover through it. */
            const MoverComponent &owner;

        public:
            explicit MoverTractionMotorsUnit(const MoverComponent &p_owner) : owner(p_owner) {}

            double get_motor_current() const override;
            double get_engine_voltage() const override;
            double get_total_current() const override;
            double get_circuit_imax() const override;
            bool get_dynamic_brake_active() const override;
            bool get_fuse_active() const override;
            bool get_motor_connectors_open() const override;
            bool is_line_contactor_closed() const override;
            bool is_pressure_switch_tripped() const override;
            void fuse_reset() const override;
            void set_motor_connectors_open(bool p_open) const override;
    };
} // namespace godot
