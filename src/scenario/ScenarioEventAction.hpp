#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /// What a ScenarioEventServer event does when it runs. The server knows only the queue, the
    /// time and the condition; the effect of an event (setting a memory, lighting a signal head,
    /// throwing a switch, queueing other events) is its action. Implement it in C++ by overriding
    /// the virtual methods, or in GDScript by overriding their script counterparts.
    ///
    /// Every callback carries the event: a Resource is shared by default, so one action may serve
    /// several events. The activator is the RailVehicleServer vehicle that queued the event, empty
    /// when nothing did. An action changes state only through the servers' APIs.
    class ScenarioEventAction : public Resource {
            GDCLASS(ScenarioEventAction, Resource)
            friend class ScenarioEventServer;

        protected:
            static void _bind_methods();

            GDVIRTUAL2(_run, RID, RID)
            GDVIRTUAL2(_run_else, RID, RID)

            /// Called by ScenarioEventServer. A C++ action overrides these; the default forwards
            /// to the script.
            /// The event's condition passed, or it has none
            virtual void run(const RID &p_event, const RID &p_activator);
            /// The event's condition failed (the original's `else`, Event.cpp:1266-1296)
            virtual void run_else(const RID &p_event, const RID &p_activator);
    };
} // namespace godot
