#include "MaszynaE3DRenderingServer.hpp"
#include "MaszynaLightMode.hpp"
#include "game_data/GameDataServer.hpp"
#include "rendering/SmokeServer.hpp"
#include "resources/ResourceLazyLoader.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "signalling/SignallingServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "utils/RIDHash.hpp"
#include "utils/Units.hpp"
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/spot_light3d.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    MaszynaE3DRenderingServer *MaszynaE3DRenderingServer::singleton = nullptr;
    const char *MaszynaE3DRenderingServer::instance_submodel_animation_finished_signal =
            "instance_submodel_animation_finished";

    void MaszynaE3DRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("instance_get_attached_node", "instance"),
                &MaszynaE3DRenderingServer::instance_get_attached_node);
        ClassDB::bind_method(D_METHOD("smoke_rebuild"), &MaszynaE3DRenderingServer::smoke_rebuild);
        ClassDB::bind_method(
                D_METHOD("instance_set_layer_mask", "instance", "mask"),
                &MaszynaE3DRenderingServer::instance_set_layer_mask);
        ClassDB::bind_method(
                D_METHOD("instance_set_visibility_range", "instance", "begin", "end"),
                &MaszynaE3DRenderingServer::instance_set_visibility_range);
        ClassDB::bind_method(
                D_METHOD("instance_set_part_lights_on_threshold", "instance", "submodel", "threshold"),
                &MaszynaE3DRenderingServer::instance_set_part_lights_on_threshold);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_modes", "instance", "modes"),
                &MaszynaE3DRenderingServer::instance_set_lights_modes);
        ClassDB::bind_method(
                D_METHOD("instance_set_lights_colors", "instance", "lights_colors"),
                &MaszynaE3DRenderingServer::instance_set_lights_colors);
        ClassDB::bind_static_method(
                get_class_static(), D_METHOD("light_get_name", "index"), &MaszynaE3DRenderingServer::light_get_name);
        ClassDB::bind_method(
                D_METHOD("instance_set_light_mode", "instance", "light", "mode"),
                &MaszynaE3DRenderingServer::instance_set_light_mode);
        ClassDB::bind_method(
                D_METHOD("instance_get_light_count", "instance"), &MaszynaE3DRenderingServer::instance_get_light_count);
        ClassDB::bind_method(
                D_METHOD("instance_set_submodel_done_event", "instance", "submodel", "event"),
                &MaszynaE3DRenderingServer::instance_set_submodel_done_event);
        ClassDB::bind_method(
                D_METHOD("instance_get_opaque_meshes", "instance"),
                &MaszynaE3DRenderingServer::instance_get_opaque_meshes);
        ClassDB::bind_method(
                D_METHOD("instance_set_submodel_rotation", "instance", "submodel", "degrees", "speed"),
                &MaszynaE3DRenderingServer::instance_set_submodel_rotation);
        ClassDB::bind_method(
                D_METHOD("instance_set_submodel_translation", "instance", "submodel", "offset", "speed"),
                &MaszynaE3DRenderingServer::instance_set_submodel_translation);
        ClassDB::bind_method(
                D_METHOD("instance_get_model", "instance"), &MaszynaE3DRenderingServer::instance_get_model);
        ClassDB::bind_method(
                D_METHOD("instance_get_data_path", "instance"), &MaszynaE3DRenderingServer::instance_get_data_path);
        ClassDB::bind_method(
                D_METHOD("instance_get_model_filename", "instance"),
                &MaszynaE3DRenderingServer::instance_get_model_filename);
        ClassDB::bind_method(
                D_METHOD("instance_get_skins", "instance"), &MaszynaE3DRenderingServer::instance_get_skins);
        ClassDB::bind_method(
                D_METHOD("instance_set_light_blink", "instance", "light", "on_time", "off_time", "phase"),
                &MaszynaE3DRenderingServer::instance_set_light_blink);
        ClassDB::bind_method(
                D_METHOD("emission_light_create", "instance", "light_name"),
                &MaszynaE3DRenderingServer::emission_light_create);
        ClassDB::bind_method(
                D_METHOD("spot_light_create", "instance", "light_name", "submodel_path"),
                &MaszynaE3DRenderingServer::spot_light_create);
        ClassDB::bind_method(
                D_METHOD("omni_light_create", "instance", "light_name", "submodel_path"),
                &MaszynaE3DRenderingServer::omni_light_create);
        ClassDB::bind_method(D_METHOD("light_free", "light"), &MaszynaE3DRenderingServer::light_free);
        ClassDB::bind_method(D_METHOD("light_enable", "light"), &MaszynaE3DRenderingServer::light_enable);
        ClassDB::bind_method(D_METHOD("light_disable", "light"), &MaszynaE3DRenderingServer::light_disable);
        ClassDB::bind_method(D_METHOD("light_get_statistics"), &MaszynaE3DRenderingServer::light_get_statistics);
        ClassDB::bind_method(
                D_METHOD("instance_set_smoke_intensity", "instance", "intensity"),
                &MaszynaE3DRenderingServer::instance_set_smoke_intensity);
        ClassDB::bind_method(
                D_METHOD("environment_set_time", "hours"), &MaszynaE3DRenderingServer::environment_set_time);
        ClassDB::bind_method(
                D_METHOD("environment_set_light_level", "level"),
                &MaszynaE3DRenderingServer::environment_set_light_level);
        ClassDB::bind_method(
                D_METHOD("material_set_resolver", "material_resolver"),
                &MaszynaE3DRenderingServer::material_set_resolver);
        ClassDB::bind_method(
                D_METHOD("model_set_loader", "model_loader"), &MaszynaE3DRenderingServer::model_set_loader);
        ClassDB::bind_method(
                D_METHOD("material_set_preloader", "material_preloader"),
                &MaszynaE3DRenderingServer::material_set_preloader);
        ClassDB::bind_method(
                D_METHOD("smoke_set_source_resolver", "smoke_source_resolver"),
                &MaszynaE3DRenderingServer::smoke_set_source_resolver);

        BIND_ENUM_CONSTANT(TRANSLUCENCY_CUTOUT);
        BIND_ENUM_CONSTANT(TRANSLUCENCY_BLENDED);
        BIND_ENUM_CONSTANT(TRANSLUCENCY_OPAQUE);

        BIND_ENUM_CONSTANT(LIGHT_MODE_OFF);
        BIND_ENUM_CONSTANT(LIGHT_MODE_ON);
        BIND_ENUM_CONSTANT(LIGHT_MODE_BLINK);
        BIND_ENUM_CONSTANT(LIGHT_MODE_DARK);
        BIND_ENUM_CONSTANT(LIGHT_MODE_HOME);

        ADD_SIGNAL(MethodInfo(
                instance_submodel_animation_finished_signal, PropertyInfo(Variant::RID, "instance"),
                PropertyInfo(Variant::STRING, "submodel")));
    }

    /// The settings a scenery light's placement is made of (MaszynaE3DLightFactory::discover()) and the
    /// range it streams with
    static const char *const LIGHT_PLACEMENT_SETTING_NAMES[] = {
            MaszynaE3DLightFactory::LIGHT_MODE_SETTING,
            MaszynaE3DLightFactory::ECONOMY_HEIGHT_OFFSET_SETTING,
            MaszynaE3DLightFactory::ECONOMY_CONE_SCALE_SETTING,
            MaszynaE3DLightFactory::LIGHT_SIZE_SETTING,
            MaszynaE3DLightFactory::LAMP_LIGHT_HEIGHT_OFFSET_SETTING,
            MaszynaE3DLightFactory::LAMP_LIGHT_CONE_SCALE_SETTING,
            MaszynaE3DLightFactory::LAMP_LIGHT_ATTENUATION_SETTING,
            MaszynaE3DRenderingServer::SCENERY_LIGHT_DISTANCE_SETTING,
    };
    /// ...and the settings its RenderingServer light is built with (_light_build())
    static const char *const LIGHT_BUILD_SETTING_NAMES[] = {
            MaszynaE3DRenderingServer::SCENERY_LIGHT_ENERGY_SETTING,
            MaszynaE3DRenderingServer::SCENERY_LIGHT_SHADOWS_SETTING,
            MaszynaE3DRenderingServer::SCENERY_LIGHT_TINT_SETTING,
            MaszynaE3DRenderingServer::SCENERY_LIGHT_VOLUMETRIC_FOG_ENERGY_SETTING,
            MaszynaE3DLightFactory::LIGHTS_SHADOW_REVERSE_CULL_FACE_SETTING,
    };
    template<size_t COUNT>
    Array read_settings(const char *const (&p_names)[COUNT]) {
        const ProjectSettings *settings = ProjectSettings::get_singleton();
        Array values;
        for (const char *name: p_names) {
            values.push_back(settings->get_setting(name));
        }
        return values;
    }

    /// What is read from the game's data is read again when the data is (GameDataServer), and what
    /// the settings shape is built again when they change
    MaszynaE3DRenderingServer *MaszynaE3DRenderingServer::get_instance() {
        return singleton;
    }

    MaszynaE3DRenderingServer::MaszynaE3DRenderingServer() : time(Time::get_singleton()) {
        singleton = this;
        light_placement_settings = read_settings(LIGHT_PLACEMENT_SETTING_NAMES);
        light_build_settings = read_settings(LIGHT_BUILD_SETTING_NAMES);
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &MaszynaE3DRenderingServer::_on_project_settings_changed));
        GameDataServer *game_data = GameDataServer::get_instance();
        ERR_FAIL_NULL(game_data);
        game_data->connect(
                GameDataServer::data_unload_requested_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::_on_data_unload_requested));
        game_data->connect(
                GameDataServer::data_reload_requested_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::_on_data_reload_requested));
        // created before the plugins' servers (register_types.cpp): the lamps of the signal heads drawn here
        SignallingServer *signalling = SignallingServer::get_instance();
        ERR_FAIL_NULL(signalling);
        signalling->connect(
                SignallingServer::signal_head_light_switched_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::_on_signal_head_light_switched));
        // created before the plugins' servers too: the events the animations queue
        ScenarioEventServer *scenario_events = ScenarioEventServer::get_instance();
        ERR_FAIL_NULL(scenario_events);
        scenario_events->connect(
                ScenarioEventServer::event_freed_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::_on_event_freed));
        // the environment is SimulationServer's: its light level and its hour
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        simulation->connect(
                SimulationServer::light_level_changed_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::environment_set_light_level));
        simulation->connect(
                SimulationServer::time_of_day_hour_changed_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::_on_time_of_day_hour_changed));
        environment_set_light_level(simulation->get_light_level());
        _on_time_of_day_hour_changed();
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        // the streamed scenery's models and their lights, each with a range of its own;
        // owners live as long as the streaming does
        stream_owner = streaming->owner_create(
                "models", callable_mp(this, &MaszynaE3DRenderingServer::_stream_preload),
                callable_mp(this, &MaszynaE3DRenderingServer::_stream_build),
                callable_mp(this, &MaszynaE3DRenderingServer::_stream_clear));
        light_stream_owner = streaming->owner_create(
                "model lights", Callable(), callable_mp(this, &MaszynaE3DRenderingServer::_light_stream_build),
                callable_mp(this, &MaszynaE3DRenderingServer::_light_clear));
    }

    /// A change of a placement setting places the scenery lights anew - the lit submodels the
    /// backends switch stay as they are, only the real lights are made again; a change of a build
    /// setting makes again the RenderingServer lights there are now
    void MaszynaE3DRenderingServer::_on_project_settings_changed() {
        const Array placement = read_settings(LIGHT_PLACEMENT_SETTING_NAMES);
        const Array build = read_settings(LIGHT_BUILD_SETTING_NAMES);
        if (placement != light_placement_settings) {
            light_placement_settings = placement;
            light_build_settings = build;
            for (KeyValue<RID, MaszynaE3DInstanceData> &item: instances) {
                MaszynaE3DInstanceData &instance = item.value;
                // the instances _build_instance_lights() gives lights to
                if (!instance.built || instance.instancer != ModelRenderingServer::INSTANCER_OPTIMIZED ||
                    !instance.stream_rid.is_valid()) {
                    continue;
                }
                _clear_instance_lights(instance);
                instance.model_lights.placements =
                        MaszynaE3DLightFactory::discover(instance.model, instance.model_filename).placements;
                _build_instance_lights(item.key, instance);
            }
        } else if (build != light_build_settings) {
            light_build_settings = build;
            for (KeyValue<RID, LightObject> &item: lights) {
                if (item.value.light.is_valid()) {
                    _light_clear(item.key);
                    _light_build(item.key);
                }
            }
            // the spotlights the NODES backend made of a vehicle's free spotlights (MaszynaE3DNodesBackend)
            const bool reverse_cull_face = ProjectSettings::get_singleton()->get_setting(
                    MaszynaE3DLightFactory::LIGHTS_SHADOW_REVERSE_CULL_FACE_SETTING, false);
            for (const KeyValue<RID, MaszynaE3DInstanceData> &item: instances) {
                Node *root = Object::cast_to<Node>(ObjectDB::get_instance(item.value.node_id));
                if (!item.value.built || item.value.instancer != ModelRenderingServer::INSTANCER_NODES ||
                    root == nullptr) {
                    continue;
                }
                const TypedArray<Node> spotlights = root->find_children("*", "SpotLight3D", true, false);
                for (int64_t index = 0; index < spotlights.size(); index++) {
                    Object::cast_to<SpotLight3D>(spotlights[index])->set_shadow_reverse_cull_face(reverse_cull_face);
                }
            }
        }
    }

    void MaszynaE3DRenderingServer::smoke_rebuild() {
        for (KeyValue<RID, MaszynaE3DInstanceData> &item: instances) {
            if (item.value.built) {
                _clear_instance_smoke_sources(item.value);
                _build_instance_smoke_sources(item.value);
            }
        }
    }

    /// The materials are the old data's - an instance built again asks for them anew; the models
    /// are let go by ResourceLazyLoader
    void MaszynaE3DRenderingServer::_on_data_unload_requested() {
        material_resolver.clear();
    }

    /// A scenery placement is built again as it is streamed, loading its model on the way; an
    /// instance created with its model is its client's to build again
    void MaszynaE3DRenderingServer::_on_data_reload_requested() {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        if (streaming != nullptr) {
            streaming->owner_rebuild(stream_owner);
        }
    }

    MaszynaE3DRenderingServer::~MaszynaE3DRenderingServer() {
        // Nodes built by the NODES backends belong to the scene tree, only RenderingServer RIDs are freed here
        for (KeyValue<RID, LightObject> &light: lights) {
            _light_clear(light.key);
        }
        lights.clear();
        _set_light_processing(false);
        _set_animation_processing(false);
        for (KeyValue<RID, MaszynaE3DInstanceData> &item: instances) {
            item.value.light_objects.clear();
            _clear_instance_smoke_sources(item.value);
            if (item.value.instancer == ModelRenderingServer::INSTANCER_OPTIMIZED) {
                optimized_backend.clear(item.value);
            }
        }
        instances.clear();
        singleton = nullptr;
    }

    MaszynaE3DInstanceBackend &MaszynaE3DRenderingServer::_get_backend(const MaszynaE3DInstanceData &p_instance) {
        return const_cast<MaszynaE3DInstanceBackend &>(
                static_cast<const MaszynaE3DRenderingServer *>(this)->_get_backend(p_instance));
    }

    const MaszynaE3DInstanceBackend &
    MaszynaE3DRenderingServer::_get_backend(const MaszynaE3DInstanceData &p_instance) const {
        switch (p_instance.instancer) {
            case ModelRenderingServer::INSTANCER_NODES:
                return nodes_backend;
            case ModelRenderingServer::INSTANCER_EDITABLE_NODES:
                return editable_nodes_backend;
            default:
                return optimized_backend;
        }
    }

    void MaszynaE3DRenderingServer::_rebuild_if_built(MaszynaE3DInstanceData &p_instance) {
        if (p_instance.built) {
            MaszynaE3DInstanceBackend &backend = _get_backend(p_instance);
            backend.clear(p_instance);
            backend.build(p_instance, material_resolver);
            _apply_client_submodels(p_instance);
        }
    }

    void MaszynaE3DRenderingServer::_update_if_built(MaszynaE3DInstanceData &p_instance) {
        if (p_instance.built) {
            _get_backend(p_instance).update(p_instance);
        }
    }

    void MaszynaE3DRenderingServer::_update_visibility_if_built(MaszynaE3DInstanceData &p_instance) {
        if (p_instance.built) {
            _get_backend(p_instance).update_visibility(p_instance);
        }
    }

    /// Creates an empty instance; set it up with instance_set_*() and
    /// instance_attach_object_instance_id(), then call instance_build().
    ///
    /// The kind is given here rather than through a setter because it cannot be changed once the
    /// instance is built: the smoke density it selects is baked into every emitter - its process
    /// material, its particle budget and its spawn rate. A client that has to change it frees the
    /// instance and creates it again, which is what MaszynaE3DModelInstance's own _dirty does.
    RID MaszynaE3DRenderingServer::instance_create(
            const Ref<Resource> &p_model, const Instancer p_instancer, const InstanceKind p_instance_kind) {
        ERR_FAIL_COND_V(p_model.is_null(), RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        MaszynaE3DInstanceData &instance = instances[rid];
        instance.model = p_model;
        instance.instancer = p_instancer;
        instance.instance_kind = p_instance_kind;
        // a vehicle's plume follows its engine, so it stays silent until the vehicle has said how
        // much - a default rate would spawn a puff from an engine that is off
        instance.smoke_intensity = p_instance_kind == ModelRenderingServer::INSTANCE_KIND_DYNAMIC ? 0.0 : 1.0;
        emit_signal(instance_created_signal, rid);
        return rid;
    }

    void MaszynaE3DRenderingServer::instance_free(const RID &p_instance) {
        MaszynaE3DInstanceData *found = instances.getptr(p_instance);
        ERR_FAIL_NULL(found);
        // Taken out of the registry before anything else runs: freeing a stream, a light or a
        // smoke source re-enters this server, and an instance created or freed in between
        // rehashes `instances` - which would leave this holding a dead entry. Cold caches make
        // that overlap routine, because a vehicle is still building models while a scenery is
        // being torn down.
        const MaszynaE3DInstanceData data = *found;
        instances.erase(p_instance);
        built_instances.erase(p_instance);

        if (data.stream_rid.is_valid()) {
            if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
                streaming->stream_free(data.stream_rid);
            }
            RID model_resource;
            {
                MutexLock lock(models_mutex);
                model_resource = stream_models[p_instance].resource;
                stream_models.erase(p_instance);
            }
            if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
                if (data.built) {
                    lazy_loader->resource_release(model_resource);
                }
                lazy_loader->resource_free(model_resource);
            }
        }
        if (blinking_instances.has(p_instance)) {
            blinking_instances.erase(p_instance);
            _set_light_processing(!blinking_instances.is_empty());
        }
        if (animating_instances.has(p_instance)) {
            animating_instances.erase(p_instance);
            _set_animation_processing(!animating_instances.is_empty());
        }
        MaszynaE3DInstanceData clearing = data;
        _clear_instance_lights(clearing);
        _clear_instance_smoke_sources(clearing);
        if (clearing.built) {
            _get_backend(clearing).clear(clearing);
        }
        for (const KeyValue<String, RID> &done: data.submodel_done_events) {
            done_event_instances.erase(done.value);
        }
        // the signal head is the lights this instance drew, so it goes with it
        if (data.signal_head.is_valid()) {
            signal_head_instances.erase(data.signal_head);
            if (SignallingServer *signalling = SignallingServer::get_instance(); signalling != nullptr) {
                signalling->signal_head_free(data.signal_head);
            }
        }
        emit_signal(instance_freed_signal, p_instance);
    }

    /// Builds (or rebuilds) the instance content. Later changes of options, attached node and
    /// scenario rebuild it again, other setters only update it.
    void MaszynaE3DRenderingServer::instance_build(const RID &p_instance) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        MaszynaE3DInstanceBackend &backend = _get_backend(*instance);
        if (instance->built) {
            _clear_instance_lights(*instance);
            _clear_instance_smoke_sources(*instance);
            backend.clear(*instance);
            instance->built = false;
        }
        // Identifying the model's lights is neither instancer's job - both only render what this
        // lists. Resolved before the build too, because the backend applies lights_state as it
        // builds.
        instance->model_lights = MaszynaE3DLightFactory::discover(instance->model, instance->model_filename);
        _resolve_lights(*instance);
        instance->built = true;
        built_instances.insert(p_instance);
        backend.build(*instance, material_resolver);
        // the model may be a new one after streaming, so the animated submodels are found again -
        // and the poses kept by the old ones' pointers go
        instance->submodel_poses.clear();
        for (KeyValue<String, MaszynaE3DInstanceData::SubmodelAnimation> &animation: instance->submodel_animations) {
            animation.value.submodel = _find_submodel(instance->model->get_submodels(), animation.key);
        }
        _apply_client_submodels(*instance);
        _build_instance_lights(p_instance, *instance);
        _build_instance_smoke_sources(*instance);
        // the model is known once the instance is built: its lights are the signal head's
        if (instance->signal_head.is_valid()) {
            SignallingServer *signalling = SignallingServer::get_instance();
            ERR_FAIL_NULL(signalling);
            signalling->signal_head_set_light_count(instance->signal_head, instance_get_light_count(p_instance));
        }
        emit_signal(instance_built_signal, p_instance);
    }

    void MaszynaE3DRenderingServer::instance_set_options(
            const RID &p_instance, const String &p_data_path, const PackedStringArray &p_skins,
            const Array &p_exclude_node_names, const bool p_force_alpha,
            const TypedArray<NodePath> &p_force_alpha_part_paths, const int p_max_texture_size) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->data_path = p_data_path;
        instance->skins = p_skins;
        instance->exclude_node_names = p_exclude_node_names;
        instance->force_alpha = p_force_alpha;
        instance->force_alpha_submodel_paths = p_force_alpha_part_paths;
        instance->max_texture_size = p_max_texture_size;
        _rebuild_if_built(*instance);
    }

    /// NODES/EDITABLE_NODES build their node tree under the Node3D of [param p_id]; OPTIMIZED
    /// instances report it as their owner (e.g. for picking in the editor). 0 detaches it.
    void MaszynaE3DRenderingServer::instance_attach_object_instance_id(const RID &p_instance, const uint64_t p_id) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->node_id = ObjectID(p_id);
        _rebuild_if_built(*instance);
    }

    uint64_t MaszynaE3DRenderingServer::instance_get_attached_node(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, 0);
        return ObjectDB::get_instance(instance->node_id) != nullptr ? static_cast<uint64_t>(instance->node_id) : 0;
    }

    void MaszynaE3DRenderingServer::instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->node_transform = p_transform;
        _rebuild_if_built(*instance);
    }

    void MaszynaE3DRenderingServer::instance_set_scenario(const RID &p_instance, const RID &p_scenario) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->scenario = p_scenario;
        if (!instance->built) {
            return;
        }
        // the RenderingServer instances are moved, not built again - a node leaving its world
        // (another scene's tab in the editor) and entering it again keeps its model, as
        // VisualInstance3D does; a NODES tree follows its node by itself
        RenderingServer *rs = RenderingServer::get_singleton();
        for (const RID &rid: instance->rids) {
            rs->instance_set_scenario(rid, p_scenario);
        }
        for (const RID &light_rid: instance->light_objects) {
            if (const LightObject *light = lights.getptr(light_rid);
                light != nullptr && light->light_instance.is_valid()) {
                rs->instance_set_scenario(light->light_instance, p_scenario);
            }
        }
        if (SmokeServer *smoke = SmokeServer::get_instance(); smoke != nullptr) {
            for (const MaszynaE3DInstanceData::SmokeEmitter &emitter: instance->smoke_emitters) {
                smoke->smoke_set_scenario(emitter.rid, p_scenario);
            }
        }
    }

    /// Global transform of an OPTIMIZED instance (a NODES tree follows its attached node)
    void MaszynaE3DRenderingServer::instance_set_transform(const RID &p_instance, const Transform3D &p_transform) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->transform = p_transform;
        if (instance->built) {
            _get_backend(*instance).apply_transform(*instance);
        }
        _update_instance_smoke(*instance);
    }

    void MaszynaE3DRenderingServer::instance_set_visible(const RID &p_instance, const bool p_visible) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->visible = p_visible;
        _update_visibility_if_built(*instance);
        _update_instance_smoke(*instance);
    }

    void MaszynaE3DRenderingServer::instance_set_layer_mask(const RID &p_instance, const uint32_t p_mask) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->layer_mask = p_mask;
        _update_if_built(*instance);
    }

    /// Limits the visibility range of all submodels of an OPTIMIZED instance (0 - no limit)
    void MaszynaE3DRenderingServer::instance_set_visibility_range(
            const RID &p_instance, const float p_begin, const float p_end) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->visibility_range_begin = p_begin;
        instance->visibility_range_end = p_end;
        _rebuild_if_built(*instance);
    }

    /// A material drawn over every submodel of the instance (an outline, a tint); null for none
    void
    MaszynaE3DRenderingServer::instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->material_overlay = p_material;
        _update_if_built(*instance);
    }

    /// Where the segment (world space) first meets the instance's meshes: `{position, distance}`
    /// (the distance from `p_from`), empty when it misses or the instance is hidden or not built
    Dictionary MaszynaE3DRenderingServer::instance_intersect_segment(
            const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const {
        Dictionary hit;
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, hit);
        if (!instance->built || !instance->visible) {
            return hit;
        }
        double distance = p_from.distance_to(p_to);
        Vector3 point;
        if (_get_backend(*instance).intersect_segment(*instance, p_from, p_to, distance, point)) {
            hit["position"] = point;
            hit["distance"] = distance;
        }
        return hit;
    }

    /// Light name -> enabled; shows the "on" or "off" submodels of the model's lights. A value set
    /// here is a manual override and wins over the mode the scenery node declared.
    /// A light the model registers (MaszynaE3DModel::get_lights())
    bool MaszynaE3DRenderingServer::instance_has_light(const RID &p_instance, const String &p_light) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, false);
        return instance->model.is_valid() && instance->model->get_lights().has(p_light);
    }

    void MaszynaE3DRenderingServer::instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->lights_override = p_lights_state.duplicate();
        _resolve_lights(*instance);
        _update_visibility_if_built(*instance);
    }

    /// Light name -> dimmed: a dimmed light shows its "_xon" submodel instead of "_on" when it has
    /// one (TButton::TurnxOnWithOnAsFallback(), DynObj.cpp:1218), and its real light shines at
    /// `p_multiplier` of its energy (the vehicle's DimmedMultiplier, lightarray.cpp:77-78)
    void MaszynaE3DRenderingServer::instance_set_lights_dimmed(
            const RID &p_instance, const Dictionary &p_lights_dimmed, const float p_multiplier) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->lights_dimmed = p_lights_dimmed.duplicate();
        instance->lights_dimmed_multiplier = p_multiplier;
        _update_visibility_if_built(*instance);
    }

    /// The `lights` list of a scenery model node, by light index: `lights 3` means light 0 is
    /// LIGHT_MODE_DARK. The index maps to the name the E3D parser gave the light_onNN submodel
    /// pair, exactly as the original binds Light_On00..07 by slot (AnimModel.cpp:303-317).
    void
    MaszynaE3DRenderingServer::instance_set_lights_modes(const RID &p_instance, const PackedFloat32Array &p_modes) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        for (int i = 0; i < p_modes.size(); i++) {
            const MaszynaLightMode parsed = MaszynaLightMode::parse(p_modes[i]);
            MaszynaE3DInstanceData::LightDeclaration &declaration =
                    instance->light_declarations[_light_name_for_index(i)];
            declaration.mode = parsed.mode;
            declaration.threshold = parsed.threshold;
            declaration.on_time = parsed.on_time;
            declaration.off_time = parsed.off_time;
            declaration.phase = parsed.phase;
        }
        _update_blinking(p_instance, *instance);
        _resolve_lights(*instance);
        _update_visibility_if_built(*instance);
    }

    /// One light by index, what `LightSet()` does for a `lights` event (AnimModel.cpp:664-670).
    /// A LIGHT_MODE_BLINK set here blinks with the default times; see instance_set_light_blink().
    /// The newest command wins: it drops a manual override of the same light, as LightSet()
    /// replaces whatever the light was set to before.
    void MaszynaE3DRenderingServer::instance_set_light_mode(
            const RID &p_instance, const int p_light, const LightMode p_mode) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const String light_name = _light_name_for_index(p_light);
        instance->lights_override.erase(light_name);
        MaszynaE3DInstanceData::LightDeclaration &declaration = instance->light_declarations[light_name];
        declaration.mode = p_mode;
        declaration.threshold = 0.0;
        declaration.on_time = MaszynaLightMode::DEFAULT_ON_TIME;
        declaration.off_time = MaszynaLightMode::DEFAULT_OFF_TIME;
        declaration.phase = 0.0;
        _update_blinking(p_instance, *instance);
        _resolve_lights(*instance);
        _update_visibility_if_built(*instance);
    }

    /// The pick pass draws only opaque submodels (opengl33renderer.cpp:1208 Render_cab(..., Alpha =
    /// false), 3674 iAlpha & iFlags & 0x1F): a translucent one (flag 0x20) hides nothing
    PackedInt64Array MaszynaE3DRenderingServer::instance_get_opaque_meshes(const RID &p_instance) const {
        PackedInt64Array meshes;
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, meshes);
        for (const KeyValue<MaszynaE3DSubModel *, ObjectID> &submodel_node: instance->submodel_nodes) {
            if (submodel_node.key->get_submodel_type() == MaszynaE3DSubModel::SUBMODEL_GL_TRIANGLES &&
                !submodel_node.key->get_material_transparent()) {
                meshes.push_back(static_cast<int64_t>(static_cast<uint64_t>(submodel_node.value)));
            }
        }
        return meshes;
    }

    /// TAnimModel::iNumLights (AnimModel.cpp:327-329): the highest index a light_onNN or
    /// light_offNN pair has, plus one
    int MaszynaE3DRenderingServer::instance_get_light_count(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, 0);
        int count = 0;
        for (const MaszynaE3DModelLight &light: instance->model_lights.lights) {
            if (light.name.is_valid_int()) {
                count = MAX(count, static_cast<int>(light.name.to_int()) + 1);
            }
        }
        return count;
    }

    void MaszynaE3DRenderingServer::instance_attach_signal_head(const RID &p_instance, const RID &p_signal_head) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        ERR_FAIL_COND_MSG(instance->signal_head.is_valid(), "The instance already draws a signal head.");
        SignallingServer *signalling = SignallingServer::get_instance();
        ERR_FAIL_NULL(signalling);
        instance->signal_head = p_signal_head;
        signal_head_instances.insert(p_signal_head, p_instance);
        signalling->signal_head_set_light_count(p_signal_head, instance_get_light_count(p_instance));
    }

    RID MaszynaE3DRenderingServer::signal_head_get_instance(const RID &p_signal_head) const {
        const RID *instance = signal_head_instances.getptr(p_signal_head);
        return instance != nullptr ? *instance : RID();
    }

    void MaszynaE3DRenderingServer::instance_set_submodel_done_event(
            const RID &p_instance, const String &p_submodel, const RID &p_event) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const String submodel = p_submodel.to_lower();
        if (const RID *previous = instance->submodel_done_events.getptr(submodel); previous != nullptr) {
            done_event_instances.erase(*previous);
        }
        instance->submodel_done_events[submodel] = p_event;
        done_event_instances[p_event] = p_instance;
    }

    void MaszynaE3DRenderingServer::_on_event_freed(const RID &p_event) {
        const RID *instance_rid = done_event_instances.getptr(p_event);
        if (instance_rid == nullptr) {
            return;
        }
        if (MaszynaE3DInstanceData *instance = instances.getptr(*instance_rid); instance != nullptr) {
            String submodel;
            for (const KeyValue<String, RID> &done: instance->submodel_done_events) {
                if (done.value == p_event) {
                    submodel = done.key;
                    break;
                }
            }
            instance->submodel_done_events.erase(submodel);
        }
        done_event_instances.erase(p_event);
    }

    void MaszynaE3DRenderingServer::_on_signal_head_light_switched(
            const RID &p_signal_head, const int p_light, const int p_state) {
        const RID *instance = signal_head_instances.getptr(p_signal_head);
        if (instance == nullptr) {
            return;
        }
        switch (p_state) {
            case SignallingServer::LIGHT_STATE_ON:
                instance_set_light_mode(*instance, p_light, LIGHT_MODE_ON);
                break;
            case SignallingServer::LIGHT_STATE_OFF:
                instance_set_light_mode(*instance, p_light, LIGHT_MODE_OFF);
                break;
            case SignallingServer::LIGHT_STATE_BLINKING: {
                const SignallingServer *signalling = SignallingServer::get_instance();
                ERR_FAIL_NULL(signalling);
                const Dictionary cycle = signalling->signal_head_get_light_blink(p_signal_head, p_light);
                instance_set_light_blink(*instance, p_light, cycle["on_time"], cycle["off_time"], cycle["phase"]);
            } break;
            default:
                break;
        }
    }

    /// One light by index, blinking: on for p_on_time seconds, off for p_off_time, the cycle
    /// shifted by p_phase seconds. Drops a manual override of the light, as above.
    void MaszynaE3DRenderingServer::instance_set_light_blink(
            const RID &p_instance, const int p_light, const float p_on_time, const float p_off_time,
            const float p_phase) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        ERR_FAIL_COND(p_on_time + p_off_time <= 0.0f);
        const String light_name = _light_name_for_index(p_light);
        instance->lights_override.erase(light_name);
        MaszynaE3DInstanceData::LightDeclaration &declaration = instance->light_declarations[light_name];
        declaration.mode = LIGHT_MODE_BLINK;
        declaration.threshold = 0.0;
        declaration.on_time = p_on_time;
        declaration.off_time = p_off_time;
        declaration.phase = p_phase;
        _update_blinking(p_instance, *instance);
        _resolve_lights(*instance);
        _update_visibility_if_built(*instance);
    }

    /// The colours of the lights, by name - a scenery model node's `lightcolors` (its light 0 "00"). A
    /// colour overrides the one the light submodel carries (SetDiffuseOverride(), AnimModel.cpp:625).
    void
    MaszynaE3DRenderingServer::instance_set_lights_colors(const RID &p_instance, const Dictionary &p_lights_colors) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const Array names = p_lights_colors.keys();
        for (int i = 0; i < names.size(); i++) {
            MaszynaE3DInstanceData::LightDeclaration &declaration = instance->light_declarations[String(names[i])];
            declaration.color = p_lights_colors[names[i]];
            declaration.has_color = true;
        }
        _rebuild_if_built(*instance); // the colour is applied when the light is created
    }

    String MaszynaE3DRenderingServer::light_get_name(const int p_index) {
        return _light_name_for_index(p_index);
    }

    /// `material_resolver(submodel: MaszynaE3DSubModel, data_path: String, skins: PackedStringArray,
    /// translucency: Translucency) -> Material`, used by instance_build()
    void MaszynaE3DRenderingServer::material_set_resolver(const Callable &p_material_resolver) {
        material_resolver.set_callable(p_material_resolver);
    }

    /// Registers a scenery model placement for streaming: nothing is built until the streaming
    /// camera comes within its visibility range of the chunk it falls into, and with lazy loading
    /// nothing is loaded until then either (ResourceLazyLoader). A range of
    /// 0 (a scenery node that declares none) means the global draw distance - see
    /// SceneryStreamingServer.
    RID MaszynaE3DRenderingServer::instance_register(
            const String &p_name, const PackedStringArray &p_skins, const Transform3D &p_transform,
            const float p_range_begin, const float p_range_end, const RID &p_scenario) {
        const String data_path = p_name.get_base_dir();
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL_V(streaming, RID());
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ERR_FAIL_NULL_V(lazy_loader, RID());
        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        MaszynaE3DInstanceData &instance = instances[rid];
        instance.instancer = ModelRenderingServer::INSTANCER_OPTIMIZED;
        instance.model_filename = p_name.get_file();
        instance.data_path = data_path;
        instance.skins = p_skins;
        instance.transform = p_transform;
        instance.visibility_range_begin = p_range_begin;
        instance.visibility_range_end = p_range_end;
        instance.scenario = p_scenario;
        // one resource per model file, shared by every placement of it
        const RID model_resource = lazy_loader->resource_register(
                p_name, callable_mp(this, &MaszynaE3DRenderingServer::model_load).bind(p_name));
        {
            MutexLock lock(models_mutex);
            stream_models[rid] = StreamModel{model_resource, data_path, p_skins};
        }
        instance.stream_rid = streaming->stream_register(stream_owner, rid, p_transform.origin, p_range_end);
        emit_signal(instance_created_signal, rid);
        return rid;
    }

    /// `model_loader(data_path: String, filename: String) -> MaszynaE3DModel`, called on the streaming
    /// worker thread for registered instances entering the camera's range
    void MaszynaE3DRenderingServer::model_set_loader(const Callable &p_model_loader) {
        MutexLock lock(models_mutex);
        model_loader = p_model_loader;
    }

    void MaszynaE3DRenderingServer::smoke_set_source_resolver(const Callable &p_smoke_source_resolver) {
        smoke_source_resolver = p_smoke_source_resolver;
    }

    /// Not memoized here: a model kept for the whole session is what filled the memory of a large
    /// scenery. The loader's own cache (ResourceLoader's) shares a model while anything holds it,
    /// and a streamed placement holds it through ResourceLazyLoader - with lazy loading only while
    /// it is built.
    /// A model's name is its path in the game's data without the extension - its directory and its file
    /// for the loader
    Ref<Resource> MaszynaE3DRenderingServer::model_load(const String &p_name) {
        Callable loader;
        {
            MutexLock lock(models_mutex);
            loader = model_loader;
        }
        return loader.is_valid() ? Ref<Resource>(loader.call(p_name.get_base_dir(), p_name.get_file()))
                                 : Ref<Resource>();
    }

    void MaszynaE3DRenderingServer::material_set_preloader(const Callable &p_material_preloader) {
        MutexLock lock(models_mutex);
        material_preloader = p_material_preloader;
    }

    Variant MaszynaE3DRenderingServer::material_preload(
            const Ref<Resource> &p_model, const String &p_data_path, const PackedStringArray &p_skins,
            const int p_max_texture_size) {
        Callable preloader;
        {
            MutexLock lock(models_mutex);
            preloader = material_preloader;
        }
        return preloader.is_valid() && p_model.is_valid()
                       ? preloader.call(p_model, p_data_path, p_skins, p_max_texture_size)
                       : Variant();
    }

    /// The model resource of a registered instance, from the copy made by instance_register()
    RID MaszynaE3DRenderingServer::_get_stream_model(const RID &p_instance) {
        MutexLock lock(models_mutex);
        const StreamModel *found = stream_models.getptr(p_instance);
        return found != nullptr ? found->resource : RID();
    }

    /// Streaming worker thread: loads the model and reads its materials' files, the build holds the
    /// model and makes the materials of what was read - the main thread reads no file
    /// (docs/findings-archive.md, 2026-10-02 streaming hitches). [model, what holds the materials' files]
    Variant MaszynaE3DRenderingServer::_stream_preload(const RID &p_instance) {
        StreamModel stream_model;
        {
            MutexLock lock(models_mutex);
            const StreamModel *found = stream_models.getptr(p_instance);
            if (found == nullptr) {
                return Variant();
            }
            stream_model = *found;
        }
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        if (lazy_loader == nullptr) {
            return Variant();
        }
        const Ref<MaszynaE3DModel> model = lazy_loader->resource_load(stream_model.resource);
        Array preloaded;
        preloaded.push_back(model);
        // a streamed placement draws with the project's texture size limit (instance_register())
        preloaded.push_back(material_preload(model, stream_model.data_path, stream_model.skins, 0));
        return preloaded;
    }

    void MaszynaE3DRenderingServer::_stream_build(const RID &p_instance, const Variant &p_preloaded) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        if (instance == nullptr) {
            return;
        }
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        // the materials' files the preload read are held by p_preloaded until the build is done
        const Array loaded = p_preloaded;
        const Ref<MaszynaE3DModel> preloaded =
                loaded.is_empty() ? Ref<MaszynaE3DModel>() : Ref<MaszynaE3DModel>(loaded[0]);
        if (preloaded.is_null() || lazy_loader == nullptr) {
            return; // the loader already reported why
        }
        // held while built; the copy another instance already holds, if there is one
        instance->model = lazy_loader->resource_hold(_get_stream_model(p_instance), preloaded);
        instance_build(p_instance);
    }

    void MaszynaE3DRenderingServer::_stream_clear(const RID &p_instance) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        if (instance == nullptr || !instance->built) {
            return;
        }
        _clear_instance_lights(*instance);
        _clear_instance_smoke_sources(*instance);
        _get_backend(*instance).clear(*instance);
        instance->built = false;
        built_instances.erase(p_instance);
        instance->model.unref();
        if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
            lazy_loader->resource_release(_get_stream_model(p_instance));
        }
    }

    RID MaszynaE3DRenderingServer::_light_create(
            const RID &p_instance, const String &p_light_name, const LightKind p_kind,
            const MaszynaE3DLightParams &p_params, const bool p_synthesized) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, RID());

        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        LightObject &light = lights[rid];
        light.owner = p_instance;
        light.kind = p_kind;
        light.light_name = p_light_name;
        light.params = p_params;
        light.synthesized = p_synthesized;
        light.enabled = instance->lights_state.get(p_light_name, false);
        instance->light_objects.push_back(rid);

        if (p_kind == LIGHT_KIND_EMISSION) {
            return rid; // the backends switch the on/off submodels from lights_state
        }

        // A scenery light is streamed with a range of its own, far shorter than the model's: a
        // street lamp is visible from half a kilometre and lights fifteen metres. Anything built
        // directly (a node, a vehicle) gets its RenderingServer light right away instead - it is
        // not part of the streamed scenery and may well be moving.
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        if (instance->stream_rid.is_valid() && streaming != nullptr) {
            const ProjectSettings *settings = ProjectSettings::get_singleton();
            const float distance =
                    settings->get_setting(SCENERY_LIGHT_DISTANCE_SETTING, DEFAULT_SCENERY_LIGHT_DISTANCE);
            const Vector3 position = (instance->transform * p_params.transform).origin;
            light.stream_rid = streaming->stream_register(light_stream_owner, rid, position, distance);
        } else {
            _light_build(rid);
        }
        return rid;
    }

    /// Creates the RenderingServer light of a spot/omni light object
    void MaszynaE3DRenderingServer::_light_build(const RID &p_light) {
        LightObject *light = lights.getptr(p_light);
        if (light == nullptr || light->kind == LIGHT_KIND_EMISSION || light->light.is_valid()) {
            return;
        }
        const MaszynaE3DInstanceData *instance = instances.getptr(light->owner);
        if (instance == nullptr) {
            return;
        }
        RenderingServer *rs = RenderingServer::get_singleton();
        ERR_FAIL_NULL(rs);

        const ProjectSettings *settings = ProjectSettings::get_singleton();
        const float energy_scale = settings->get_setting(SCENERY_LIGHT_ENERGY_SETTING, DEFAULT_SCENERY_LIGHT_ENERGY);
        const bool shadows = settings->get_setting(SCENERY_LIGHT_SHADOWS_SETTING, DEFAULT_SCENERY_LIGHT_SHADOWS);
        const float tint = settings->get_setting(SCENERY_LIGHT_TINT_SETTING, DEFAULT_SCENERY_LIGHT_TINT);
        const float fog_energy = settings->get_setting(
                SCENERY_LIGHT_VOLUMETRIC_FOG_ENERGY_SETTING, DEFAULT_SCENERY_LIGHT_VOLUMETRIC_FOG_ENERGY);

        light->light = light->kind == LIGHT_KIND_OMNI ? rs->omni_light_create() : rs->spot_light_create();
        rs->light_set_color(light->light, Color(1.0, 1.0, 1.0).lerp(light->params.color, tint));
        rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_ENERGY, light->params.energy * energy_scale);
        rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_RANGE, light->params.range);
        rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_ATTENUATION, light->params.attenuation);
        rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_SIZE, light->params.size);
        if (light->kind == LIGHT_KIND_SPOT) {
            rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_SPOT_ANGLE, light->params.spot_angle);
            rs->light_set_param(
                    light->light, RenderingServer::LIGHT_PARAM_SPOT_ATTENUATION, light->params.spot_attenuation);
        }
        rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_VOLUMETRIC_FOG_ENERGY, fog_energy);
        rs->light_set_shadow(light->light, shadows);
        if (shadows) {
            rs->light_set_shadow_caster_mask(light->light, ~SCENERY_LIGHT_OWNER_LAYER);
            rs->light_set_param(
                    light->light, RenderingServer::LIGHT_PARAM_SHADOW_BIAS,
                    light->kind == LIGHT_KIND_OMNI ? OMNI_LIGHT_SHADOW_BIAS : SPOT_LIGHT_SHADOW_BIAS);
            rs->light_set_param(
                    light->light, RenderingServer::LIGHT_PARAM_SHADOW_NORMAL_BIAS, LIGHT_SHADOW_NORMAL_BIAS);
            rs->light_set_param(light->light, RenderingServer::LIGHT_PARAM_SHADOW_BLUR, LIGHT_SHADOW_BLUR);
            // OmniLight3D's constructor (light_3d.cpp:663); the server starts with dual paraboloid
            if (light->kind == LIGHT_KIND_OMNI) {
                rs->light_omni_set_shadow_mode(light->light, RenderingServer::LIGHT_OMNI_SHADOW_CUBE);
            }
            // Must be set explicitly, like the biases above: a RenderingServer light does not get
            // it from Light3D's constructor and starts with it on, which stripes the ground with
            // shadow acne. The setting can bring back the original's front-face culling in shadow
            // maps (opengl33renderer.cpp:1758); off by default, like a Light3D.
            rs->light_set_reverse_cull_face_mode(
                    light->light,
                    settings->get_setting(MaszynaE3DLightFactory::LIGHTS_SHADOW_REVERSE_CULL_FACE_SETTING, false));
            // The light keeps reaching as far as it is streamed; only its shadow map stops early,
            // because a scenery puts 152 of these within 300 m
            const float distance =
                    settings->get_setting(SCENERY_LIGHT_DISTANCE_SETTING, DEFAULT_SCENERY_LIGHT_DISTANCE);
            rs->light_set_distance_fade(
                    light->light, true, distance, SCENERY_LIGHT_SHADOW_FADE_DISTANCE,
                    distance * SCENERY_LIGHT_FADE_LENGTH_SHARE);
        }

        light->light_instance = rs->instance_create();
        rs->instance_set_base(light->light_instance, light->light);
        rs->instance_set_scenario(light->light_instance, instance->scenario);
        rs->instance_set_transform(light->light_instance, instance->transform * light->params.transform);
        rs->instance_set_visible(light->light_instance, light->enabled);
        light->streamed_in = true;
    }

    /// The build callback of the light stream; separate from _light_build() only because
    /// SceneryStreamingServer passes the preloaded value along
    void MaszynaE3DRenderingServer::_light_stream_build(const RID &p_light, const Variant &p_preloaded) {
        _light_build(p_light);
    }

    void MaszynaE3DRenderingServer::_light_clear(const RID &p_light) {
        LightObject *light = lights.getptr(p_light);
        if (light == nullptr) {
            return;
        }
        light->streamed_in = false;
        RenderingServer *rs = RenderingServer::get_singleton();
        if (rs == nullptr) {
            return;
        }
        if (light->light_instance.is_valid()) {
            rs->free_rid(light->light_instance);
            light->light_instance = RID();
        }
        if (light->light.is_valid()) {
            rs->free_rid(light->light);
            light->light = RID();
        }
    }

    void MaszynaE3DRenderingServer::_light_apply_enabled(LightObject &p_light) {
        if (!p_light.light_instance.is_valid()) {
            return;
        }
        RenderingServer *rs = RenderingServer::get_singleton();
        ERR_FAIL_NULL(rs);
        rs->instance_set_visible(p_light.light_instance, p_light.enabled);
    }

    /// Creates the light objects of a freshly built instance out of the lights MaszynaE3DLightFactory
    /// found in the model
    void
    MaszynaE3DRenderingServer::_build_instance_lights(const RID &p_instance, MaszynaE3DInstanceData &p_instance_data) {
        // The NODES backends build SpotLight3D nodes of their own. A vehicle far enough away to
        // have switched to OPTIMIZED is past the distance where those were faded out anyway
        // (reloaded/vehicles/detail_distance), so only the streamed scenery gets lights
        // here - the ones a caller asks for by hand still go through *_light_create().
        if (p_instance_data.instancer != ModelRenderingServer::INSTANCER_OPTIMIZED ||
            !p_instance_data.stream_rid.is_valid()) {
            return;
        }

        for (const MaszynaE3DModelLightPlacement &placement: p_instance_data.model_lights.placements) {
            MaszynaE3DLightParams params = placement.params;
            _apply_declared_color(p_instance_data, placement.light_name, params);
            _light_create(
                    p_instance, placement.light_name, params.omni ? LIGHT_KIND_OMNI : LIGHT_KIND_SPOT, params,
                    placement.synthesized);
        }
        // The model now owns a light, so keep its own geometry out of every scenery shadow map.
        // Only this layer: Godot casts when (layer_mask & shadow_caster_mask) is non-zero
        // (renderer_scene_cull.cpp:2427), so any layer kept beside it would still cast.
        if (!p_instance_data.light_objects.is_empty() && p_instance_data.layer_mask != SCENERY_LIGHT_OWNER_LAYER) {
            p_instance_data.layer_mask = SCENERY_LIGHT_OWNER_LAYER;
            _update_if_built(p_instance_data);
        }
    }

    /// `lightcolors` of the scenery node overrides the colour the model carries
    void MaszynaE3DRenderingServer::_apply_declared_color(
            const MaszynaE3DInstanceData &p_instance_data, const String &p_light_name,
            MaszynaE3DLightParams &p_params) {
        const MaszynaE3DInstanceData::LightDeclaration *declaration =
                p_instance_data.light_declarations.getptr(p_light_name);
        if (declaration != nullptr && declaration->has_color) {
            p_params.color = declaration->color;
            p_params.color.a = 1.0;
        }
    }

    void MaszynaE3DRenderingServer::_clear_instance_lights(MaszynaE3DInstanceData &p_instance_data) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        for (const RID &light_rid: p_instance_data.light_objects) {
            LightObject *light = lights.getptr(light_rid);
            if (light == nullptr) {
                continue;
            }
            if (light->stream_rid.is_valid() && streaming != nullptr) {
                streaming->stream_free(light->stream_rid);
            }
            _light_clear(light_rid);
            lights.erase(light_rid);
        }
        p_instance_data.light_objects.clear();
    }

    /// Creates the emitters of a freshly built instance out of what MaszynaE3DSmokeSourceFactory found
    /// in the model, through SmokeServer. Unlike the lights this runs for every instancer: a distant
    /// vehicle has no node tree left, and the OPTIMIZED backend renders no emitter of its own.
    void MaszynaE3DRenderingServer::_build_instance_smoke_sources(MaszynaE3DInstanceData &p_instance_data) {
        SmokeServer *smoke = SmokeServer::get_instance();
        ERR_FAIL_NULL(smoke);
        if (!smoke_source_resolver.is_valid()) {
            return;
        }
        const Vector<MaszynaE3DSmokeSourcePlacement> placements =
                MaszynaE3DSmokeSourceFactory::discover(p_instance_data.model);
        // a locomotive's plume and a chimney's are seen from different distances and made of
        // different numbers, so each kind has its own
        const SmokeServer::Kind kind = p_instance_data.instance_kind == ModelRenderingServer::INSTANCE_KIND_DYNAMIC
                                               ? SmokeServer::KIND_DYNAMIC
                                               : SmokeServer::KIND_STATIC;
        for (const MaszynaE3DSmokeSourcePlacement &placement: placements) {
            const Dictionary resolved = smoke_source_resolver.call(placement.template_name);
            const Transform3D transform = _smoke_transform(p_instance_data, placement.offset);
            // A scenery emitter streams with a range of its own, the way a scenery light does.
            // Anything built directly (a vehicle, an editor model) is built right away - it is not
            // part of the streamed scenery and is usually moving.
            const RID rid = p_instance_data.stream_rid.is_valid() ? smoke->smoke_register(kind, transform)
                                                                  : smoke->smoke_create(kind);
            smoke->smoke_set_scenario(rid, p_instance_data.scenario);
            smoke->smoke_set_transform(rid, transform);
            smoke->smoke_set_visible(rid, p_instance_data.visible);
            smoke->smoke_set_intensity(rid, p_instance_data.smoke_intensity);
            smoke->smoke_set_sprite(rid, Ref<Texture2D>(resolved.get("sprite", Variant())));
            smoke->smoke_set_source(rid, Ref<SmokeSource>(resolved.get("source", Variant())));
            p_instance_data.smoke_emitters.push_back({rid, placement.offset});
        }
    }

    /// Where the emitter spawns: the model root's own basis (the original launches the particles
    /// along the owner's up vector, particles.cpp:63/300) over the submodel's offset
    Transform3D MaszynaE3DRenderingServer::_smoke_transform(
            const MaszynaE3DInstanceData &p_instance_data, const Vector3 &p_offset) {
        return p_instance_data.transform * Transform3D(Basis(), p_offset);
    }

    void MaszynaE3DRenderingServer::_clear_instance_smoke_sources(MaszynaE3DInstanceData &p_instance_data) {
        if (SmokeServer *smoke = SmokeServer::get_instance(); smoke != nullptr) {
            for (const MaszynaE3DInstanceData::SmokeEmitter &emitter: p_instance_data.smoke_emitters) {
                smoke->smoke_free(emitter.rid);
            }
        }
        p_instance_data.smoke_emitters.clear();
    }

    void MaszynaE3DRenderingServer::_update_instance_smoke(const MaszynaE3DInstanceData &p_instance_data) {
        SmokeServer *smoke = SmokeServer::get_instance();
        ERR_FAIL_NULL(smoke);
        for (const MaszynaE3DInstanceData::SmokeEmitter &emitter: p_instance_data.smoke_emitters) {
            smoke->smoke_set_transform(emitter.rid, _smoke_transform(p_instance_data, emitter.offset));
            smoke->smoke_set_visible(emitter.rid, p_instance_data.visible);
        }
    }

    void MaszynaE3DRenderingServer::instance_set_smoke_intensity(const RID &p_instance, const float p_intensity) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->smoke_intensity = p_intensity;
        SmokeServer *smoke = SmokeServer::get_instance();
        ERR_FAIL_NULL(smoke);
        for (const MaszynaE3DInstanceData::SmokeEmitter &emitter: instance->smoke_emitters) {
            smoke->smoke_set_intensity(emitter.rid, p_intensity);
        }
    }

    /// Advances the blinking lights. A light is resolved again only when its cycle crossed an
    /// edge, so a frame between two edges does no more than the arithmetic.
    void MaszynaE3DRenderingServer::_process_lights() {
        const int size = static_cast<int>(blinking_instances.size());
        if (size == 0) {
            return;
        }
        light_clock = static_cast<double>(time->get_ticks_usec()) / Units::USEC_PER_SECOND;
        const int visited = MIN(size, MAX_BLINKING_INSTANCES_PER_FRAME);
        for (int i = 0; i < visited; i++) {
            if (blinking_cursor >= size) {
                blinking_cursor = 0;
            }
            MaszynaE3DInstanceData *instance = instances.getptr(blinking_instances[blinking_cursor]);
            blinking_cursor++;
            if (instance == nullptr) {
                continue;
            }
            for (const KeyValue<String, MaszynaE3DInstanceData::LightDeclaration> &declaration:
                 instance->light_declarations) {
                if (declaration.value.mode == LIGHT_MODE_BLINK &&
                    _is_light_on(declaration.value) != declaration.value.blink_on) {
                    _resolve_lights(*instance);
                    _update_visibility_if_built(*instance);
                    break;
                }
            }
        }
    }

    /// Keeps the instance on the blinking list while any of its lights blinks, and the tick
    /// connected while the list holds anything
    void
    MaszynaE3DRenderingServer::_update_blinking(const RID &p_instance, const MaszynaE3DInstanceData &p_instance_data) {
        bool blinking = false;
        for (const KeyValue<String, MaszynaE3DInstanceData::LightDeclaration> &declaration:
             p_instance_data.light_declarations) {
            blinking = blinking || declaration.value.mode == LIGHT_MODE_BLINK;
        }
        const bool listed = blinking_instances.has(p_instance);
        if (blinking && !listed) {
            blinking_instances.push_back(p_instance);
        } else if (!blinking && listed) {
            blinking_instances.erase(p_instance);
        }
        _set_light_processing(!blinking_instances.is_empty());
    }

    void MaszynaE3DRenderingServer::_set_light_processing(const bool p_processing) {
        if (light_processing == p_processing) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr) {
            return;
        }
        light_processing = p_processing;
        if (p_processing) {
            light_clock = static_cast<double>(time->get_ticks_usec()) / Units::USEC_PER_SECOND;
            tree->connect("process_frame", callable_mp(this, &MaszynaE3DRenderingServer::_process_lights));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &MaszynaE3DRenderingServer::_process_lights));
    }

    void MaszynaE3DRenderingServer::instance_set_submodel_rotation(
            const RID &p_instance, const String &p_submodel, const Vector3 &p_degrees, const double p_speed) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        MaszynaE3DInstanceData::SubmodelAnimation &animation = instance->submodel_animations[p_submodel.to_lower()];
        animation.target_angles = p_degrees;
        animation.rotate_speed = p_speed;
        _start_submodel_animation(p_instance, *instance, p_submodel.to_lower());
    }

    void MaszynaE3DRenderingServer::instance_set_submodel_translation(
            const RID &p_instance, const String &p_submodel, const Vector3 &p_offset, const double p_speed) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        MaszynaE3DInstanceData::SubmodelAnimation &animation = instance->submodel_animations[p_submodel.to_lower()];
        animation.target_offset = p_offset;
        animation.translate_speed = p_speed;
        _start_submodel_animation(p_instance, *instance, p_submodel.to_lower());
    }

    void MaszynaE3DRenderingServer::_start_submodel_animation(
            const RID &p_instance, MaszynaE3DInstanceData &p_instance_data, const String &p_submodel) {
        if (p_instance_data.built) {
            p_instance_data.submodel_animations[p_submodel].submodel =
                    _find_submodel(p_instance_data.model->get_submodels(), p_submodel);
        }
        if (!animating_instances.has(p_instance)) {
            animating_instances.push_back(p_instance);
            // advanced by the whole slice at the next pass - the slice its event runs in, if that
            // pass has not come yet - as the original moves an animation in the frame its event
            // fires (TAnimContainer::UpdateModel())
            p_instance_data.animation_pass = animation_pass;
        }
        _set_animation_processing(true);
    }

    MaszynaE3DSubModel *
    MaszynaE3DRenderingServer::_find_submodel(const TypedArray<MaszynaE3DSubModel> &p_submodels, const String &p_name) {
        for (int i = 0; i < p_submodels.size(); i++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[i];
            if (submodel.is_null()) {
                continue;
            }
            if (submodel->get_name().to_lower() == p_name) {
                return submodel.ptr();
            }
            if (MaszynaE3DSubModel *found = _find_submodel(submodel->get_submodels(), p_name); found != nullptr) {
                return found;
            }
        }
        return nullptr;
    }

    /// TSubModel::RaAnimation() at_RotateXYZ (Model3d.cpp:1145-1152): the offset, then the angles
    /// about x, y and z, on top of the submodel's own transform; a client's pose on top of that
    /// Written over in place, not rebuilt: an entry stays from the build on (instance_build()
    /// clears them), and a settings pose takes its animation from the animation itself
    void MaszynaE3DRenderingServer::_pose_submodels(MaszynaE3DInstanceData &p_instance) {
        for (const KeyValue<String, MaszynaE3DInstanceData::SubmodelAnimation> &animation:
             p_instance.submodel_animations) {
            if (animation.value.submodel != nullptr) {
                p_instance.submodel_poses[animation.value.submodel] = _animation_pose(animation.value);
            }
        }
        for (const KeyValue<String, MaszynaE3DInstanceData::SubmodelSettings> &settings: p_instance.submodel_settings) {
            if (settings.value.submodel == nullptr || !settings.value.posed) {
                continue;
            }
            const MaszynaE3DInstanceData::SubmodelAnimation *animation =
                    p_instance.submodel_animations.getptr(settings.key);
            p_instance.submodel_poses[settings.value.submodel] =
                    animation != nullptr && animation->submodel != nullptr
                            ? _animation_pose(*animation) * settings.value.pose
                            : settings.value.pose;
        }
        _get_backend(p_instance).apply_poses(p_instance);
    }

    Transform3D
    MaszynaE3DRenderingServer::_animation_pose(const MaszynaE3DInstanceData::SubmodelAnimation &p_animation) {
        const Vector3 &angles = p_animation.angles;
        const Basis rotation = Basis(Vector3(1.0, 0.0, 0.0), Math::deg_to_rad(angles.x)) *
                               Basis(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(angles.y)) *
                               Basis(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(angles.z));
        return Transform3D(rotation, p_animation.offset);
    }

    /// What a new build is given of what was set on it before: the client's submodel settings
    /// and every pose
    void MaszynaE3DRenderingServer::_apply_client_submodels(MaszynaE3DInstanceData &p_instance) {
        if (!p_instance.submodel_settings.is_empty()) {
            _resolve_submodel_settings(p_instance);
            _get_backend(p_instance).update(p_instance);
        }
        if (!p_instance.submodel_animations.is_empty() || !p_instance.submodel_settings.is_empty()) {
            _pose_submodels(p_instance);
        }
    }

    void MaszynaE3DRenderingServer::_resolve_submodel_settings(MaszynaE3DInstanceData &p_instance) {
        p_instance.shown_submodels.clear();
        p_instance.hidden_submodels.clear();
        p_instance.submodel_materials.clear();
        p_instance.submodel_emission_energies.clear();
        p_instance.submodel_light_thresholds.clear();
        for (KeyValue<String, MaszynaE3DInstanceData::SubmodelSettings> &settings: p_instance.submodel_settings) {
            settings.value.submodel = _find_submodel(p_instance.model->get_submodels(), settings.key);
            if (settings.value.submodel == nullptr) {
                continue;
            }
            if (settings.value.visibility_set) {
                if (settings.value.hidden) {
                    p_instance.hidden_submodels.insert(settings.value.submodel);
                } else {
                    p_instance.shown_submodels.insert(settings.value.submodel);
                }
            }
            if (settings.value.material_override.is_valid()) {
                p_instance.submodel_materials[settings.value.submodel] = settings.value.material_override;
            }
            if (settings.value.emission_energy >= 0.0) {
                _set_subtree_emission_energy(p_instance, settings.value.submodel, settings.value.emission_energy);
            }
            if (settings.value.lights_on_threshold > MaszynaE3DInstanceData::SubmodelSettings::NO_THRESHOLD) {
                _set_subtree_lights_on_threshold(
                        p_instance, settings.value.submodel, settings.value.lights_on_threshold);
            }
        }
    }

    void MaszynaE3DRenderingServer::_set_subtree_lights_on_threshold(
            MaszynaE3DInstanceData &p_instance, const MaszynaE3DSubModel *p_submodel, const float p_threshold) {
        p_instance.submodel_light_thresholds[p_submodel] = p_threshold;
        const TypedArray<MaszynaE3DSubModel> children = p_submodel->get_submodels();
        for (int index = 0; index < children.size(); index++) {
            const Ref<MaszynaE3DSubModel> child = children[index];
            if (child.is_valid()) {
                _set_subtree_lights_on_threshold(p_instance, child.ptr(), p_threshold);
            }
        }
    }

    void MaszynaE3DRenderingServer::_set_subtree_emission_energy(
            MaszynaE3DInstanceData &p_instance, const MaszynaE3DSubModel *p_submodel, const float p_energy) {
        p_instance.submodel_emission_energies[p_submodel] = p_energy;
        const TypedArray<MaszynaE3DSubModel> children = p_submodel->get_submodels();
        for (int index = 0; index < children.size(); index++) {
            const Ref<MaszynaE3DSubModel> child = children[index];
            if (child.is_valid()) {
                _set_subtree_emission_energy(p_instance, child.ptr(), p_energy);
            }
        }
    }

    void MaszynaE3DRenderingServer::instance_set_part_poses(const RID &p_instance, const Dictionary &p_poses) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        bool added = false;
        const Array names = p_poses.keys();
        for (int index = 0; index < names.size(); index++) {
            const String name = String(names[index]).to_lower();
            added = added || !instance->submodel_settings.has(name);
            MaszynaE3DInstanceData::SubmodelSettings &settings = instance->submodel_settings[name];
            settings.pose = p_poses[names[index]];
            settings.posed = true;
        }
        if (!instance->built) {
            return;
        }
        if (added) {
            _resolve_submodel_settings(*instance);
        }
        _pose_submodels(*instance);
    }

    void MaszynaE3DRenderingServer::instance_set_part_visible(
            const RID &p_instance, const String &p_submodel, const bool p_visible) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        MaszynaE3DInstanceData::SubmodelSettings &settings = instance->submodel_settings[p_submodel.to_lower()];
        settings.visibility_set = true;
        settings.hidden = !p_visible;
        if (instance->built) {
            _resolve_submodel_settings(*instance);
            _get_backend(*instance).update_visibility(*instance);
        }
    }

    void MaszynaE3DRenderingServer::instance_set_part_material_override(
            const RID &p_instance, const String &p_submodel, const Ref<Material> &p_material) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        instance->submodel_settings[p_submodel.to_lower()].material_override = p_material;
        if (instance->built) {
            _resolve_submodel_settings(*instance);
            _get_backend(*instance).update(*instance);
        }
    }

    /// The threshold is set on the instance's own copies of its emissive materials
    void MaszynaE3DRenderingServer::instance_set_part_lights_on_threshold(
            const RID &p_instance, const String &p_submodel, const float p_threshold) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const bool copies_needed = instance->emission_energy < 0.0 && !instance->submodel_emission;
        instance->submodel_emission = true;
        instance->submodel_settings[p_submodel.to_lower()].lights_on_threshold = p_threshold;
        if (!instance->built) {
            return;
        }
        // the copies are made as it is built
        if (copies_needed) {
            _rebuild_if_built(*instance);
            return;
        }
        _resolve_submodel_settings(*instance);
        _get_backend(*instance).update(*instance);
    }

    void MaszynaE3DRenderingServer::instance_set_part_emission_energy(
            const RID &p_instance, const String &p_submodel, const float p_energy) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const bool copies_needed = instance->emission_energy < 0.0 && !instance->submodel_emission;
        instance->submodel_emission = true;
        instance->submodel_settings[p_submodel.to_lower()].emission_energy = p_energy;
        if (!instance->built) {
            return;
        }
        // the instance's own copies of its emissive materials are made as it is built
        if (copies_needed) {
            _rebuild_if_built(*instance);
            return;
        }
        _resolve_submodel_settings(*instance);
        _get_backend(*instance).update(*instance);
    }

    bool MaszynaE3DRenderingServer::instance_has_part(const RID &p_instance, const String &p_submodel) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, false);
        return _find_submodel(instance->model->get_submodels(), p_submodel.to_lower()) != nullptr;
    }

    bool MaszynaE3DRenderingServer::_find_submodel_transform(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const String &p_name, const Transform3D &p_parent,
            Transform3D &p_r_transform) {
        for (int index = 0; index < p_submodels.size(); index++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent * submodel->get_transform();
            if (submodel->get_name().to_lower() == p_name) {
                p_r_transform = transform;
                return true;
            }
            if (_find_submodel_transform(submodel->get_submodels(), p_name, transform, p_r_transform)) {
                return true;
            }
        }
        return false;
    }

    Transform3D
    MaszynaE3DRenderingServer::instance_get_part_transform(const RID &p_instance, const String &p_submodel) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Transform3D());
        Transform3D transform;
        ERR_FAIL_COND_V_MSG(
                !_find_submodel_transform(
                        instance->model->get_submodels(), p_submodel.to_lower(), Transform3D(), transform),
                Transform3D(), vformat("No submodel '%s' in the model.", p_submodel));
        return transform;
    }

    bool MaszynaE3DRenderingServer::_merge_submodel_aabb(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const Transform3D &p_parent, AABB &p_r_aabb) {
        bool found = false;
        for (int index = 0; index < p_submodels.size(); index++) {
            const Ref<MaszynaE3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent * submodel->get_transform();
            if (submodel->get_mesh().is_valid()) {
                const AABB aabb = transform.xform(submodel->get_mesh()->get_aabb());
                p_r_aabb = found || p_r_aabb.has_volume() ? p_r_aabb.merge(aabb) : aabb;
                found = true;
            }
            found = _merge_submodel_aabb(submodel->get_submodels(), transform, p_r_aabb) || found;
        }
        return found;
    }

    Ref<MaszynaE3DModel> MaszynaE3DRenderingServer::instance_get_model(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Ref<MaszynaE3DModel>());
        return instance->model;
    }

    Transform3D MaszynaE3DRenderingServer::instance_get_transform(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, Transform3D());
        return instance->transform;
    }

    String MaszynaE3DRenderingServer::instance_get_data_path(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, String());
        return instance->data_path;
    }

    String MaszynaE3DRenderingServer::instance_get_model_filename(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, String());
        return instance->model_filename;
    }

    PackedStringArray MaszynaE3DRenderingServer::instance_get_skins(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, PackedStringArray());
        return instance->skins;
    }

    AABB MaszynaE3DRenderingServer::instance_get_aabb(const RID &p_instance) const {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, AABB());
        AABB aabb;
        _merge_submodel_aabb(instance->model->get_submodels(), Transform3D(), aabb);
        return aabb;
    }

    void MaszynaE3DRenderingServer::instance_set_instancer(const RID &p_instance, const Instancer p_instancer) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        if (instance->instancer == p_instancer) {
            return;
        }
        const bool built = instance->built;
        // cleared by the instancer that built it, built by the new one
        if (built) {
            _clear_instance_lights(*instance);
            _clear_instance_smoke_sources(*instance);
            _get_backend(*instance).clear(*instance);
            instance->built = false;
            built_instances.erase(p_instance);
        }
        instance->instancer = p_instancer;
        if (built) {
            instance_build(p_instance);
        }
    }

    void MaszynaE3DRenderingServer::instance_set_emission_energy(const RID &p_instance, const float p_energy) {
        MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL(instance);
        const bool copies_needed = instance->emission_energy < 0.0 && !instance->submodel_emission && p_energy >= 0.0;
        instance->emission_energy = p_energy;
        // the instance's own copies of its emissive materials are made as it is built
        if (copies_needed) {
            _rebuild_if_built(*instance);
        }
        _update_if_built(*instance);
    }

    /// TAnimContainer::UpdateModel() (AnimModel.cpp:92-188): every angle turns towards its target at
    /// the speed, the offset moves along the straight line to its own. A model out of range keeps
    /// moving, so it is where it should be when it comes back.
    void MaszynaE3DRenderingServer::_process_animations(const double p_seconds) {
        const SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        const double now = simulation->simulation_get_time();
        animation_pass++;
        const int visits = MIN(static_cast<int>(animating_instances.size()), MAX_ANIMATED_INSTANCES_PER_SLICE);
        for (int visit = 0; visit < visits && !animating_instances.is_empty(); visit++) {
            if (animation_cursor >= animating_instances.size()) {
                animation_cursor = 0;
            }
            const RID instance_rid = animating_instances[animation_cursor];
            MaszynaE3DInstanceData *instance = instances.getptr(instance_rid);
            bool moving = false;
            finished_animations.clear();
            if (instance != nullptr) {
                // the slice itself, not a difference of two clock readings, which loses the last
                // bit of a step and lets an arrival slip by a tick
                const double seconds =
                        instance->animation_pass + 1 == animation_pass ? p_seconds : now - instance->animation_time;
                instance->animation_pass = animation_pass;
                instance->animation_time = now;
                for (KeyValue<String, MaszynaE3DInstanceData::SubmodelAnimation> &item: instance->submodel_animations) {
                    MaszynaE3DInstanceData::SubmodelAnimation &animation = item.value;
                    if (animation.rotate_speed != 0.0) {
                        const double step = Math::abs(animation.rotate_speed) * seconds;
                        for (int axis = Vector3::AXIS_X; axis <= Vector3::AXIS_Z; axis++) {
                            const double difference = animation.target_angles[axis] - animation.angles[axis];
                            animation.angles[axis] =
                                    Math::abs(difference) <= step
                                            ? animation.target_angles[axis]
                                            : animation.angles[axis] + static_cast<real_t>(SIGN(difference) * step);
                        }
                        if (animation.angles == animation.target_angles) {
                            animation.rotate_speed = 0.0;
                            finished_animations.push_back(item.key);
                        } else {
                            moving = true;
                        }
                    }
                    if (animation.translate_speed != 0.0) {
                        const Vector3 difference = animation.target_offset - animation.offset;
                        const double step = Math::abs(animation.translate_speed) * seconds;
                        if (difference.length() <= MAX(step, ANIMATION_TRANSLATION_EPSILON)) {
                            animation.offset = animation.target_offset;
                            animation.translate_speed = 0.0;
                            finished_animations.push_back(item.key);
                        } else {
                            animation.offset += difference.normalized() * static_cast<real_t>(step);
                            moving = true;
                        }
                    }
                }
                if (instance->built) {
                    _pose_submodels(*instance);
                }
            }
            if (moving) {
                animation_cursor++;
            } else {
                animating_instances.remove_at(animation_cursor);
            }
            // after the list is settled: a listener or an event may start another animation
            for (const String &submodel: finished_animations) {
                emit_signal(instance_submodel_animation_finished_signal, instance_rid, submodel);
                const MaszynaE3DInstanceData *finished = instances.getptr(instance_rid);
                const RID *done_event =
                        finished != nullptr ? finished->submodel_done_events.getptr(submodel.to_lower()) : nullptr;
                if (done_event != nullptr) {
                    ScenarioEventServer *scenario_events = ScenarioEventServer::get_instance();
                    ERR_FAIL_NULL(scenario_events);
                    scenario_events->event_queue(*done_event);
                }
            }
        }
        _set_animation_processing(!animating_instances.is_empty());
    }

    void MaszynaE3DRenderingServer::_set_animation_processing(const bool p_processing) {
        if (animation_processing == p_processing) {
            return;
        }
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        animation_processing = p_processing;
        if (p_processing) {
            simulation->connect(
                    SimulationServer::simulation_advanced_signal,
                    callable_mp(this, &MaszynaE3DRenderingServer::_process_animations));
            return;
        }
        simulation->disconnect(
                SimulationServer::simulation_advanced_signal,
                callable_mp(this, &MaszynaE3DRenderingServer::_process_animations));
    }

    /// Addressable handle for the model's own light_onNN/light_offNN submodel pair. The submodels
    /// themselves are switched by the backend from lights_state, so this only gives a caller
    /// something to enable and disable uniformly with the real lights.
    RID MaszynaE3DRenderingServer::emission_light_create(const RID &p_instance, const String &p_light_name) {
        return _light_create(p_instance, p_light_name, LIGHT_KIND_EMISSION, MaszynaE3DLightParams());
    }

    RID MaszynaE3DRenderingServer::spot_light_create(
            const RID &p_instance, const String &p_light_name, const NodePath &p_submodel_path) {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, RID());
        ERR_FAIL_COND_V(instance->model.is_null(), RID());
        const Ref<MaszynaE3DSubModel> submodel = instance->model->get_node_or_null(p_submodel_path);
        ERR_FAIL_COND_V(submodel.is_null(), RID());
        const MaszynaE3DLightParams params = MaszynaE3DLightFactory::from_submodel(submodel.ptr(), p_light_name);
        return _light_create(p_instance, p_light_name, LIGHT_KIND_SPOT, params);
    }

    RID MaszynaE3DRenderingServer::omni_light_create(
            const RID &p_instance, const String &p_light_name, const NodePath &p_submodel_path) {
        const MaszynaE3DInstanceData *instance = instances.getptr(p_instance);
        ERR_FAIL_NULL_V(instance, RID());
        ERR_FAIL_COND_V(instance->model.is_null(), RID());
        const Ref<MaszynaE3DSubModel> submodel = instance->model->get_node_or_null(p_submodel_path);
        ERR_FAIL_COND_V(submodel.is_null(), RID());
        const MaszynaE3DLightParams params = MaszynaE3DLightFactory::from_submodel(submodel.ptr(), p_light_name);
        return _light_create(p_instance, p_light_name, LIGHT_KIND_OMNI, params);
    }

    void MaszynaE3DRenderingServer::light_free(const RID &p_light) {
        const RIDMap<LightObject>::Iterator item = lights.find(p_light);
        ERR_FAIL_COND(item == lights.end());
        if (MaszynaE3DInstanceData *instance = instances.getptr(item->value.owner); instance != nullptr) {
            instance->light_objects.erase(p_light);
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
            item->value.stream_rid.is_valid() && streaming != nullptr) {
            streaming->stream_free(item->value.stream_rid);
        }
        _light_clear(p_light);
        lights.remove(item);
    }

    void MaszynaE3DRenderingServer::light_enable(const RID &p_light) {
        LightObject *light = lights.getptr(p_light);
        ERR_FAIL_NULL(light);
        light->enabled = true;
        _light_apply_enabled(*light);
        if (MaszynaE3DInstanceData *instance = instances.getptr(light->owner); instance != nullptr) {
            instance->lights_override[light->light_name] = true;
            _resolve_lights(*instance);
            _update_visibility_if_built(*instance);
        }
    }

    void MaszynaE3DRenderingServer::light_disable(const RID &p_light) {
        LightObject *light = lights.getptr(p_light);
        ERR_FAIL_NULL(light);
        light->enabled = false;
        _light_apply_enabled(*light);
        if (MaszynaE3DInstanceData *instance = instances.getptr(light->owner); instance != nullptr) {
            instance->lights_override[light->light_name] = false;
            _resolve_lights(*instance);
            _update_visibility_if_built(*instance);
        }
    }

    /// Light 0 of a scenery node is the "00" the E3D parser derived from "light_on00"
    String MaszynaE3DRenderingServer::_light_name_for_index(const int p_index) {
        return String::num_int64(p_index).pad_zeros(2);
    }

    /// TAnimModel::RaPrepare(), AnimModel.cpp:578-627, and the timers of RaAnimate(),
    /// AnimModel.cpp:534-540 - without the opacity transition, a blinking light is on or off.
    /// The cycle runs on one clock for every light instead of a timer per model; the original's
    /// timers all start at load, so they are in step as well.
    bool MaszynaE3DRenderingServer::_is_light_on(const MaszynaE3DInstanceData::LightDeclaration &p_declaration) const {
        switch (p_declaration.mode) {
            case LIGHT_MODE_OFF:
                return false;
            case LIGHT_MODE_ON:
                return true;
            case LIGHT_MODE_BLINK: {
                const double period = p_declaration.on_time + p_declaration.off_time;
                return Math::fmod(light_clock + p_declaration.phase, period) < p_declaration.on_time;
            }
            case LIGHT_MODE_HOME: {
                // like dark, but forced off late at night
                if (current_time >= HOME_LIGHTS_OFF_FROM_HOUR && current_time < HOME_LIGHTS_OFF_TO_HOUR) {
                    return false;
                }
                [[fallthrough]];
            }
            default: {
                // the fraction carries the light's own threshold, e.g. `lights 3.4` means 0.4
                const double threshold =
                        p_declaration.threshold > 0.0 ? p_declaration.threshold : DEFAULT_DARK_THRESHOLD;
                return light_level <= threshold;
            }
        }
    }

    void MaszynaE3DRenderingServer::_resolve_lights(MaszynaE3DInstanceData &p_instance) {
        Dictionary state;
        for (KeyValue<String, MaszynaE3DInstanceData::LightDeclaration> &declaration: p_instance.light_declarations) {
            const bool on = _is_light_on(declaration.value);
            declaration.value.blink_on = on;
            state[declaration.key] = on;
        }
        state.merge(p_instance.lights_override, true);
        p_instance.lights_state = state;

        for (const RID &light_rid: p_instance.light_objects) {
            LightObject *light = lights.getptr(light_rid);
            if (light == nullptr) {
                continue;
            }
            const bool enabled = state.get(light->light_name, false);
            if (light->enabled == enabled) {
                continue;
            }
            light->enabled = enabled;
            _light_apply_enabled(*light);
        }
    }

    /// Only the built instances: one built later is resolved against the time and the light of
    /// that moment (instance_build())
    void MaszynaE3DRenderingServer::_resolve_all_lights() {
        for (const RID &rid: built_instances) {
            MaszynaE3DInstanceData *instance = instances.getptr(rid);
            if (instance == nullptr || instance->light_declarations.is_empty()) {
                continue; // nothing automatic to decide
            }
            const Dictionary previous = instance->lights_state;
            _resolve_lights(*instance);
            if (!(previous == instance->lights_state)) {
                _update_visibility_if_built(*instance);
            }
        }
    }

    /* The scenery's home lights go dark between 1:00 and 5:00, so the hour is all they need */
    void MaszynaE3DRenderingServer::_on_time_of_day_hour_changed() {
        if (const SimulationServer *simulation = SimulationServer::get_instance(); simulation != nullptr) {
            environment_set_time(simulation->get_time_of_day());
        }
    }

    void MaszynaE3DRenderingServer::environment_set_time(const double p_hours) {
        if (Math::is_equal_approx(current_time, p_hours)) {
            return;
        }
        current_time = p_hours;
        _resolve_all_lights();
    }

    void MaszynaE3DRenderingServer::environment_set_light_level(const double p_level) {
        if (Math::is_equal_approx(light_level, p_level)) {
            return;
        }
        light_level = p_level;
        _resolve_all_lights();
    }

    Dictionary MaszynaE3DRenderingServer::light_get_statistics() const {
        int lit = 0;
        int spot = 0;
        int omni = 0;
        int synthesized = 0;
        for (const KeyValue<RID, LightObject> &item: lights) {
            if (item.value.kind == LIGHT_KIND_EMISSION) {
                continue;
            }
            if (item.value.enabled) {
                lit++;
            }
            if (item.value.kind == LIGHT_KIND_OMNI) {
                omni++;
            } else {
                spot++;
            }
            if (item.value.synthesized) {
                synthesized++;
            }
        }
        Dictionary statistics;
        statistics["total"] = static_cast<int>(lights.size());
        statistics["lit"] = lit;
        statistics["spot"] = spot;
        statistics["omni"] = omni;
        statistics["synthesized"] = synthesized;
        return statistics;
    }

} // namespace godot
