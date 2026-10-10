#include "SignalHeadKind.hpp"
#include "SignalHeadNode.hpp"
#include <godot_cpp/core/object.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>

namespace godot {
    const char *SignalHeadNode::light_state_changed_signal = "light_state_changed";
    const char *SignalHeadNode::aspect_changed_signal = "aspect_changed";
    const char *SignalHeadNode::signal_head_registered_signal = "signal_head_registered";
    const char *SignalHeadNode::signal_head_unregistered_signal = "signal_head_unregistered";

    /// E3DModelInstance is a GDScript class, so its signal and method are reached by name
    static constexpr const char *MODEL_INSTANCE_CREATED_SIGNAL = "e3d_instance_created";
    static constexpr const char *MODEL_GET_INSTANCE_METHOD = "get_e3d_instance";
    static constexpr const char *ASPECT_PROPERTY = "aspect";
    static constexpr const char *LIGHT_PROPERTY_PREFIX = "light_";
    static constexpr const char *LIGHT_STATE_PROPERTY_SUFFIX = "_state";

    void SignalHeadNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_signal_head_name", "name"), &SignalHeadNode::set_signal_head_name);
        ClassDB::bind_method(D_METHOD("get_signal_head_name"), &SignalHeadNode::get_signal_head_name);
        ClassDB::bind_method(D_METHOD("set_model", "model"), &SignalHeadNode::set_model);
        ClassDB::bind_method(D_METHOD("get_model"), &SignalHeadNode::get_model);
        ClassDB::bind_method(D_METHOD("set_kind", "kind"), &SignalHeadNode::set_kind);
        ClassDB::bind_method(D_METHOD("get_kind"), &SignalHeadNode::get_kind);
        ClassDB::bind_method(D_METHOD("set_aspect", "aspect"), &SignalHeadNode::set_aspect);
        ClassDB::bind_method(D_METHOD("get_aspect"), &SignalHeadNode::get_aspect);
        ClassDB::bind_method(D_METHOD("get_aspects"), &SignalHeadNode::get_aspects);
        ClassDB::bind_method(D_METHOD("get_light_count"), &SignalHeadNode::get_light_count);
        ClassDB::bind_method(D_METHOD("get_signal_head"), &SignalHeadNode::get_signal_head);
        ClassDB::bind_method(D_METHOD("enable_light", "light"), &SignalHeadNode::enable_light);
        ClassDB::bind_method(D_METHOD("disable_light", "light"), &SignalHeadNode::disable_light);
        ClassDB::bind_method(
                D_METHOD("blink_light", "light", "on_time", "off_time", "phase"), &SignalHeadNode::blink_light);
        ClassDB::bind_method(D_METHOD("get_light_state", "light"), &SignalHeadNode::get_light_state);

        ADD_PROPERTY(
                PropertyInfo(Variant::STRING_NAME, "signal_head_name"), "set_signal_head_name", "get_signal_head_name");
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "model", PROPERTY_HINT_NODE_TYPE, "E3DModelInstance"), "set_model",
                "get_model");
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "kind", PROPERTY_HINT_RESOURCE_TYPE, "SignalHeadKind"), "set_kind",
                "get_kind");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "light_count", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY),
                "", "get_light_count");

        ADD_SIGNAL(MethodInfo(
                light_state_changed_signal, PropertyInfo(Variant::INT, "light"),
                PropertyInfo(Variant::INT, "state", PROPERTY_HINT_ENUM, SignallingServer::light_state_hint())));
        ADD_SIGNAL(MethodInfo(aspect_changed_signal, PropertyInfo(Variant::STRING_NAME, "aspect")));
        ADD_SIGNAL(MethodInfo(signal_head_registered_signal));
        ADD_SIGNAL(MethodInfo(signal_head_unregistered_signal));
    }

    /// Runs in the editor too: the model is a @tool, so the signal head - its light count, its
    /// aspects, what an aspect lights - is there to preview while the scene is edited
    void SignalHeadNode::_notification(const int p_what) {
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE: {
                server->connect(
                        SignallingServer::signal_head_registered_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_registered));
                server->connect(
                        SignallingServer::signal_head_unregistered_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_unregistered));
                server->connect(
                        SignallingServer::signal_head_config_changed_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_config_changed));
                server->connect(
                        SignallingServer::signal_head_light_state_changed_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_light_state_changed));
                server->connect(
                        SignallingServer::signal_head_aspect_changed_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_aspect_changed));
                Node *model = get_model();
                if (model == nullptr) {
                    signal_head = server->signal_head_get_rid_by_name(signal_head_name);
                    break;
                }
                model->connect(
                        MODEL_INSTANCE_CREATED_SIGNAL, callable_mp(this, &SignalHeadNode::_on_model_instance_created));
                // a model that re-entered the tree before this node has its instance already
                if (const RID instance = model->call(MODEL_GET_INSTANCE_METHOD); instance.is_valid()) {
                    _on_model_instance_created(instance);
                }
            } break;
            case NOTIFICATION_EXIT_TREE: {
                server->disconnect(
                        SignallingServer::signal_head_registered_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_registered));
                server->disconnect(
                        SignallingServer::signal_head_unregistered_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_unregistered));
                server->disconnect(
                        SignallingServer::signal_head_config_changed_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_config_changed));
                server->disconnect(
                        SignallingServer::signal_head_light_state_changed_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_light_state_changed));
                server->disconnect(
                        SignallingServer::signal_head_aspect_changed_signal,
                        callable_mp(this, &SignalHeadNode::_on_signal_head_aspect_changed));
                if (Node *model = get_model(); model != nullptr) {
                    model->disconnect(
                            MODEL_INSTANCE_CREATED_SIGNAL,
                            callable_mp(this, &SignalHeadNode::_on_model_instance_created));
                }
                // a signal head registered from the model goes with the model's instance
                signal_head = RID();
            } break;
            default:
                break;
        }
    }

    /// The model built a new instance (first load or a reload): its lights become the signal head,
    /// of the kind and showing the aspect the scene sets
    void SignalHeadNode::_on_model_instance_created(const RID &p_instance) {
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        signal_head = server->signal_head_create(p_instance);
        server->signal_head_set_kind(signal_head, kind);
        if (kind.is_valid() && kind->has_aspect(aspect)) {
            server->signal_head_set_aspect(signal_head, aspect);
        }
        server->signal_head_set_name(signal_head, signal_head_name);
        notify_property_list_changed();
    }

    void SignalHeadNode::_on_signal_head_registered(const RID &p_signal_head, const StringName &p_name) {
        if (!(p_name == signal_head_name)) {
            return;
        }
        signal_head = p_signal_head;
        notify_property_list_changed();
        emit_signal(signal_head_registered_signal);
    }

    void SignalHeadNode::_on_signal_head_unregistered(const RID &p_signal_head, const StringName &p_name) {
        if (!(p_signal_head == signal_head)) {
            return;
        }
        signal_head = RID();
        notify_property_list_changed();
        emit_signal(signal_head_unregistered_signal);
    }

    /// The model's lights became known, or the kind changed - the property list follows both
    void SignalHeadNode::_on_signal_head_config_changed(const RID &p_signal_head) {
        if (p_signal_head == signal_head) {
            notify_property_list_changed();
        }
    }

    void SignalHeadNode::_on_signal_head_light_state_changed(
            const RID &p_signal_head, const int p_light, const int p_state) {
        if (p_signal_head == signal_head) {
            emit_signal(light_state_changed_signal, p_light, p_state);
        }
    }

    void SignalHeadNode::_on_signal_head_aspect_changed(const RID &p_signal_head, const StringName &p_aspect) {
        if (p_signal_head == signal_head) {
            emit_signal(aspect_changed_signal, p_aspect);
        }
    }

    int SignalHeadNode::_parse_light_state_property(const StringName &p_name) {
        const String name = p_name;
        if (!name.begins_with(LIGHT_PROPERTY_PREFIX) || !name.ends_with(LIGHT_STATE_PROPERTY_SUFFIX)) {
            return -1;
        }
        const String index = name.trim_prefix(LIGHT_PROPERTY_PREFIX).trim_suffix(LIGHT_STATE_PROPERTY_SUFFIX);
        return index.is_valid_int() ? static_cast<int>(index.to_int()) : -1;
    }

    /// `aspect`, and light_<n>_state - a proxy of the light on the server, blinking with the
    /// default times
    bool SignalHeadNode::_set(const StringName &p_name, const Variant &p_value) {
        if (p_name == StringName(ASPECT_PROPERTY)) {
            set_aspect(p_value);
            return true;
        }
        const int light = _parse_light_state_property(p_name);
        if (light < 0) {
            return false;
        }
        switch (static_cast<int>(p_value)) {
            case SignallingServer::LIGHT_STATE_ON:
                enable_light(light);
                break;
            case SignallingServer::LIGHT_STATE_BLINKING:
                blink_light(light, SignalHeadKind::DEFAULT_BLINK_TIME, SignalHeadKind::DEFAULT_BLINK_TIME, 0.0);
                break;
            default:
                disable_light(light);
                break;
        }
        return true;
    }

    bool SignalHeadNode::_get(const StringName &p_name, Variant &p_value) const {
        if (p_name == StringName(ASPECT_PROPERTY)) {
            p_value = get_aspect();
            return true;
        }
        const int light = _parse_light_state_property(p_name);
        if (light < 0) {
            return false;
        }
        p_value = get_light_state(light);
        return true;
    }

    /// The aspect as an enum of the kind's aspects (kept in the scene), and one state per light of
    /// the model while the signal head exists (not kept - it is the server's)
    void SignalHeadNode::_get_property_list(List<PropertyInfo> *p_list) const {
        p_list->push_back(PropertyInfo(
                Variant::STRING_NAME, ASPECT_PROPERTY, PROPERTY_HINT_ENUM, String(",").join(get_aspects())));
        const int count = get_light_count();
        for (int light = 0; light < count; light++) {
            p_list->push_back(PropertyInfo(
                    Variant::INT,
                    String(LIGHT_PROPERTY_PREFIX) + String::num_int64(light) + String(LIGHT_STATE_PROPERTY_SUFFIX),
                    PROPERTY_HINT_ENUM, SignallingServer::light_state_hint(), PROPERTY_USAGE_EDITOR));
        }
    }

    /// Renames the signal head this node registered, or attaches to the one registered under the name
    void SignalHeadNode::set_signal_head_name(const StringName &p_name) {
        signal_head_name = p_name;
        if (!is_inside_tree()) {
            return;
        }
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        if (!model_id.is_valid()) {
            signal_head = server->signal_head_get_rid_by_name(p_name);
            notify_property_list_changed();
        } else if (signal_head.is_valid()) {
            server->signal_head_set_name(signal_head, p_name);
        }
    }

    StringName SignalHeadNode::get_signal_head_name() const {
        return signal_head_name;
    }

    /// Taken when the node enters the tree
    void SignalHeadNode::set_model(Node *p_model) {
        model_id = p_model != nullptr ? ObjectID(p_model->get_instance_id()) : ObjectID();
    }

    Node *SignalHeadNode::get_model() const {
        return Object::cast_to<Node>(ObjectDB::get_instance(model_id));
    }

    void SignalHeadNode::set_kind(const Ref<SignalHeadKind> &p_kind) {
        kind = p_kind;
        if (signal_head.is_valid()) {
            SignallingServer *server = SignallingServer::get_instance();
            ERR_FAIL_NULL(server);
            server->signal_head_set_kind(signal_head, kind);
        }
        notify_property_list_changed();
    }

    Ref<SignalHeadKind> SignalHeadNode::get_kind() const {
        const SignallingServer *server = SignallingServer::get_instance();
        return signal_head.is_valid() && server != nullptr ? server->signal_head_get_kind(signal_head) : kind;
    }

    void SignalHeadNode::set_aspect(const StringName &p_aspect) {
        aspect = p_aspect;
        if (!signal_head.is_valid() || p_aspect.is_empty()) {
            return;
        }
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->signal_head_set_aspect(signal_head, aspect);
    }

    StringName SignalHeadNode::get_aspect() const {
        const SignallingServer *server = SignallingServer::get_instance();
        return signal_head.is_valid() && server != nullptr ? server->signal_head_get_aspect(signal_head) : aspect;
    }

    /// The aspects the signal head's kind can show
    PackedStringArray SignalHeadNode::get_aspects() const {
        const Ref<SignalHeadKind> current = get_kind();
        return current.is_valid() ? current->get_aspect_names() : PackedStringArray();
    }

    /// The lights of the signal head's model; 0 until the signal head exists and its model is loaded
    int SignalHeadNode::get_light_count() const {
        const SignallingServer *server = SignallingServer::get_instance();
        return signal_head.is_valid() && server != nullptr ? server->signal_head_get_light_count(signal_head) : 0;
    }

    RID SignalHeadNode::get_signal_head() const {
        return signal_head;
    }

    void SignalHeadNode::enable_light(const int p_light) {
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->signal_head_light_enable(signal_head, p_light);
    }

    void SignalHeadNode::disable_light(const int p_light) {
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->signal_head_light_disable(signal_head, p_light);
    }

    void
    SignalHeadNode::blink_light(const int p_light, const float p_on_time, const float p_off_time, const float p_phase) {
        SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL(server);
        server->signal_head_light_blink(signal_head, p_light, p_on_time, p_off_time, p_phase);
    }

    SignallingServer::LightState SignalHeadNode::get_light_state(const int p_light) const {
        const SignallingServer *server = SignallingServer::get_instance();
        ERR_FAIL_NULL_V(server, SignallingServer::LIGHT_STATE_OFF);
        return server->signal_head_get_light_state(signal_head, p_light);
    }
} // namespace godot
