#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleCircuitUnit.hpp"

namespace godot {
    /* The traction circuit on the vendored Mover. */
    class MoverCircuitUnit : public RailVehicleCircuitUnit {
        private:
            /* The Mover* component that owns this unit - it reaches the Mover through it. */
            const MoverComponent &owner;

        public:
            explicit MoverCircuitUnit(const MoverComponent &p_owner) : owner(p_owner) {}

            bool get_contactors_active() const override;
            bool get_diff_relay_active() const override;
            bool get_resistors_active() const override;
            bool get_vent_overload_active() const override;
            bool get_highcurrent_active() const override;
            bool get_mainbreaker_active() const override;
            bool get_camshaft_available() const override;
            bool get_converter_overload() const override;
            double get_line_breaker_delay() const override;
            double get_line_breaker_initial_delay() const override;
            bool get_line_breaker_closes_at_no_power() const override;
            void apply_configuration(const RailVehicleElectricEngine *p_engine) const override;
            void converter_fuse_reset() const override;
    };
} // namespace godot
