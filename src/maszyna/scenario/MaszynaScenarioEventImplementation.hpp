#pragma once
#include "scenario/ScenarioEventImplementation.hpp"

namespace godot {
    /// The original's event system as the implementation of a ScenarioEventServer event system:
    /// what MaszynaScenarioEventServer keeps of the system's events, memories and launchers
    class MaszynaScenarioEventImplementation : public ScenarioEventImplementation {
            GDCLASS(MaszynaScenarioEventImplementation, ScenarioEventImplementation)

        protected:
            static void _bind_methods() {}

            void system_detached(const RID &p_system) override;
            bool event_can_queue(const RID &p_system, const RID &p_event, const RID &p_activator) const override;
            double event_get_run_time(
                    const RID &p_system, const RID &p_event, const RID &p_activator, double p_now,
                    double p_run_time) const override;
            void event_ran(const RID &p_system, const RID &p_event, const RID &p_activator) override;
            void scheduled(const RID &p_system, const RID &p_owner, int64_t p_ticket) override;
    };
} // namespace godot
