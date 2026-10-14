#pragma once
#include "SignallingServer.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    /// A SignallingServer system in a scene: creates it on entering the tree with the implementation
    /// set here and groups the signal heads listed by name (also those registered later). A scenery
    /// creates its systems on the server directly; this node is for hand-built scenes.
    class SignallingSystemNode : public Node {
            GDCLASS(SignallingSystemNode, Node)

        private:
            Ref<SignallingImplementation> implementation;
            PackedStringArray signal_head_names;
            RID system;

            void _on_signal_head_registered(const RID &p_signal_head, const StringName &p_name);

        protected:
            static void _bind_methods();
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);

        public:
            void set_implementation(const Ref<SignallingImplementation> &p_implementation);
            Ref<SignallingImplementation> get_implementation() const;
            void set_signal_head_names(const PackedStringArray &p_names);
            PackedStringArray get_signal_head_names() const;

            RID get_system() const;
            void send_event(const StringName &p_event, const Dictionary &p_arguments);
    };
} // namespace godot
