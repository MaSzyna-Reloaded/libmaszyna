#pragma once
#include "scenario/ScenarioEventAction.hpp"
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's `voltage` event (voltage_event, Event.cpp:1993-2002): sets the nominal voltage
    /// of its traction power sources (TTractionPowerSource::VoltageSet())
    class MaszynaLegacyVoltageAction : public ScenarioEventAction {
            GDCLASS(MaszynaLegacyVoltageAction, ScenarioEventAction)

        private:
            TypedArray<RID> power_sources;
            double voltage = 0.0;

        protected:
            static void _bind_methods();

            void run(const RID &p_event, const RID &p_activator) override;

        public:
            void set_power_sources(const TypedArray<RID> &p_power_sources);
            TypedArray<RID> get_power_sources() const;
            void set_voltage(double p_voltage);
            double get_voltage() const;
    };
} // namespace godot
