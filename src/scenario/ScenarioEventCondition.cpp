#include "ScenarioEventCondition.hpp"

namespace godot {
    void ScenarioEventCondition::_bind_methods() {
        GDVIRTUAL_BIND(_test, "event", "activator");
    }

    bool ScenarioEventCondition::test(const RID &p_event, const RID &p_activator) const {
        bool passed = true;
        GDVIRTUAL_CALL(_test, p_event, p_activator, passed);
        return passed;
    }
} // namespace godot
