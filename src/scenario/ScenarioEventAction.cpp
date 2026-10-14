#include "ScenarioEventAction.hpp"

namespace godot {
    void ScenarioEventAction::_bind_methods() {
        GDVIRTUAL_BIND(_run, "event", "activator");
        GDVIRTUAL_BIND(_run_else, "event", "activator");
    }

    void ScenarioEventAction::run(const RID &p_event, const RID &p_activator) {
        GDVIRTUAL_CALL(_run, p_event, p_activator);
    }

    void ScenarioEventAction::run_else(const RID &p_event, const RID &p_activator) {
        GDVIRTUAL_CALL(_run_else, p_event, p_activator);
    }
} // namespace godot
