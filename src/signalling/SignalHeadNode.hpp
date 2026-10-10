#pragma once
#include "SignallingServer.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/object_id.hpp>

namespace godot {
    /// One signal head of SignallingServer, driven freely - no state machine. With a model set, the
    /// node registers that model's lights as the signal head itself; without one, it attaches to
    /// the signal head already registered under its name (a scenery's).
    ///
    /// The properties are proxies of the signal head on the server: they read and change it there,
    /// so a change made by a system shows here as well. Only the kind and the aspect a registered
    /// signal head starts with are kept on the node, for the scene to set. A signal head that belongs
    /// to a system is still changed from here, and the system overwrites it with its next event.
    class SignalHeadNode : public Node {
            GDCLASS(SignalHeadNode, Node)

        public:
            static const char *light_state_changed_signal;
            static const char *aspect_changed_signal;
            static const char *signal_head_registered_signal;
            static const char *signal_head_unregistered_signal;

        private:
            StringName signal_head_name;
            ObjectID model_id; // the model may be freed before this node
            RID signal_head;
            Ref<SignalHeadKind> kind;
            StringName aspect;

            void _on_model_instance_created(const RID &p_instance);
            void _on_signal_head_registered(const RID &p_signal_head, const StringName &p_name);
            void _on_signal_head_unregistered(const RID &p_signal_head, const StringName &p_name);
            void _on_signal_head_config_changed(const RID &p_signal_head);
            void _on_signal_head_light_state_changed(const RID &p_signal_head, int p_light, int p_state);
            void _on_signal_head_aspect_changed(const RID &p_signal_head, const StringName &p_aspect);
            /// "light_<n>_state" -> n; -1 for any other name
            static int _parse_light_state_property(const StringName &p_name);

        protected:
            static void _bind_methods();
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            bool _set(const StringName &p_name, const Variant &p_value);
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            bool _get(const StringName &p_name, Variant &p_value) const;
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _get_property_list(List<PropertyInfo> *p_list) const;

        public:
            void set_signal_head_name(const StringName &p_name);
            StringName get_signal_head_name() const;
            void set_model(Node *p_model);
            Node *get_model() const;
            void set_kind(const Ref<SignalHeadKind> &p_kind);
            Ref<SignalHeadKind> get_kind() const;
            void set_aspect(const StringName &p_aspect);
            StringName get_aspect() const;
            PackedStringArray get_aspects() const;
            int get_light_count() const;

            RID get_signal_head() const;
            void enable_light(int p_light);
            void disable_light(int p_light);
            void blink_light(int p_light, float p_on_time, float p_off_time, float p_phase);
            SignallingServer::LightState get_light_state(int p_light) const;
    };
} // namespace godot
