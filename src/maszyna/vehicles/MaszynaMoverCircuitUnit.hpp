#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverElectricEngineConfiguration.hpp"
#include "vehicles/rail/RailVehicleCircuitUnit.hpp"

namespace godot {
    /* The traction circuit on the vendored Mover. */
    class MaszynaMoverCircuitUnit : public RailVehicleCircuitUnit {
        private:
            /* The MaszynaMover* component that owns this unit - it reaches the Mover through it. */
            const MaszynaMoverComponent &owner;
            /* What the owner's Mover model is configured with - the owner's own property */
            const Ref<MaszynaMoverElectricEngineConfiguration> &configuration;

        public:
            MaszynaMoverCircuitUnit(
                    const MaszynaMoverComponent &p_owner,
                    const Ref<MaszynaMoverElectricEngineConfiguration> &p_configuration) :
                owner(p_owner), configuration(p_configuration) {}

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
