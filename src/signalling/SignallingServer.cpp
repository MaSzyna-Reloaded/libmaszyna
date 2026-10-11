#include "../macros.hpp"
#include "SignallingServer.hpp"
#include "legacy/e3d/E3DRenderingServer.hpp"
#include "utils/Names.hpp"
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    static Vector<RID> rids_to_vector(const TypedArray<RID> &p_rids) {
        Vector<RID> result;
        for (int index = 0; index < p_rids.size(); index++) {
            result.push_back(p_rids[index]);
        }
        return result;
    }

    static TypedArray<RID> vector_to_rids(const Vector<RID> &p_rids) {
        TypedArray<RID> result;
        for (const RID &rid: p_rids) {
            result.push_back(rid);
        }
        return result;
    }

    const char *SignallingServer::signal_head_registered_signal = "signal_head_registered";
    const char *SignallingServer::signal_head_unregistered_signal = "signal_head_unregistered";
    const char *SignallingServer::signal_head_light_state_changed_signal = "signal_head_light_state_changed";
    const char *SignallingServer::signal_head_aspect_changed_signal = "signal_head_aspect_changed";
    const char *SignallingServer::signal_head_config_changed_signal = "signal_head_config_changed";
    const char *SignallingServer::system_signal_head_added_signal = "system_signal_head_added";
    const char *SignallingServer::system_signal_head_removed_signal = "system_signal_head_removed";
    const char *SignallingServer::system_source_added_signal = "system_source_added";
    const char *SignallingServer::system_source_removed_signal = "system_source_removed";
    const char *SignallingServer::system_switch_added_signal = "system_switch_added";
    const char *SignallingServer::system_switch_removed_signal = "system_switch_removed";
    const char *SignallingServer::system_linked_signal = "system_linked";
    const char *SignallingServer::system_unlinked_signal = "system_unlinked";
    const char *SignallingServer::route_state_changed_signal = "route_state_changed";
    const char *SignallingServer::route_section_released_signal = "route_section_released";
    const char *SignallingServer::route_config_changed_signal = "route_config_changed";

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

        ClassDB::bind_method(D_METHOD("system_add_switch", "system", "switch"), &SignallingServer::system_add_switch);
        ClassDB::bind_method(
                D_METHOD("system_remove_switch", "system", "switch"), &SignallingServer::system_remove_switch);
        ClassDB::bind_method(D_METHOD("system_get_switches", "system"), &SignallingServer::system_get_switches);
        ClassDB::bind_method(D_METHOD("switch_get_system", "switch"), &SignallingServer::switch_get_system);
        ClassDB::bind_method(D_METHOD("system_link", "system", "other"), &SignallingServer::system_link);
        ClassDB::bind_method(D_METHOD("system_unlink", "system", "other"), &SignallingServer::system_unlink);
        ClassDB::bind_method(
                D_METHOD("system_get_linked_systems", "system"), &SignallingServer::system_get_linked_systems);
        ClassDB::bind_method(D_METHOD("system_add_route", "system", "route"), &SignallingServer::system_add_route);
        ClassDB::bind_method(
                D_METHOD("system_remove_route", "system", "route"), &SignallingServer::system_remove_route);
        ClassDB::bind_method(D_METHOD("system_get_routes", "system"), &SignallingServer::system_get_routes);

        ClassDB::bind_method(D_METHOD("source_create"), &SignallingServer::source_create);
        ClassDB::bind_method(D_METHOD("source_free", "source"), &SignallingServer::source_free);
        ClassDB::bind_method(D_METHOD("source_set_name", "source", "name"), &SignallingServer::source_set_name);
        ClassDB::bind_method(D_METHOD("source_get_name", "source"), &SignallingServer::source_get_name);
        ClassDB::bind_method(D_METHOD("source_get_rid_by_name", "name"), &SignallingServer::source_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("source_get_system", "source"), &SignallingServer::source_get_system);
        ClassDB::bind_method(
                D_METHOD("source_send_event", "source", "event", "arguments"), &SignallingServer::source_send_event,
                DEFVAL(Dictionary()));

        ClassDB::bind_method(D_METHOD("route_create"), &SignallingServer::route_create);
        ClassDB::bind_method(D_METHOD("route_free", "route"), &SignallingServer::route_free);
        ClassDB::bind_method(D_METHOD("route_set_name", "route", "name"), &SignallingServer::route_set_name);
        ClassDB::bind_method(D_METHOD("route_get_name", "route"), &SignallingServer::route_get_name);
        ClassDB::bind_method(D_METHOD("route_get_rid_by_name", "name"), &SignallingServer::route_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("route_get_system", "route"), &SignallingServer::route_get_system);
        ClassDB::bind_method(D_METHOD("route_set_kind", "route", "kind"), &SignallingServer::route_set_kind);
        ClassDB::bind_method(D_METHOD("route_get_kind", "route"), &SignallingServer::route_get_kind);
        ClassDB::bind_method(
                D_METHOD("route_set_start_signal_head", "route", "signal_head"),
                &SignallingServer::route_set_start_signal_head);
        ClassDB::bind_method(
                D_METHOD("route_get_start_signal_head", "route"), &SignallingServer::route_get_start_signal_head);
        ClassDB::bind_method(
                D_METHOD("route_set_end_signal_head", "route", "signal_head"),
                &SignallingServer::route_set_end_signal_head);
        ClassDB::bind_method(
                D_METHOD("route_get_end_signal_head", "route"), &SignallingServer::route_get_end_signal_head);
        ClassDB::bind_method(D_METHOD("route_set_sections", "route", "sources"), &SignallingServer::route_set_sections);
        ClassDB::bind_method(D_METHOD("route_get_sections", "route"), &SignallingServer::route_get_sections);
        ClassDB::bind_method(
                D_METHOD("route_set_overlap_sections", "route", "sources"),
                &SignallingServer::route_set_overlap_sections);
        ClassDB::bind_method(
                D_METHOD("route_get_overlap_sections", "route"), &SignallingServer::route_get_overlap_sections);
        ClassDB::bind_method(
                D_METHOD("route_set_approach_sections", "route", "sources"),
                &SignallingServer::route_set_approach_sections);
        ClassDB::bind_method(
                D_METHOD("route_get_approach_sections", "route"), &SignallingServer::route_get_approach_sections);
        ClassDB::bind_method(
                D_METHOD("route_set_switch_position", "route", "switch", "switch_track"),
                &SignallingServer::route_set_switch_position);
        ClassDB::bind_method(
                D_METHOD("route_remove_switch_position", "route", "switch"),
                &SignallingServer::route_remove_switch_position);
        ClassDB::bind_method(
                D_METHOD("route_get_switch_positions", "route"), &SignallingServer::route_get_switch_positions);
        ClassDB::bind_method(D_METHOD("route_add_conflict", "route", "other"), &SignallingServer::route_add_conflict);
        ClassDB::bind_method(
                D_METHOD("route_remove_conflict", "route", "other"), &SignallingServer::route_remove_conflict);
        ClassDB::bind_method(D_METHOD("route_get_conflicts", "route"), &SignallingServer::route_get_conflicts);
        ClassDB::bind_method(D_METHOD("route_set_state", "route", "state"), &SignallingServer::route_set_state);
        ClassDB::bind_method(D_METHOD("route_get_state", "route"), &SignallingServer::route_get_state);
        ClassDB::bind_method(
                D_METHOD("route_release_section", "route", "source"), &SignallingServer::route_release_section);
        ClassDB::bind_method(
                D_METHOD("route_get_locked_sections", "route"), &SignallingServer::route_get_locked_sections);
        ClassDB::bind_method(D_METHOD("route_request_set", "route"), &SignallingServer::route_request_set);
        ClassDB::bind_method(D_METHOD("route_request_cancel", "route"), &SignallingServer::route_request_cancel);
        ClassDB::bind_method(D_METHOD("route_request_release", "route"), &SignallingServer::route_request_release);
        ClassDB::bind_method(
                D_METHOD("route_request_emergency_release", "route"),
                &SignallingServer::route_request_emergency_release);

        BIND_ENUM_CONSTANT(LIGHT_STATE_OFF);
        BIND_ENUM_CONSTANT(LIGHT_STATE_ON);
        BIND_ENUM_CONSTANT(LIGHT_STATE_BLINKING);
        BIND_ENUM_CONSTANT(SOURCE_POLARITY_NONE);
        BIND_ENUM_CONSTANT(SOURCE_POLARITY_POSITIVE);
        BIND_ENUM_CONSTANT(SOURCE_POLARITY_NEGATIVE);
        BIND_ENUM_CONSTANT(ROUTE_KIND_TRAIN);
        BIND_ENUM_CONSTANT(ROUTE_KIND_SHUNTING);
        BIND_ENUM_CONSTANT(ROUTE_STATE_IDLE);
        BIND_ENUM_CONSTANT(ROUTE_STATE_SETTING);
        BIND_ENUM_CONSTANT(ROUTE_STATE_CLOSED);
        BIND_ENUM_CONSTANT(ROUTE_STATE_LOCKED);
        BIND_ENUM_CONSTANT(ROUTE_STATE_RELEASING);
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
        ADD_SIGNAL(MethodInfo(
                system_switch_added_signal, PropertyInfo(Variant::RID, "system"),
                PropertyInfo(Variant::STRING_NAME, "switch")));
        ADD_SIGNAL(MethodInfo(
                system_switch_removed_signal, PropertyInfo(Variant::RID, "system"),
                PropertyInfo(Variant::STRING_NAME, "switch")));
        ADD_SIGNAL(MethodInfo(
                system_linked_signal, PropertyInfo(Variant::RID, "system"), PropertyInfo(Variant::RID, "other")));
        ADD_SIGNAL(MethodInfo(
                system_unlinked_signal, PropertyInfo(Variant::RID, "system"), PropertyInfo(Variant::RID, "other")));
        ADD_SIGNAL(MethodInfo(
                route_state_changed_signal, PropertyInfo(Variant::RID, "route"),
                PropertyInfo(Variant::INT, "state", PROPERTY_HINT_ENUM, route_state_hint())));
        ADD_SIGNAL(MethodInfo(
                route_section_released_signal, PropertyInfo(Variant::RID, "route"),
                PropertyInfo(Variant::RID, "source")));
        ADD_SIGNAL(MethodInfo(route_config_changed_signal, PropertyInfo(Variant::RID, "route")));
    }

    String SignallingServer::light_state_hint() {
        return enum_hint({{"Off", LIGHT_STATE_OFF}, {"On", LIGHT_STATE_ON}, {"Blinking", LIGHT_STATE_BLINKING}});
    }

    String SignallingServer::route_state_hint() {
        return enum_hint(
                {{"Idle", ROUTE_STATE_IDLE},
                 {"Setting", ROUTE_STATE_SETTING},
                 {"Closed", ROUTE_STATE_CLOSED},
                 {"Locked", ROUTE_STATE_LOCKED},
                 {"Releasing", ROUTE_STATE_RELEASING}});
    }

    /// A signal head is the lights of an instance, so it goes when the instance does; a switch of a
    /// system reports to its implementation where it came to lie, and when it comes or goes
    SignallingServer::SignallingServer() {
        E3DRenderingServer *rendering = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(rendering);
        rendering->connect(
                E3DRenderingServer::instance_freed_signal, callable_mp(this, &SignallingServer::_on_instance_freed));
        rendering->connect(
                E3DRenderingServer::instance_built_signal, callable_mp(this, &SignallingServer::_on_instance_built));
        TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        tracks->connect(
                TrackServer::switch_movement_finished_signal,
                callable_mp(this, &SignallingServer::_on_switch_movement_finished));
        tracks->connect(TrackServer::switch_created_signal, callable_mp(this, &SignallingServer::_on_switch_created));
        tracks->connect(TrackServer::switch_freed_signal, callable_mp(this, &SignallingServer::_on_switch_freed));
    }

    Ref<SignallingImplementation>
    SignallingServer::_switch_implementation(const StringName &p_switch, RID &p_r_system) const {
        const RID *system_rid = systems_by_switch_name.getptr(p_switch);
        if (system_rid == nullptr) {
            return Ref<SignallingImplementation>();
        }
        const SystemData *system = systems.getptr(*system_rid);
        ERR_FAIL_NULL_V(system, Ref<SignallingImplementation>());
        p_r_system = *system_rid;
        return system->implementation;
    }

    void SignallingServer::_on_switch_movement_finished(const RID &p_switch, const int p_active_track) {
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        const StringName name = tracks->switch_get_name(p_switch);
        RID system;
        const Ref<SignallingImplementation> implementation = _switch_implementation(name, system);
        if (implementation.is_valid()) {
            implementation->switch_movement_finished(system, name, p_active_track);
        }
    }

    void SignallingServer::_on_switch_created(const RID &p_switch) {
        const TrackServer *tracks = TrackServer::get_instance();
        ERR_FAIL_NULL(tracks);
        const StringName name = tracks->switch_get_name(p_switch);
        RID system;
        const Ref<SignallingImplementation> implementation = _switch_implementation(name, system);
        if (implementation.is_valid()) {
            implementation->switch_created(system, name);
        }
    }

    void SignallingServer::_on_switch_freed(const RID &p_switch, const String &p_name) {
        RID system;
        const Ref<SignallingImplementation> implementation = _switch_implementation(p_name, system);
        if (implementation.is_valid()) {
            implementation->switch_freed(system, p_name);
        }
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
        _forget_in_routes(p_signal_head);
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

    /// Frees the system; its signal heads, sources, switches and routes stay, belonging to no system, and
    /// its links to other systems go
    void SignallingServer::system_free(const RID &p_system) {
        ERR_FAIL_COND(!systems.has(p_system));
        system_attach_implementation(p_system, Ref<SignallingImplementation>());
        const Vector<RID> linked = systems.getptr(p_system)->linked_systems;
        for (const RID &other: linked) {
            system_unlink(p_system, other);
        }
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
        for (const StringName &switch_name: freed.switch_names) {
            systems_by_switch_name.erase(switch_name);
        }
        for (const RID &route_rid: freed.routes) {
            if (RouteData *route = routes.getptr(route_rid); route != nullptr) {
                route->system = RID();
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
        const Vector<StringName> held_switches = system->switch_names;
        const Vector<RID> held_links = system->linked_systems;
        const Vector<RID> held_routes = system->routes;
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
        for (const StringName &switch_name: held_switches) {
            p_implementation->switch_added(p_system, switch_name);
        }
        for (const RID &other: held_links) {
            p_implementation->system_linked(p_system, other);
        }
        for (const RID &route: held_routes) {
            p_implementation->route_added(p_system, route);
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
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, TypedArray<RID>());
        return vector_to_rids(system->signal_heads);
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
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, TypedArray<RID>());
        return vector_to_rids(system->sources);
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

    void SignallingServer::system_add_switch(const RID &p_system, const StringName &p_switch) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        ERR_FAIL_COND_MSG(p_switch.is_empty(), "A switch is added by its name.");
        ERR_FAIL_COND_MSG(systems_by_switch_name.has(p_switch), "The switch already belongs to a system.");
        systems_by_switch_name.insert(p_switch, p_system);
        system->switch_names.push_back(p_switch);
        const Ref<SignallingImplementation> implementation = system->implementation;
        emit_signal(system_switch_added_signal, p_system, p_switch);
        if (implementation.is_valid()) {
            implementation->switch_added(p_system, p_switch);
        }
    }

    void SignallingServer::system_remove_switch(const RID &p_system, const StringName &p_switch) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        const RID *owner = systems_by_switch_name.getptr(p_switch);
        ERR_FAIL_COND_MSG(owner == nullptr || !(*owner == p_system), "The switch does not belong to this system.");
        systems_by_switch_name.erase(p_switch);
        system->switch_names.erase(p_switch);
        const Ref<SignallingImplementation> implementation = system->implementation;
        emit_signal(system_switch_removed_signal, p_system, p_switch);
        if (implementation.is_valid()) {
            implementation->switch_removed(p_system, p_switch);
        }
    }

    PackedStringArray SignallingServer::system_get_switches(const RID &p_system) const {
        PackedStringArray result;
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, result);
        for (const StringName &switch_name: system->switch_names) {
            result.push_back(switch_name);
        }
        return result;
    }

    RID SignallingServer::switch_get_system(const StringName &p_switch) const {
        const RID *system = systems_by_switch_name.getptr(p_switch);
        return system != nullptr ? *system : RID();
    }

    void SignallingServer::system_link(const RID &p_system, const RID &p_other) {
        ERR_FAIL_COND_MSG(p_system == p_other, "A system cannot be linked to itself.");
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        SystemData *other = systems.getptr(p_other);
        ERR_FAIL_NULL(other);
        ERR_FAIL_COND_MSG(system->linked_systems.has(p_other), "The systems are already linked.");
        system->linked_systems.push_back(p_other);
        other->linked_systems.push_back(p_system);
        const Ref<SignallingImplementation> implementation = system->implementation;
        const Ref<SignallingImplementation> other_implementation = other->implementation;
        emit_signal(system_linked_signal, p_system, p_other);
        if (implementation.is_valid()) {
            implementation->system_linked(p_system, p_other);
        }
        if (other_implementation.is_valid()) {
            // the other system is told from its own side, so the arguments are meant the other way round
            // NOLINTNEXTLINE(readability-suspicious-call-argument)
            other_implementation->system_linked(p_other, p_system);
        }
    }

    void SignallingServer::system_unlink(const RID &p_system, const RID &p_other) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        SystemData *other = systems.getptr(p_other);
        ERR_FAIL_NULL(other);
        ERR_FAIL_COND_MSG(!system->linked_systems.has(p_other), "The systems are not linked.");
        system->linked_systems.erase(p_other);
        other->linked_systems.erase(p_system);
        const Ref<SignallingImplementation> implementation = system->implementation;
        const Ref<SignallingImplementation> other_implementation = other->implementation;
        emit_signal(system_unlinked_signal, p_system, p_other);
        if (implementation.is_valid()) {
            implementation->system_unlinked(p_system, p_other);
        }
        if (other_implementation.is_valid()) {
            // the other system is told from its own side, so the arguments are meant the other way round
            // NOLINTNEXTLINE(readability-suspicious-call-argument)
            other_implementation->system_unlinked(p_other, p_system);
        }
    }

    TypedArray<RID> SignallingServer::system_get_linked_systems(const RID &p_system) const {
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, TypedArray<RID>());
        return vector_to_rids(system->linked_systems);
    }

    void SignallingServer::system_add_route(const RID &p_system, const RID &p_route) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        ERR_FAIL_COND_MSG(route->system.is_valid(), "The route already belongs to a system.");
        route->system = p_system;
        system->routes.push_back(p_route);
        const Ref<SignallingImplementation> implementation = system->implementation;
        if (implementation.is_valid()) {
            implementation->route_added(p_system, p_route);
        }
    }

    void SignallingServer::system_remove_route(const RID &p_system, const RID &p_route) {
        SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL(system);
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        ERR_FAIL_COND_MSG(!(route->system == p_system), "The route does not belong to this system.");
        route->system = RID();
        system->routes.erase(p_route);
        const Ref<SignallingImplementation> implementation = system->implementation;
        if (implementation.is_valid()) {
            implementation->route_removed(p_system, p_route);
        }
    }

    TypedArray<RID> SignallingServer::system_get_routes(const RID &p_system) const {
        const SystemData *system = systems.getptr(p_system);
        ERR_FAIL_NULL_V(system, TypedArray<RID>());
        return vector_to_rids(system->routes);
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
        _forget_in_routes(p_source);
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

    // --- route ---

    RID SignallingServer::route_create() {
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        routes.insert(rid, RouteData());
        return rid;
    }

    /// Frees the route; the routes it conflicted with no longer name it
    void SignallingServer::route_free(const RID &p_route) {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        if (route->system.is_valid()) {
            system_remove_route(route->system, p_route);
        }
        const RouteData freed = *routes.getptr(p_route);
        routes.erase(p_route);
        names_rename(routes_by_name, freed.name, StringName(), p_route);
        for (const RID &other_rid: freed.conflicts) {
            if (RouteData *other = routes.getptr(other_rid); other != nullptr) {
                other->conflicts.erase(p_route);
            }
        }
        for (const RID &other_rid: freed.conflicts) {
            emit_signal(route_config_changed_signal, other_rid);
        }
    }

    void SignallingServer::route_set_name(const RID &p_route, const StringName &p_name) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        const StringName previous = route->name;
        route->name = p_name;
        names_rename(routes_by_name, previous, p_name, p_route);
    }

    StringName SignallingServer::route_get_name(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, StringName());
        return route->name;
    }

    RID SignallingServer::route_get_rid_by_name(const StringName &p_name) const {
        const RID *rid = routes_by_name.getptr(p_name);
        return rid != nullptr ? *rid : RID();
    }

    RID SignallingServer::route_get_system(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, RID());
        return route->system;
    }

    void SignallingServer::route_set_kind(const RID &p_route, const RouteKind p_kind) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->kind = p_kind;
        emit_signal(route_config_changed_signal, p_route);
    }

    SignallingServer::RouteKind SignallingServer::route_get_kind(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, ROUTE_KIND_TRAIN);
        return route->kind;
    }

    void SignallingServer::route_set_start_signal_head(const RID &p_route, const RID &p_signal_head) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->start_signal_head = p_signal_head;
        emit_signal(route_config_changed_signal, p_route);
    }

    RID SignallingServer::route_get_start_signal_head(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, RID());
        return route->start_signal_head;
    }

    /// An empty RID: the route ends at the station's boundary or a W5 sign
    void SignallingServer::route_set_end_signal_head(const RID &p_route, const RID &p_signal_head) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->end_signal_head = p_signal_head;
        emit_signal(route_config_changed_signal, p_route);
    }

    RID SignallingServer::route_get_end_signal_head(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, RID());
        return route->end_signal_head;
    }

    void SignallingServer::route_set_sections(const RID &p_route, const TypedArray<RID> &p_sources) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->sections = rids_to_vector(p_sources);
        emit_signal(route_config_changed_signal, p_route);
    }

    TypedArray<RID> SignallingServer::route_get_sections(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, TypedArray<RID>());
        return vector_to_rids(route->sections);
    }

    void SignallingServer::route_set_overlap_sections(const RID &p_route, const TypedArray<RID> &p_sources) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->overlap_sections = rids_to_vector(p_sources);
        emit_signal(route_config_changed_signal, p_route);
    }

    TypedArray<RID> SignallingServer::route_get_overlap_sections(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, TypedArray<RID>());
        return vector_to_rids(route->overlap_sections);
    }

    void SignallingServer::route_set_approach_sections(const RID &p_route, const TypedArray<RID> &p_sources) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->approach_sections = rids_to_vector(p_sources);
        emit_signal(route_config_changed_signal, p_route);
    }

    TypedArray<RID> SignallingServer::route_get_approach_sections(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, TypedArray<RID>());
        return vector_to_rids(route->approach_sections);
    }

    void SignallingServer::route_set_switch_position(
            const RID &p_route, const StringName &p_switch, const TrackServer::SwitchTrack p_switch_track) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        route->switch_positions.insert(p_switch, p_switch_track);
        emit_signal(route_config_changed_signal, p_route);
    }

    void SignallingServer::route_remove_switch_position(const RID &p_route, const StringName &p_switch) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        ERR_FAIL_COND_MSG(!route->switch_positions.erase(p_switch), "The route has no position of the switch.");
        emit_signal(route_config_changed_signal, p_route);
    }

    Dictionary SignallingServer::route_get_switch_positions(const RID &p_route) const {
        Dictionary result;
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, result);
        for (const KeyValue<StringName, TrackServer::SwitchTrack> &position: route->switch_positions) {
            result[position.key] = position.value;
        }
        return result;
    }

    void SignallingServer::route_add_conflict(const RID &p_route, const RID &p_other) {
        ERR_FAIL_COND_MSG(p_route == p_other, "A route cannot conflict with itself.");
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        RouteData *other = routes.getptr(p_other);
        ERR_FAIL_NULL(other);
        ERR_FAIL_COND_MSG(route->conflicts.has(p_other), "The routes already conflict.");
        route->conflicts.push_back(p_other);
        other->conflicts.push_back(p_route);
        emit_signal(route_config_changed_signal, p_route);
        emit_signal(route_config_changed_signal, p_other);
    }

    void SignallingServer::route_remove_conflict(const RID &p_route, const RID &p_other) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        RouteData *other = routes.getptr(p_other);
        ERR_FAIL_NULL(other);
        ERR_FAIL_COND_MSG(!route->conflicts.has(p_other), "The routes do not conflict.");
        route->conflicts.erase(p_other);
        other->conflicts.erase(p_route);
        emit_signal(route_config_changed_signal, p_route);
        emit_signal(route_config_changed_signal, p_other);
    }

    TypedArray<RID> SignallingServer::route_get_conflicts(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, TypedArray<RID>());
        return vector_to_rids(route->conflicts);
    }

    /// Closing or locking a route that held nothing holds all its sections and its overlap; going idle
    /// lets them go
    void SignallingServer::route_set_state(const RID &p_route, const RouteState p_state) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        if (route->state == p_state) {
            return;
        }
        const bool holding = route->state == ROUTE_STATE_CLOSED || route->state == ROUTE_STATE_LOCKED ||
                             route->state == ROUTE_STATE_RELEASING;
        if (p_state == ROUTE_STATE_IDLE) {
            route->locked_sections.clear();
        } else if (!holding && (p_state == ROUTE_STATE_CLOSED || p_state == ROUTE_STATE_LOCKED)) {
            route->locked_sections = route->sections;
            route->locked_sections.append_array(route->overlap_sections);
        }
        route->state = p_state;
        emit_signal(route_state_changed_signal, p_route, p_state);
    }

    SignallingServer::RouteState SignallingServer::route_get_state(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, ROUTE_STATE_IDLE);
        return route->state;
    }

    void SignallingServer::route_release_section(const RID &p_route, const RID &p_source) {
        RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL(route);
        ERR_FAIL_COND_MSG(!route->locked_sections.has(p_source), "The route does not hold the section.");
        route->locked_sections.erase(p_source);
        emit_signal(route_section_released_signal, p_route, p_source);
    }

    TypedArray<RID> SignallingServer::route_get_locked_sections(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, TypedArray<RID>());
        return vector_to_rids(route->locked_sections);
    }

    Ref<SignallingImplementation> SignallingServer::_route_implementation(const RID &p_route) const {
        const RouteData *route = routes.getptr(p_route);
        ERR_FAIL_NULL_V(route, Ref<SignallingImplementation>());
        const SystemData *system = systems.getptr(route->system);
        ERR_FAIL_NULL_V_MSG(system, Ref<SignallingImplementation>(), "The route belongs to no system.");
        ERR_FAIL_COND_V_MSG(
                system->implementation.is_null(), Ref<SignallingImplementation>(),
                "The route's system has no implementation.");
        return system->implementation;
    }

    void SignallingServer::route_request_set(const RID &p_route) {
        const Ref<SignallingImplementation> implementation = _route_implementation(p_route);
        if (implementation.is_valid()) {
            implementation->route_set_requested(routes.getptr(p_route)->system, p_route);
        }
    }

    void SignallingServer::route_request_cancel(const RID &p_route) {
        const Ref<SignallingImplementation> implementation = _route_implementation(p_route);
        if (implementation.is_valid()) {
            implementation->route_cancel_requested(routes.getptr(p_route)->system, p_route);
        }
    }

    void SignallingServer::route_request_release(const RID &p_route) {
        const Ref<SignallingImplementation> implementation = _route_implementation(p_route);
        if (implementation.is_valid()) {
            implementation->route_release_requested(routes.getptr(p_route)->system, p_route);
        }
    }

    void SignallingServer::route_request_emergency_release(const RID &p_route) {
        const Ref<SignallingImplementation> implementation = _route_implementation(p_route);
        if (implementation.is_valid()) {
            implementation->route_emergency_release_requested(routes.getptr(p_route)->system, p_route);
        }
    }

    void SignallingServer::_forget_in_routes(const RID &p_element) {
        Vector<RID> changed;
        for (KeyValue<RID, RouteData> &entry: routes) {
            RouteData &route = entry.value;
            bool named = false;
            for (Vector<RID> *list:
                 {&route.sections, &route.overlap_sections, &route.approach_sections, &route.locked_sections}) {
                if (list->has(p_element)) {
                    list->erase(p_element);
                    named = true;
                }
            }
            for (RID *signal_head: {&route.start_signal_head, &route.end_signal_head}) {
                if (*signal_head == p_element) {
                    *signal_head = RID();
                    named = true;
                }
            }
            if (named) {
                changed.push_back(entry.key);
            }
        }
        for (const RID &route: changed) {
            emit_signal(route_config_changed_signal, route);
        }
    }
} // namespace godot
