#include "SignallingSystemNode.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
    void SignallingSystemNode::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_implementation", "implementation"), &SignallingSystemNode::set_implementation);
        ClassDB::bind_method(D_METHOD("get_implementation"), &SignallingSystemNode::get_implementation);
        ClassDB::bind_method(D_METHOD("set_signal_head_names", "names"), &SignallingSystemNode::set_signal_head_names);
        ClassDB::bind_method(D_METHOD("get_signal_head_names"), &SignallingSystemNode::get_signal_head_names);
        ClassDB::bind_method(D_METHOD("get_system"), &SignallingSystemNode::get_system);
        ClassDB::bind_method(
                D_METHOD("send_event", "event", "arguments"), &SignallingSystemNode::send_event, DEFVAL(Dictionary()));

        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "implementation", PROPERTY_HINT_RESOURCE_TYPE, "SignallingImplementation"),
                "set_implementation", "get_implementation");
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "signal_head_names"), "set_signal_head_names",
                "get_signal_head_names");
    }

    void SignallingSystemNode::_notification(const int p_what) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE: {
                system = server->system_create();
                server->system_attach_implementation(system, implementation);
                server->connect(
                        SignallingServer::signal_head_registered_signal,
                        callable_mp(this, &SignallingSystemNode::_on_signal_head_registered));
                for (const String &name: signal_head_names) {
                    if (const RID signal_head = server->signal_head_get_rid_by_name(name); signal_head.is_valid()) {
                        server->system_add_signal_head(system, signal_head);
                    }
                }
            } break;
            case NOTIFICATION_EXIT_TREE: {
                server->disconnect(
                        SignallingServer::signal_head_registered_signal,
                        callable_mp(this, &SignallingSystemNode::_on_signal_head_registered));
                server->system_free(system);
                system = RID();
            } break;
            default:
                break;
        }
    }

    /// A listed signal head that shows up after the system was created - a model built later, or
    /// the same model after a reload
    void SignallingSystemNode::_on_signal_head_registered(const RID &p_signal_head, const StringName &p_name) {
        if (!signal_head_names.has(p_name)) {
            return;
        }
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        if (server->signal_head_get_system(p_signal_head) == system) {
            return; // renamed within the list
        }
        server->system_add_signal_head(system, p_signal_head);
    }

    void SignallingSystemNode::set_implementation(const Ref<SignallingImplementation> &p_implementation) {
        implementation = p_implementation;
        if (!system.is_valid()) {
            return;
        }
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->system_attach_implementation(system, implementation);
    }

    Ref<SignallingImplementation> SignallingSystemNode::get_implementation() const {
        return implementation;
    }

    /// Taken when the node enters the tree, and for signal heads registered after that
    void SignallingSystemNode::set_signal_head_names(const PackedStringArray &p_names) {
        signal_head_names = p_names;
    }

    PackedStringArray SignallingSystemNode::get_signal_head_names() const {
        return signal_head_names;
    }

    RID SignallingSystemNode::get_system() const {
        return system;
    }

    void SignallingSystemNode::send_event(const StringName &p_event, const Dictionary &p_arguments) {
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->system_send_event(system, p_event, p_arguments);
    }
} // namespace godot
