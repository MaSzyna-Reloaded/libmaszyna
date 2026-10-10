#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /// Decides whether a ScenarioEventServer event runs its action's run() or run_else(), and
    /// whether a launcher fires. Implement it in C++ by overriding test(), or in GDScript by
    /// overriding _test(). A condition that implements neither passes.
    class ScenarioEventCondition : public Resource {
            GDCLASS(ScenarioEventCondition, Resource)
            friend class ScenarioEventServer;

        protected:
            static void _bind_methods();

            GDVIRTUAL2RC(bool, _test, RID, RID)

            /// Called by ScenarioEventServer with the event about to run and its activator
            virtual bool test(const RID &p_event, const RID &p_activator) const;
    };
} // namespace godot
