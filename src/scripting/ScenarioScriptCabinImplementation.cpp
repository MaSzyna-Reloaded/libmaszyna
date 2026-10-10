#include "ScenarioScriptCabinImplementation.hpp"

namespace godot {
    const char *ScenarioScriptCabinImplementation::control_changed_signal = "control_changed";

    void ScenarioScriptCabinImplementation::_bind_methods() {
        GDVIRTUAL_BIND(_act, "cabin", "control_id", "action", "value");
        GDVIRTUAL_BIND(_get_control, "cabin", "control_id");
        GDVIRTUAL_BIND(_get_controls, "cabin");

        ADD_SIGNAL(MethodInfo(
                control_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::STRING_NAME, "control_id"), PropertyInfo(Variant::NIL, "value")));
    }

    Variant ScenarioScriptCabinImplementation::act(
            const RID &p_cabin, const StringName &p_control_id, const StringName &p_action, const Variant &p_value) {
        Variant result;
        GDVIRTUAL_CALL(_act, p_cabin, p_control_id, p_action, p_value, result);
        return result;
    }

    Variant ScenarioScriptCabinImplementation::get_control(const RID &p_cabin, const StringName &p_control_id) const {
        Variant result;
        GDVIRTUAL_CALL(_get_control, p_cabin, p_control_id, result);
        return result;
    }

    Array ScenarioScriptCabinImplementation::get_controls(const RID &p_cabin) const {
        Array result;
        GDVIRTUAL_CALL(_get_controls, p_cabin, result);
        return result;
    }
} // namespace godot
