#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /// How a scenario script reaches the cabs. The cab layer (CabinSystem) is written in GDScript,
    /// so the scripting layer cannot name it; an implementation implemented next to it forwards the
    /// scripts' manipulations and reports back what changed in a cab (control_changed).
    class ScenarioScriptCabinImplementation : public Resource {
            GDCLASS(ScenarioScriptCabinImplementation, Resource)

        protected:
            static void _bind_methods();

            GDVIRTUAL4R(Variant, _act, RID, StringName, StringName, Variant)
            GDVIRTUAL2RC(Variant, _get_control, RID, StringName)
            GDVIRTUAL1RC(Array, _get_controls, RID)

        public:
            /// A control of a cab changed (cabin: RID, control_id: StringName, value)
            static const char *control_changed_signal;

            /// Manipulates a control of the VehicleServer cabin; returns what the control answered
            Variant
            act(const RID &p_cabin, const StringName &p_control_id, const StringName &p_action, const Variant &p_value);
            Variant get_control(const RID &p_cabin, const StringName &p_control_id) const;
            Array get_controls(const RID &p_cabin) const;
    };
} // namespace godot
