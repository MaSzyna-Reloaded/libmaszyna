#include "../macros.hpp"
#include "SignallingServer.hpp"
#include "legacy/e3d/E3DRenderingServer.hpp"
#include "utils/Names.hpp"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *SignallingServer::signal_head_registered_signal = "signal_head_registered";
    const char *SignallingServer::signal_head_unregistered_signal = "signal_head_unregistered";
    const char *SignallingServer::signal_head_light_state_changed_signal = "signal_head_light_state_changed";
    const char *SignallingServer::signal_head_aspect_changed_signal = "signal_head_aspect_changed";
    const char *SignallingServer::signal_head_config_changed_signal = "signal_head_config_changed";
    const char *SignallingServer::system_signal_head_added_signal = "system_signal_head_added";
    const char *SignallingServer::system_signal_head_removed_signal = "system_signal_head_removed";
    const char *SignallingServer::system_source_added_signal = "system_source_added";
    const char *SignallingServer::system_source_removed_signal = "system_source_removed";

    void SignallingServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("signal_head_create", "instance"), &SignallingServer::signal_head_create);
        ClassDB::bind_method(D_METHOD("signal_head_free", "signal_head"), &SignallingServer::signal_head_free);
        ClassDB::bind_method(
                D_METHOD("signal_head_set_name", "signal_head", "name"), &SignallingServer::signal_head_set_name);
        ClassDB::bind_method(D_METHOD("signal_head_get_name", "signal_head"), &SignallingServer::signal_head_get_name);
        ClassDB::bind_method(D_METHOD("signal_head_get_rids"), &SignallingServer::signal_head_get_rids);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_instance", "signal_head"), &SignallingServer::signal_head_get_instance);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_rid_by_name", "name"), &SignallingServer::signal_head_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_system", "signal_head"), &SignallingServer::signal_head_get_system);
        ClassDB::bind_method(
                D_METHOD("signal_head_light_enable", "signal_head", "light"),
                &SignallingServer::signal_head_light_enable);
        ClassDB::bind_method(
                D_METHOD("signal_head_light_disable", "signal_head", "light"),
                &SignallingServer::signal_head_light_disable);
        ClassDB::bind_method(
                D_METHOD("signal_head_light_blink", "signal_head", "light", "on_time", "off_time", "phase"),
                &SignallingServer::signal_head_light_blink);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_light_state", "signal_head", "light"),
                &SignallingServer::signal_head_get_light_state);

        ClassDB::bind_method(
                D_METHOD("signal_head_get_light_count", "signal_head"), &SignallingServer::signal_head_get_light_count);
        ClassDB::bind_method(
                D_METHOD("signal_head_set_kind", "signal_head", "kind"), &SignallingServer::signal_head_set_kind);
        ClassDB::bind_method(D_METHOD("signal_head_get_kind", "signal_head"), &SignallingServer::signal_head_get_kind);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_aspects", "signal_head"), &SignallingServer::signal_head_get_aspects);
        ClassDB::bind_method(
                D_METHOD("signal_head_set_aspect", "signal_head", "aspect"), &SignallingServer::signal_head_set_aspect);
        ClassDB::bind_method(
                D_METHOD("signal_head_get_aspect", "signal_head"), &SignallingServer::signal_head_get_aspect);

        ClassDB::bind_method(D_METHOD("system_create"), &SignallingServer::system_create);
        ClassDB::bind_method(D_METHOD("system_free", "system"), &SignallingServer::system_free);
        ClassDB::bind_method(
                D_METHOD("system_attach_implementation", "system", "implementation"),
                &SignallingServer::system_attach_implementation);
        ClassDB::bind_method(
                D_METHOD("system_get_implementation", "system"), &SignallingServer::system_get_implementation);
        ClassDB::bind_method(D_METHOD("system_set_name", "system", "name"), &SignallingServer::system_set_name);
        ClassDB::bind_method(D_METHOD("system_get_name", "system"), &SignallingServer::system_get_name);
        ClassDB::bind_method(D_METHOD("system_get_rid_by_name", "name"), &SignallingServer::system_get_rid_by_name);
        ClassDB::bind_method(
                D_METHOD("system_add_signal_head", "system", "signal_head"), &SignallingServer::system_add_signal_head);
        ClassDB::bind_method(
                D_METHOD("system_remove_signal_head", "system", "signal_head"),
                &SignallingServer::system_remove_signal_head);
        ClassDB::bind_method(D_METHOD("system_get_signal_heads", "system"), &SignallingServer::system_get_signal_heads);
        ClassDB::bind_method(D_METHOD("system_add_source", "system", "source"), &SignallingServer::system_add_source);
        ClassDB::bind_method(
                D_METHOD("system_remove_source", "system", "source"), &SignallingServer::system_remove_source);
        ClassDB::bind_method(D_METHOD("system_get_sources", "system"), &SignallingServer::system_get_sources);
        ClassDB::bind_method(
                D_METHOD("system_send_event", "system", "event", "arguments"), &SignallingServer::system_send_event,
                DEFVAL(Dictionary()));

        ClassDB::bind_method(D_METHOD("source_create"), &SignallingServer::source_create);
        ClassDB::bind_method(D_METHOD("source_free", "source"), &SignallingServer::source_free);
        ClassDB::bind_method(D_METHOD("source_set_name", "source", "name"), &SignallingServer::source_set_name);
        ClassDB::bind_method(D_METHOD("source_get_name", "source"), &SignallingServer::source_get_name);
        ClassDB::bind_method(D_METHOD("source_get_rid_by_name", "name"), &SignallingServer::source_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("source_get_system", "source"), &SignallingServer::source_get_system);
        ClassDB::bind_method(
                D_METHOD("source_send_event", "source", "event", "arguments"), &SignallingServer::source_send_event,
                DEFVAL(Dictionary()));

        BIND_ENUM_CONSTANT(LIGHT_STATE_OFF);
        BIND_ENUM_CONSTANT(LIGHT_STATE_ON);
        BIND_ENUM_CONSTANT(LIGHT_STATE_BLINKING);
        BIND_CONSTANT(MAX_LIGHTS);

        ADD_SIGNAL(MethodInfo(
                signal_head_registered_signal, PropertyInfo(Variant::RID, "signal_head"),
                PropertyInfo(Variant::STRING_NAME, "name")));
        ADD_SIGNAL(MethodInfo(
                signal_head_unregistered_signal, PropertyInfo(Variant::RID, "signal_head"),
                PropertyInfo(Variant::STRING_NAME, "name")));
        ADD_SIGNAL(MethodInfo(
                signal_head_light_state_changed_signal, PropertyInfo(Variant::RID, "signal_head"),
                PropertyInfo(Variant::INT, "light"),
                PropertyInfo(Variant::INT, "state", PROPERTY_HINT_ENUM, light_state_hint())));
        ADD_SIGNAL(MethodInfo(signal_head_config_changed_signal, PropertyInfo(Variant::RID, "signal_head")));
        ADD_SIGNAL(MethodInfo(
                signal_head_aspect_changed_signal, PropertyInfo(Variant::RID, "signal_head"),
                PropertyInfo(Variant::STRING_NAME, "aspect")));
        ADD_SIGNAL(MethodInfo(
                system_signal_head_added_signal, PropertyInfo(Variant::RID, "system"),
                PropertyInfo(Variant::RID, "signal_head")));
        ADD_SIGNAL(MethodInfo(
                system_signal_head_removed_signal, PropertyInfo(Variant::RID, "system"),
                PropertyInfo(Variant::RID, "signal_head")));
        ADD_SIGNAL(MethodInfo(
                system_source_added_signal, PropertyInfo(Variant::RID, "system"),
                PropertyInfo(Variant::RID, "source")));
        ADD_SIGNAL(MethodInfo(
                system_source_removed_signal, PropertyInfo(Variant::RID, "system"),
                PropertyInfo(Variant::RID, "source")));
    }

    String SignallingServer::light_state_hint() {
        return enum_hint({{"Off", LIGHT_STATE_OFF}, {"On", LIGHT_STATE_ON}, {"Blinking", LIGHT_STATE_BLINKING}});
    }

    /// A signal head is the lights of an instance, so it goes when the instance does
    SignallingServer::SignallingServer() {
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        rendering->connect(
                E3DRenderingServer::instance_freed_signal, callable_mp(this, &SignallingServer::_on_instance_freed));
        rendering->connect(
                E3DRenderingServer::instance_built_signal, callable_mp(this, &SignallingServer::_on_instance_built));
    }

    /// The model is known once the instance is built: its lights are the signal head's
    void SignallingServer::_on_instance_built(const RID &p_instance) {
        const RID *signal_head = signal_heads_by_instance.getptr(p_instance);
        if (signal_head == nullptr) {
            return;
        }
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        SignalHeadData *data = signal_heads.getptr(*signal_head);
        ERR_FAIL_NULL(data);
        const int light_count = MIN(rendering->instance_get_light_count(p_instance), MAX_LIGHTS);
        if (data->light_count == light_count) {
            return;
        }
        data->light_count = light_count;
        emit_signal(signal_head_config_changed_signal, *signal_head);
    }

    void SignallingServer::_on_instance_freed(const RID &p_instance) {
        const RID *signal_head = signal_heads_by_instance.getptr(p_instance);
        if (signal_head == nullptr) {
            return;
        }
        signal_head_free(*signal_head);
    }

    // --- signal head ---

    RID SignallingServer::signal_head_create(const RID &p_instance) {
        ERR_FAIL_COND_V_MSG(signal_heads_by_instance.has(p_instance), RID(), "The instance already has a signal head.");
        RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL_V(rendering, RID());
        SignalHeadData data;
        data.instance = p_instance;
        data.light_count = MIN(rendering->instance_get_light_count(p_instance), MAX_LIGHTS);
        signal_heads.insert(rid, data);
        signal_heads_by_instance.insert(p_instance, rid);
        return rid;
    }

    void SignallingServer::signal_head_free(const RID &p_signal_head) {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        if (data->system.is_valid()) {
            system_remove_signal_head(data->system, p_signal_head);
        }
        const SignalHeadData freed = *signal_heads.getptr(p_signal_head);
        signal_heads.erase(p_signal_head);
        signal_heads_by_instance.erase(freed.instance);
        if (!freed.name.is_empty()) {
            names_rename(signal_heads_by_name, freed.name, StringName(), p_signal_head);
            emit_signal(signal_head_unregistered_signal, p_signal_head, freed.name);
        }
    }

    /// Registers the signal head under the name - what a SignalHeadNode, a script or a scenery event
    /// finds it by. An empty name unregisters it.
    void SignallingServer::signal_head_set_name(const RID &p_signal_head, const StringName &p_name) {
        SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        const StringName previous = data->name;
        if (previous == p_name) {
            return;
        }
        data->name = p_name;
        names_rename(signal_heads_by_name, previous, p_name, p_signal_head);
        if (!previous.is_empty()) {
            emit_signal(signal_head_unregistered_signal, p_signal_head, previous);
        }
        if (!p_name.is_empty()) {
            emit_signal(signal_head_registered_signal, p_signal_head, p_name);
        }
    }

    StringName SignallingServer::signal_head_get_name(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, StringName());
        return data->name;
    }

    RID SignallingServer::signal_head_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = signal_heads_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    TypedArray<RID> SignallingServer::signal_head_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, SignalHeadData> &signal_head: signal_heads) {
            result.push_back(signal_head.key);
        }
        return result;
    }

    RID SignallingServer::signal_head_get_instance(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, RID());
        return data->instance;
    }

    RID SignallingServer::signal_head_get_system(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, RID());
        return data->system;
    }

    void SignallingServer::_set_light_state(
            const RID &p_signal_head, SignalHeadData &p_data, const int p_light, const LightState p_state) {
        const LightState previous = p_data.light_states[p_light];
        p_data.light_states[p_light] = p_state;
        if (!(previous == p_state)) {
            emit_signal(signal_head_light_state_changed_signal, p_signal_head, p_light, p_state);
        }
    }

    void SignallingServer::signal_head_light_enable(const RID &p_signal_head, const int p_light) {
        SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        ERR_FAIL_INDEX(p_light, MAX_LIGHTS);
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        rendering->instance_set_light_mode(data->instance, p_light, E3DRenderingServer::LIGHT_MODE_ON);
        _set_light_state(p_signal_head, *data, p_light, LIGHT_STATE_ON);
    }

    void SignallingServer::signal_head_light_disable(const RID &p_signal_head, const int p_light) {
        SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        ERR_FAIL_INDEX(p_light, MAX_LIGHTS);
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        rendering->instance_set_light_mode(data->instance, p_light, E3DRenderingServer::LIGHT_MODE_OFF);
        _set_light_state(p_signal_head, *data, p_light, LIGHT_STATE_OFF);
    }

    void SignallingServer::signal_head_light_blink(
            const RID &p_signal_head, const int p_light, const float p_on_time, const float p_off_time,
            const float p_phase) {
        SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        ERR_FAIL_INDEX(p_light, MAX_LIGHTS);
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        rendering->instance_set_light_blink(data->instance, p_light, p_on_time, p_off_time, p_phase);
        _set_light_state(p_signal_head, *data, p_light, LIGHT_STATE_BLINKING);
    }

    SignallingServer::LightState
    SignallingServer::signal_head_get_light_state(const RID &p_signal_head, const int p_light) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, LIGHT_STATE_OFF);
        ERR_FAIL_INDEX_V(p_light, MAX_LIGHTS, LIGHT_STATE_OFF);
        return data->light_states[p_light];
    }

    /// How many lights the signal head's model has (TAnimModel::iNumLights); 0 until the model is
    /// loaded. A light past it may still be set, as LightSet() allows (AnimModel.cpp:664-670).
    int SignallingServer::signal_head_get_light_count(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, 0);
        return data->light_count;
    }

    /// What the signal head can show. A new kind leaves the lights as they are and no aspect set.
    void SignallingServer::signal_head_set_kind(const RID &p_signal_head, const Ref<SignalHeadKind> &p_kind) {
        SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        data->kind = p_kind;
        data->aspect = StringName();
        emit_signal(signal_head_config_changed_signal, p_signal_head);
    }

    Ref<SignalHeadKind> SignallingServer::signal_head_get_kind(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, Ref<SignalHeadKind>());
        return data->kind;
    }

    PackedStringArray SignallingServer::signal_head_get_aspects(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, PackedStringArray());
        return data->kind.is_valid() ? data->kind->get_aspect_names() : PackedStringArray();
    }

    /// Lights every light the aspect describes and tells the signal head's system, whose implementation
    /// may react (an automatic block changes the signal head behind)
    void SignallingServer::signal_head_set_aspect(const RID &p_signal_head, const StringName &p_aspect) {
        SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(data);
        ERR_FAIL_COND_MSG(data->kind.is_null(), "The signal head has no kind.");
        ERR_FAIL_COND_MSG(
                !data->kind->has_aspect(p_aspect), "The signal head's kind has no aspect " + String(p_aspect) + ".");
        const Ref<SignalHeadKind> kind = data->kind;
        const Ref<SignalAspect> shown = kind->get_aspect(p_aspect);
        ERR_FAIL_COND(shown.is_null());
        const PackedInt32Array lights = shown->get_lights();
        const PackedFloat32Array on_times = shown->get_on_times();
        const PackedFloat32Array off_times = shown->get_off_times();
        const PackedFloat32Array phases = shown->get_phases();
        const int count = MIN(static_cast<int>(lights.size()), MAX_LIGHTS);
        for (int light = 0; light < count; light++) {
            switch (lights[light]) {
                case SignalAspect::LIGHT_OFF:
                    signal_head_light_disable(p_signal_head, light);
                    break;
                case SignalAspect::LIGHT_ON:
                    signal_head_light_enable(p_signal_head, light);
                    break;
                case SignalAspect::LIGHT_BLINK:
                    signal_head_light_blink(
                            p_signal_head, light, light < on_times.size() ? on_times[light] : kind->get_blink_on_time(),
                            light < off_times.size() ? off_times[light] : kind->get_blink_off_time(),
                            light < phases.size() ? phases[light] : 0.0f);
                    break;
                default:
                    break; // LIGHT_KEEP
            }
        }
        data = signal_heads.getptr(p_signal_head); // the light signals may have rehashed the table
        ERR_FAIL_NULL(data);
        data->aspect = p_aspect;
        const RID system_rid = data->system;
        emit_signal(signal_head_aspect_changed_signal, p_signal_head, p_aspect);
        const SystemData *system = systems.getptr(system_rid);
        if (system != nullptr && system->implementation.is_valid()) {
            const Ref<SignallingImplementation> implementation = system->implementation;
            implementation->signal_head_aspect_changed(system_rid, p_signal_head, p_aspect);
        }
    }

    StringName SignallingServer::signal_head_get_aspect(const RID &p_signal_head) const {
        const SignalHeadData *data = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL_V(data, StringName());
        return data->aspect;
    }

    // --- system ---

    RID SignallingServer::system_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        systems.insert(rid, SystemData());
        return rid;
    }

    /// Frees the system; its signal heads and sources stay, belonging to no system
    void SignallingServer::system_free(const RID &p_system) {
        ERR_FAIL_COND(!systems.has(p_system));
        system_attach_implementation(p_system, Ref<SignallingImplementation>());
        const SystemData freed = *systems.getptr(p_system);
        systems.erase(p_system);
        names_rename(systems_by_name, freed.name, StringName(), p_system);
        for (const RID &signal_head_rid: freed.signal_heads) {
            if (SignalHeadData *signal_head = signal_heads.getptr(signal_head_rid); signal_head != nullptr) {
                signal_head->system = RID();
            }
        }
        for (const RID &source_rid: freed.sources) {
            if (SourceData *source = sources.getptr(source_rid); source != nullptr) {
                source->system = RID();
            }
        }
    }

    /// Replaces the system's implementation (null detaches it). The new implementation is told about every
    /// signal head and source the system already holds, so the order of setting up does not matter.
    void SignallingServer::system_attach_implementation(
            const RID &p_system, const Ref<SignallingImplementation> &p_implementation) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        const Ref<SignallingImplementation> previous = system->implementation;
        system->implementation = p_implementation;
        // copied: an implementation may call back into this server and rehash `systems`
        const Vector<RID> held_signal_heads = system->signal_heads;
        const Vector<RID> held_sources = system->sources;
        if (previous.is_valid()) {
            previous->system_detached(p_system);
        }
        if (p_implementation.is_null()) {
            return;
        }
        p_implementation->system_attached(p_system);
        for (const RID &signal_head: held_signal_heads) {
            p_implementation->signal_head_added(p_system, signal_head);
        }
        for (const RID &source: held_sources) {
            p_implementation->source_added(p_system, source);
        }
    }

    Ref<SignallingImplementation> SignallingServer::system_get_implementation(const RID &p_system) const {
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, Ref<SignallingImplementation>());
        return system->implementation;
    }

    void SignallingServer::system_set_name(const RID &p_system, const StringName &p_name) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        const StringName previous = system->name;
        system->name = p_name;
        names_rename(systems_by_name, previous, p_name, p_system);
    }

    StringName SignallingServer::system_get_name(const RID &p_system) const {
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, StringName());
        return system->name;
    }

    RID SignallingServer::system_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = systems_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    /// A signal head belongs to one system at most - the one that decides its aspects
    void SignallingServer::system_add_signal_head(const RID &p_system, const RID &p_signal_head) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        SignalHeadData *signal_head = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(signal_head);
        ERR_FAIL_COND_MSG(signal_head->system.is_valid(), "The signal head already belongs to a system.");
        signal_head->system = p_system;
        system->signal_heads.push_back(p_signal_head);
        const Ref<SignallingImplementation> implementation = system->implementation;
        emit_signal(system_signal_head_added_signal, p_system, p_signal_head);
        if (implementation.is_valid()) {
            implementation->signal_head_added(p_system, p_signal_head);
        }
    }

    void SignallingServer::system_remove_signal_head(const RID &p_system, const RID &p_signal_head) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        SignalHeadData *signal_head = signal_heads.getptr(p_signal_head);
        ERR_FAIL_NULL(signal_head);
        ERR_FAIL_COND_MSG(!(signal_head->system == p_system), "The signal head does not belong to this system.");
        signal_head->system = RID();
        system->signal_heads.erase(p_signal_head);
        const Ref<SignallingImplementation> implementation = system->implementation;
        emit_signal(system_signal_head_removed_signal, p_system, p_signal_head);
        if (implementation.is_valid()) {
            implementation->signal_head_removed(p_system, p_signal_head);
        }
    }

    TypedArray<RID> SignallingServer::system_get_signal_heads(const RID &p_system) const {
        TypedArray<RID> result;
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, result);
        for (const RID &signal_head: system->signal_heads) {
            result.push_back(signal_head);
        }
        return result;
    }

    /// A source belongs to one system at most - the one its events go to
    void SignallingServer::system_add_source(const RID &p_system, const RID &p_source) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL(source);
        ERR_FAIL_COND_MSG(source->system.is_valid(), "The source already belongs to a system.");
        source->system = p_system;
        system->sources.push_back(p_source);
        const Ref<SignallingImplementation> implementation = system->implementation;
        emit_signal(system_source_added_signal, p_system, p_source);
        if (implementation.is_valid()) {
            implementation->source_added(p_system, p_source);
        }
    }

    void SignallingServer::system_remove_source(const RID &p_system, const RID &p_source) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL(source);
        ERR_FAIL_COND_MSG(!(source->system == p_system), "The source does not belong to this system.");
        source->system = RID();
        system->sources.erase(p_source);
        const Ref<SignallingImplementation> implementation = system->implementation;
        emit_signal(system_source_removed_signal, p_system, p_source);
        if (implementation.is_valid()) {
            implementation->source_removed(p_system, p_source);
        }
    }

    TypedArray<RID> SignallingServer::system_get_sources(const RID &p_system) const {
        TypedArray<RID> result;
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, result);
        for (const RID &source: system->sources) {
            result.push_back(source);
        }
        return result;
    }

    /// A command to the system as a whole, handed to its implementation
    void
    SignallingServer::system_send_event(const RID &p_system, const StringName &p_event, const Dictionary &p_arguments) {
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        const Ref<SignallingImplementation> implementation = system->implementation;
        ERR_FAIL_COND_MSG(implementation.is_null(), "The system has no implementation.");
        implementation->handle_event(p_system, p_event, p_arguments);
    }

    // --- source ---

    RID SignallingServer::source_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        sources.insert(rid, SourceData());
        return rid;
    }

    void SignallingServer::source_free(const RID &p_source) {
        const SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL(source);
        if (source->system.is_valid()) {
            system_remove_source(source->system, p_source);
        }
        const StringName name = sources.getptr(p_source)->name;
        sources.erase(p_source);
        names_rename(sources_by_name, name, StringName(), p_source);
    }

    void SignallingServer::source_set_name(const RID &p_source, const StringName &p_name) {
        SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL(source);
        const StringName previous = source->name;
        source->name = p_name;
        names_rename(sources_by_name, previous, p_name, p_source);
    }

    StringName SignallingServer::source_get_name(const RID &p_source) const {
        const SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL_V(source, StringName());
        return source->name;
    }

    RID SignallingServer::source_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = sources_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    RID SignallingServer::source_get_system(const RID &p_source) const {
        const SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL_V(source, RID());
        return source->system;
    }

    /// A report of the source, handed to the implementation of the system it belongs to
    void
    SignallingServer::source_send_event(const RID &p_source, const StringName &p_event, const Dictionary &p_arguments) {
        const SourceData *source = sources.getptr(p_source);
        ERR_FAIL_NULL(source);
        const SystemData *system = systems.getptr(source->system);
        ERR_FAIL_NULL_MSG(system, "The source belongs to no system.");
        const Ref<SignallingImplementation> implementation = system->implementation;
        ERR_FAIL_COND_MSG(implementation.is_null(), "The system has no implementation.");
        implementation->handle_source_event(source->system, p_source, p_event, p_arguments);
    }
} // namespace godot
