#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    /// The behaviour of a SignallingServer system: its state machine and the vocabulary of its
    /// aspects. A system is always the same server object; what differs between signalling
    /// systems (the original's scenery events, an automatic block, an interlocking...) is its
    /// implementation. This class is only the interface - implement it in C++ by overriding the virtual
    /// methods, or in GDScript by overriding their script counterparts.
    ///
    /// Every callback carries the system: a Resource is shared by default, so one implementation may
    /// serve several systems and keeps whatever state it needs per system RID. An implementation drives
    /// the lights only through the SignallingServer API.
    class SignallingImplementation : public Resource {
            GDCLASS(SignallingImplementation, Resource)
            friend class SignallingServer;

        protected:
            static void _bind_methods();

            GDVIRTUAL1(_system_attached, RID)
            GDVIRTUAL1(_system_detached, RID)
            GDVIRTUAL2(_signal_head_added, RID, RID)
            GDVIRTUAL2(_signal_head_removed, RID, RID)
            GDVIRTUAL2(_source_added, RID, RID)
            GDVIRTUAL2(_source_removed, RID, RID)
            GDVIRTUAL3(_signal_head_aspect_changed, RID, RID, StringName)
            GDVIRTUAL3(_handle_event, RID, StringName, Dictionary)
            GDVIRTUAL4(_handle_source_event, RID, RID, StringName, Dictionary)

            /// Called by SignallingServer. A C++ implementation overrides these; the default forwards to
            /// the script.
            virtual void system_attached(const RID &p_system);
            virtual void system_detached(const RID &p_system);
            virtual void signal_head_added(const RID &p_system, const RID &p_signal_head);
            virtual void signal_head_removed(const RID &p_system, const RID &p_signal_head);
            virtual void source_added(const RID &p_system, const RID &p_source);
            virtual void source_removed(const RID &p_system, const RID &p_source);
            /// One of the system's signal heads now shows the aspect (SignallingServer.signal_head_set_aspect())
            virtual void
            signal_head_aspect_changed(const RID &p_system, const RID &p_signal_head, const StringName &p_aspect);
            /// A command to the system as a whole (a script, a dispatcher, a scenery event)
            virtual void handle_event(const RID &p_system, const StringName &p_event, const Dictionary &p_arguments);
            /// A report of one of the system's registered sources
            virtual void handle_source_event(
                    const RID &p_system, const RID &p_source, const StringName &p_event, const Dictionary &p_arguments);
    };
} // namespace godot
