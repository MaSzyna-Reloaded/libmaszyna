#pragma once
#include "E3DInstanceBackend.hpp"
#include "E3DInstanceTypes.hpp"
#include "E3DLightFactory.hpp"
#include "E3DNodesBackend.hpp"
#include "E3DOptimizedBackend.hpp"
#include "E3DSmokeSourceFactory.hpp"
#include "scenery/SceneryModelPlacement.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/particle_process_material.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/mutex.hpp>

namespace godot {
    /// RID based server of E3D model instances, similar to RenderingServer.
    /// The instancer of an instance selects how it is built: OPTIMIZED renders RenderingServer
    /// instances without any nodes, NODES/EDITABLE_NODES build a node tree under the attached node.
    /// RenderingServer instances are freed when the singleton is deleted.
    ///
    /// Scenery placements are registered with instance_register() instead of being built right
    /// away: SceneryStreamingServer builds them only while the camera is within their range.
    /// InstanceKind and Translucency are E3DInstanceTypes', shared with the backends below it.
    class E3DRenderingServer : public Object, public E3DInstanceTypes {
            GDCLASS(E3DRenderingServer, Object)

        public:
            enum Instancer {
                INSTANCER_OPTIMIZED,
                INSTANCER_NODES,
                INSTANCER_EDITABLE_NODES,
            };

            /// Light state of a scenery model node (TLightState, AnimModel.h:27-33)
            enum LightMode {
                LIGHT_MODE_OFF = 0,
                LIGHT_MODE_ON = 1,
                LIGHT_MODE_BLINK = 2,
                LIGHT_MODE_DARK = 3, // lit automatically once it gets dark
                LIGHT_MODE_HOME = 4, // like dark, but off late at night
            };

            /// Threshold of the light level below which a LIGHT_MODE_DARK light comes on, when the
            /// declared mode carries no fraction of its own (DefaultDarkThresholdLevel,
            /// AnimModel.h:24)
            static constexpr double DEFAULT_DARK_THRESHOLD = 0.325;
            /// A translation this close to its target has arrived (1 cm, AnimModel.cpp:101)
            static constexpr double ANIMATION_TRANSLATION_EPSILON = 0.01;
            /// LIGHT_MODE_HOME lights are forced off between these hours (AnimModel.cpp:601-607)
            static constexpr double HOME_LIGHTS_OFF_FROM_HOUR = 1.0;
            static constexpr double HOME_LIGHTS_OFF_TO_HOUR = 5.0;

            static constexpr const char *SCENERY_LIGHT_DISTANCE_SETTING = "maszyna/scenery/lights/distance";
            static constexpr float DEFAULT_SCENERY_LIGHT_DISTANCE = 400.0;
            static constexpr const char *SCENERY_LIGHT_SHADOWS_SETTING = "maszyna/scenery/lights/cast_shadows";
            static constexpr bool DEFAULT_SCENERY_LIGHT_SHADOWS = true;
            /// Shadows are dropped well before the light itself is, the way E3DNodesBackend fades
            /// a vehicle spotlight out
            static constexpr float SCENERY_LIGHT_SHADOW_FADE_DISTANCE = 80.0;
            /// The light fades out over this share of its streaming distance - the wrapper's own
            static constexpr float SCENERY_LIGHT_FADE_LENGTH_SHARE = 0.25;
            /// A lamp must not shadow its own light. With one light per arm each arm was lit by its
            /// neighbours; economy mode leaves a single light in the middle, below which the arms
            /// and the pole throw long dark spokes right across the pool. The geometry of a model
            /// that carries a light goes on this layer alone (a layer kept beside it would still
            /// match the caster mask), and every scenery light leaves the layer out of its shadow
            /// caster mask - so the lamp still renders, is still lit, and still casts a shadow from
            /// the sun, just not into its own light.
            static constexpr uint32_t SCENERY_LIGHT_OWNER_LAYER = 1u << 19;
            /// A light created through RenderingServer starts with the server's own parameters,
            /// not with the ones SpotLight3D/OmniLight3D set in their constructors - they have to
            /// be set, or the ground self-shadows into stripes. A node's values (spot 0.03, normal
            /// 1.0) are not enough for a street lamp: its cone reaches ~60 degrees off the axis
            /// and its map is 128-256 px, so a texel on the ground is 0.0135-0.027 x the distance.
            /// The depth bias (applied before the perspective divide, ~20 x bias x distance of
            /// depth) covers the ground near the axis at 128 px; the normal bias, which Godot
            /// scales by 10 / map size and by 1 - cos(angle), needs ~2.5 at 30-45 degrees off the
            /// axis to cover one texel of slope plus the PCF kernel, so 3.0 (see FINDINGS.md,
            /// 2026-09-27 thin station objects).
            static constexpr float SPOT_LIGHT_SHADOW_BIAS = 0.06;
            static constexpr float OMNI_LIGHT_SHADOW_BIAS = 0.1;
            static constexpr float LIGHT_SHADOW_NORMAL_BIAS = 3.0;
            /// Light3D's constructor value (light_3d.cpp:505); the server starts at 0, and a spot's
            /// depth bias is multiplied by it (light_storage.cpp:1232) - at 0 the ground has no
            /// depth bias at all and shadows itself across the whole pool
            static constexpr float LIGHT_SHADOW_BLUR = 1.0;
            static constexpr const char *SCENERY_LIGHT_ENERGY_SETTING = "maszyna/scenery/lights/energy";
            static constexpr float DEFAULT_SCENERY_LIGHT_ENERGY = 1.0;
            /// How much of the lamp's own colour is mixed into a white light. A sodium lamp's
            /// (1.0, 0.66, 0.18) used raw throws away most of the light's luminance and the pool
            /// comes out nearly black, so the colour tints white light instead of replacing it.
            static constexpr const char *SCENERY_LIGHT_TINT_SETTING = "maszyna/scenery/lights/tint";
            static constexpr float DEFAULT_SCENERY_LIGHT_TINT = 0.5;
            static constexpr const char *SCENERY_LIGHT_VOLUMETRIC_FOG_ENERGY_SETTING =
                    "maszyna/scenery/lights/volumetric_fog_energy";
            static constexpr float DEFAULT_SCENERY_LIGHT_VOLUMETRIC_FOG_ENERGY = 4.0;

            /// The original's gfx.smoke (Globals.cpp:1314). With it off no emitter is created at all.
            static constexpr const char *SMOKE_ENABLED_SETTING = "maszyna/smoke/enabled";
            static constexpr bool DEFAULT_SMOKE_ENABLED = true;
            /// A scenery emitter streams in at this distance. The original stops spawning beyond
            /// 2 * BaseDrawRange * fDistanceFactor (particles.cpp:452); a chimney has to be visible
            /// from further away than a street lamp, so this is not the scenery light distance.
            static constexpr const char *SMOKE_DYNAMIC_DISTANCE_SETTING = "maszyna/smoke/dynamic/distance";
            static constexpr const char *SMOKE_STATIC_DISTANCE_SETTING = "maszyna/smoke/static/distance";
            static constexpr float DEFAULT_SMOKE_DISTANCE = 1500.0;
            /// The original displaces a particle by 0.1 * age * wind every step
            /// (particles.cpp:383), which integrates to 0.05 * wind * t^2 - exactly what a
            /// constant acceleration of 0.1 * wind gives, so the drift rides the process
            /// material's gravity.
            static constexpr float SMOKE_WIND_ACCELERATION = 0.1;
            /// Emitters visited per frame. Below this every emitter is visited every frame, which
            /// is what spreads its particles out evenly; above it the tick carries on where it
            /// left off, so an emitter waits a few frames and then spawns the whole backlog its
            /// own clock owes it. The count of particles is unchanged either way - only how
            /// evenly they are spread.
            static constexpr int MAX_SMOKE_SOURCES_PER_FRAME = 64;
            /// Animating instances advanced per simulation slice, round-robin as the emitters; one
            /// left for a later slice is advanced by the simulated time it waited
            static constexpr int MAX_ANIMATED_INSTANCES_PER_SLICE = 64;
            /// Blinking instances visited per frame; above it an instance's edge comes a few
            /// frames late, the cycle itself runs on the clock and does not drift
            static constexpr int MAX_BLINKING_INSTANCES_PER_FRAME = 64;

            /// Emitted once an instance is freed, so whatever refers to it can let go
            static const char *instance_freed_signal;
            /// Emitted after an instance is built - its model, and so its lights, are known from then
            static const char *instance_built_signal;
            /// A submodel reached what instance_set_submodel_rotation()/_translation() sent it to
            /// (TAnimContainer's evDone, AnimModel.cpp:112-120, 175-180)
            static const char *instance_submodel_animation_finished_signal;

            static E3DRenderingServer *get_instance() {
                return Object::cast_to<E3DRenderingServer>(
                        Engine::get_singleton()->get_singleton("E3DRenderingServer"));
            }

        private:
            /// An addressable light of an instance. An emission light only switches the model's
            /// own light_onNN/light_offNN submodels (the backends do that from lights_state), a
            /// spot or omni light additionally owns a RenderingServer light.
            enum LightKind {
                LIGHT_KIND_EMISSION,
                LIGHT_KIND_SPOT,
                LIGHT_KIND_OMNI,
            };

            struct LightObject {
                    RID owner; // the E3D instance this light belongs to
                    LightKind kind = LIGHT_KIND_EMISSION;
                    String light_name; // the E3DModel.lights entry it follows
                    bool enabled = false;
                    bool synthesized = false; // added by the street lamp quirk
                    E3DLightParams params;
                    RID light;          // RenderingServer light
                    RID light_instance; // its RenderingServer instance
                    RID stream_rid;     // SceneryStreamingServer registration, scenery lights only
                    bool streamed_in = false;
            };

            /// A particle emitter of an instance. Unlike a light it exists for every instancer:
            /// a vehicle past maszyna/vehicles/detail_distance has no node tree left to
            /// hang one on, and its plume is the thing still visible at that range.
            struct SmokeObject {
                    RID owner; // the E3D instance this emitter belongs to
                    String template_name;
                    Vector3 offset; // relative to the model root
                    RID particles;
                    RID particles_instance;
                    Ref<ParticleProcessMaterial> process_material;
                    /// Reach of the plume around the emitter, in its own space; the particles are
                    /// left behind rather than carried, so the box follows the emitter and not the
                    /// plume - a fast vehicle's trail is culled with its emitter (see TODO.md)
                    AABB local_aabb;
                    float spawn_rate = 0.0;       // particles per second the template declares
                    float spawn_backlog = 0.0;    // fractional particles carried to the next tick
                    uint64_t last_spawn_usec = 0; // its own clock, so a skipped frame costs nothing
                    int amount = 0;               // pool size, the cap on one tick's spawns
                    float intensity = 1.0;        // spawn rate multiplier, mirrored from the owner instance
                    /// Mirrored from the owner instance whenever it moves or is shown/hidden, so
                    /// the per-frame tick is arithmetic on this struct alone and never looks an
                    /// instance up
                    Transform3D transform;
                    bool visible = true;
                    /// A vehicle's emitter, spawned by hand (_process_smoke()); a static one the
                    /// engine emits for
                    bool dynamic = false;
                    /// In smoke_order: dynamic, visible, built and of an intensity above zero
                    bool ordered = false;
                    RID stream_rid; // SceneryStreamingServer registration, scenery emitters only
                    bool streamed_in = false;
            };

            HashMap<RID, E3DInstanceData> instances;
            HashMap<RID, LightObject> lights;
            HashMap<RID, SmokeObject> smoke_objects;
            /// The emitters that spawn now (SmokeObject::ordered), in a flat list the per-frame tick
            /// walks round-robin; a HashMap's elements do not move, so they are held by pointer
            LocalVector<SmokeObject *> smoke_order;
            /// Taken once: the smoke tick reads the clock every frame
            Time *time = nullptr;
            int smoke_cursor = 0;
            E3DOptimizedBackend optimized_backend;
            E3DNodesBackend nodes_backend{false};
            E3DNodesBackend editable_nodes_backend{true};
            E3DMaterialResolver material_resolver;

            /// Registered instances are built through SceneryStreamingServer under this owner
            int stream_owner = -1;
            /// ...and the real lights of scenery instances under this one, with a range of their
            /// own: a street lamp is visible from half a kilometre and lights fifteen metres
            int light_stream_owner = -1;
            /// ...and the particle emitters under this one, with a range of their own again
            int smoke_stream_owner = -1;
            // Pushed by MaszynaEnvironmentNode: the vector the emitters drift with, in m/s
            Vector3 wind;
            bool smoke_processing = false;
            /// Instances with a LIGHT_MODE_BLINK light, walked round-robin by _process_lights()
            Vector<RID> blinking_instances;
            int blinking_cursor = 0;
            bool light_processing = false;
            /// Instances with a submodel still moving towards its target, advanced by
            /// _process_animations() on the simulation's clock - at its speed, standing while it
            /// stands (Timer::GetDeltaTime())
            Vector<RID> animating_instances;
            int animation_cursor = 0;
            /// Counts the passes of _process_animations() (E3DInstanceData::animation_pass)
            uint64_t animation_pass = 0;
            /// The submodels whose animation arrived in the instance being advanced, kept so a
            /// pass allocates nothing
            LocalVector<String> finished_animations;
            bool animation_processing = false;
            double light_clock = 0.0;   // seconds, the clock every blinking light cycles on
            double current_time = 12.0; // hours, 0..24
            double light_level = 1.0;   // Global.fLuminance equivalent (simulationenvironment.cpp:184)
            /// The instances built now - the only ones whose lights a change of time or light reaches
            HashSet<RID> built_instances;
            Callable model_loader;
            Callable smoke_source_resolver;
            /// The ResourceLazyLoader resource of each registered instance's model - in a map of
            /// its own, as _stream_preload() runs off the main thread, where instances is not safe
            HashMap<RID, RID> stream_models;
            Mutex models_mutex;

            E3DInstanceBackend &_get_backend(const E3DInstanceData &p_instance);
            const E3DInstanceBackend &_get_backend(const E3DInstanceData &p_instance) const;
            void _rebuild_if_built(E3DInstanceData &p_instance);
            void _update_if_built(E3DInstanceData &p_instance);
            void _on_data_unload_requested();
            void _on_data_reload_requested();
            /// What the lights and the emitters were built with, by the settings that shape them -
            /// the placements of the scenery lights, their RenderingServer lights, the emitters: a
            /// change of one of them builds that again (_on_project_settings_changed())
            Array light_placement_settings;
            Array light_build_settings;
            Array smoke_settings;
            void _on_project_settings_changed();
            /// A model a SceneryStreamingProvider supplies: a scenery placement named by nothing
            RID _adopt_model(const Ref<SceneryModelPlacement> &p_placement, const RID &p_scenario);
            RID _get_stream_model(const RID &p_instance);
            Variant _stream_preload(const RID &p_instance);
            void _stream_build(const RID &p_instance, const Variant &p_preloaded);
            void _stream_clear(const RID &p_instance);

            RID _light_create(
                    const RID &p_instance, const String &p_light_name, LightKind p_kind, const E3DLightParams &p_params,
                    bool p_synthesized = false);
            void _light_build(const RID &p_light);
            void _light_stream_build(const RID &p_light, const Variant &p_preloaded);
            void _light_clear(const RID &p_light);
            void _light_apply_enabled(LightObject &p_light);
            void _build_instance_lights(const RID &p_instance, E3DInstanceData &p_instance_data);
            static void _apply_declared_color(
                    const E3DInstanceData &p_instance_data, const String &p_light_name, E3DLightParams &p_params);
            void _clear_instance_lights(E3DInstanceData &p_instance_data);

            void _build_instance_smoke_sources(const RID &p_instance, E3DInstanceData &p_instance_data);
            void _smoke_build(const RID &p_smoke);
            void _smoke_stream_build(const RID &p_smoke, const Variant &p_preloaded);
            void _smoke_clear(const RID &p_smoke);
            void _clear_instance_smoke_sources(E3DInstanceData &p_instance_data);
            void _update_instance_smoke(const E3DInstanceData &p_instance_data);
            static Transform3D _smoke_transform(const E3DInstanceData &p_instance_data, const SmokeObject &p_smoke);
            void _apply_smoke_placement(const E3DInstanceData &p_instance_data, SmokeObject &p_smoke);
            /// The emitter enters smoke_order when it starts to spawn and leaves it when it stops
            void _refresh_smoke_order(SmokeObject &p_smoke);
            void _apply_smoke_wind(const SmokeObject &p_smoke) const;
            /// Connected to SceneTree's process_frame while any emitter exists, the way
            /// SceneryStreamingServer drives its own streaming - no script runs per frame
            void _process_smoke();
            void _process_smoke_source(SmokeObject &p_smoke, uint64_t p_now);
            void _set_smoke_processing(bool p_processing);
            /// Resolves lights_state out of the declared modes, the manual overrides and the time
            /// of day, then applies it to the backend and to the instance's light objects
            void _resolve_lights(E3DInstanceData &p_instance);
            void _resolve_all_lights();
            bool _is_light_on(const E3DInstanceData::LightDeclaration &p_declaration) const;
            /// Connected to SceneTree's process_frame while any light blinks
            void _process_lights();
            void _update_blinking(const RID &p_instance, const E3DInstanceData &p_instance_data);
            void _set_light_processing(bool p_processing);
            static String _light_name_for_index(int p_index);
            /// First submodel of the name in the tree, compared in lower case (TModel3d::GetFromName())
            static E3DSubModel *_find_submodel(const TypedArray<E3DSubModel> &p_submodels, const String &p_name);
            /// Puts the instance on the animating list, with the animated submodel found if it is built
            void _start_submodel_animation(
                    const RID &p_instance, E3DInstanceData &p_instance_data, const String &p_submodel);
            /// Composes the poses out of the animations and hands them to the backend
            void _pose_submodels(E3DInstanceData &p_instance);
            static Transform3D _animation_pose(const E3DInstanceData::SubmodelAnimation &p_animation);
            /// Finds the submodels the client's settings name in the built model, and what the
            /// backends read of them
            void _resolve_submodel_settings(E3DInstanceData &p_instance);
            static void
            _set_subtree_emission_energy(E3DInstanceData &p_instance, const E3DSubModel *p_submodel, float p_energy);
            void _apply_client_submodels(E3DInstanceData &p_instance);
            /// The submodel's transform in the model, through every parent; false when not found
            static bool _find_submodel_transform(
                    const TypedArray<E3DSubModel> &p_submodels, const String &p_name, const Transform3D &p_parent,
                    Transform3D &p_r_transform);
            static bool _merge_submodel_aabb(
                    const TypedArray<E3DSubModel> &p_submodels, const Transform3D &p_parent, AABB &p_r_aabb);
            /// Connected to SceneTree's process_frame while a submodel moves
            void _process_animations(double p_seconds);
            void _set_animation_processing(bool p_processing);

        protected:
            static void _bind_methods();

        public:
            E3DRenderingServer();
            ~E3DRenderingServer() override;

            RID instance_create(const Ref<E3DModel> &p_model, Instancer p_instancer, InstanceKind p_instance_kind);
            RID instance_register(
                    const String &p_data_path, const String &p_model_filename, const PackedStringArray &p_skins,
                    const Transform3D &p_transform, float p_range_begin, float p_range_end, const RID &p_scenario);
            void instance_free(const RID &p_instance);
            /// Turns the submodel towards p_degrees about its own x, y and z at p_speed degrees per
            /// second (a scenery `animation ... rotate` event, TAnimContainer::SetRotateAnim())
            void instance_set_submodel_rotation(
                    const RID &p_instance, const String &p_submodel, const Vector3 &p_degrees, double p_speed);
            /// Moves the submodel towards p_offset at p_speed metres per second (`animation ...
            /// translate`, TAnimContainer::SetTranslateAnim())
            void instance_set_submodel_translation(
                    const RID &p_instance, const String &p_submodel, const Vector3 &p_offset, double p_speed);
            /// Poses submodels, by name, on top of their own transform - a submodel to pose and the
            /// transform it takes, kept across rebuilds. What a vehicle's running gear, pantographs,
            /// wipers and mirrors are drawn with; an empty transform takes a submodel back to rest.
            void instance_set_submodel_poses(const RID &p_instance, const Dictionary &p_poses);
            /// Shows or hides a submodel, by name, kept across rebuilds - a vehicle's couplers and hoses
            void instance_set_submodel_visible(const RID &p_instance, const String &p_submodel, bool p_visible);
            /// Draws a submodel, by name, with p_material instead of its own - a head display
            void instance_set_submodel_material_override(
                    const RID &p_instance, const String &p_submodel, const Ref<Material> &p_material);
            /// The self-illumination energy of a submodel, by name, and of everything under it,
            /// kept across rebuilds; below 0 gives it the instance's own again - one cab of a
            /// low-poly interior lit by its cab light
            void instance_set_submodel_emission_energy(const RID &p_instance, const String &p_submodel, float p_energy);
            bool instance_has_submodel(const RID &p_instance, const String &p_submodel) const;
            /// The submodel's transform in the model, through every parent, without any pose
            Transform3D instance_get_submodel_transform(const RID &p_instance, const String &p_submodel) const;
            /// The model the instance draws
            Ref<E3DModel> instance_get_model(const RID &p_instance) const;
            /// Where the instance stands (instance_set_transform(), instance_register())
            Transform3D instance_get_transform(const RID &p_instance) const;
            /// The directory its model and textures are read from
            String instance_get_data_path(const RID &p_instance) const;
            /// The model file of a registered instance (instance_register()); empty for one created
            /// from a model (instance_create())
            String instance_get_model_filename(const RID &p_instance) const;
            PackedStringArray instance_get_skins(const RID &p_instance) const;
            /// The bounds of every mesh of the model, in the model's space
            AABB instance_get_aabb(const RID &p_instance) const;
            /// Builds the instance again with another instancer, keeping the RID and everything set on
            /// it - a vehicle drawn as nodes near the camera and as RenderingServer instances far away
            void instance_set_instancer(const RID &p_instance, Instancer p_instancer);
            /// The self-illumination energy of every emissive material of the instance (the
            /// shaders' emission_energy) - a low-poly interior lit by the roof light
            void instance_set_emission_energy(const RID &p_instance, float p_energy);
            void instance_build(const RID &p_instance);
            /// Every instance's emitters are built again - what they are made of has changed
            void smoke_rebuild();
            void instance_set_options(
                    const RID &p_instance, const String &p_data_path, const PackedStringArray &p_skins,
                    const Array &p_exclude_node_names, bool p_force_alpha,
                    const TypedArray<NodePath> &p_force_alpha_submodel_paths, int p_max_texture_size);
            void instance_attach_object_instance_id(const RID &p_instance, uint64_t p_id);
            /// The node attached, by its ObjectID - 0 for none, or for one that is gone
            uint64_t instance_get_attached_node(const RID &p_instance) const;
            /// Where the model sits in the attached node - a node tree built under it is placed
            /// there; the instance's transform stays the model's own, in the world
            void instance_set_node_transform(const RID &p_instance, const Transform3D &p_transform);
            void instance_set_scenario(const RID &p_instance, const RID &p_scenario);
            void instance_set_transform(const RID &p_instance, const Transform3D &p_transform);
            void instance_set_visible(const RID &p_instance, bool p_visible);
            void instance_set_layer_mask(const RID &p_instance, uint32_t p_mask);
            void instance_set_visibility_range(const RID &p_instance, float p_begin, float p_end);
            void instance_set_material_overlay(const RID &p_instance, const Ref<Material> &p_material);
            Dictionary
            instance_intersect_segment(const RID &p_instance, const Vector3 &p_from, const Vector3 &p_to) const;
            void instance_set_lights_state(const RID &p_instance, const Dictionary &p_lights_state);
            void
            instance_set_lights_dimmed(const RID &p_instance, const Dictionary &p_lights_dimmed, float p_multiplier);
            /// The scenery node's `lights` list, by light index (light 0 is "00", AnimModel.cpp:303)
            void instance_set_lights_modes(const RID &p_instance, const PackedFloat32Array &p_modes);
            /// The scenery node's `lightcolors` list, in the same order
            void instance_set_lights_colors(const RID &p_instance, const PackedColorArray &p_colors);
            void instance_set_light_mode(const RID &p_instance, int p_light, LightMode p_mode);
            /// Lights addressed by index (light_on00...), 0 until the instance is built
            int instance_get_light_count(const RID &p_instance) const;
            /// The instance ids of the drawn meshes of opaque submodels - what the original draws into
            /// its pick buffer; empty until the instance is built as nodes
            PackedInt64Array instance_get_opaque_meshes(const RID &p_instance) const;
            void instance_set_light_blink(
                    const RID &p_instance, int p_light, float p_on_time, float p_off_time, float p_phase);

            RID emission_light_create(const RID &p_instance, const String &p_light_name);
            RID spot_light_create(const RID &p_instance, const String &p_light_name, const NodePath &p_submodel_path);
            RID omni_light_create(const RID &p_instance, const String &p_light_name, const NodePath &p_submodel_path);
            void light_free(const RID &p_light);
            void light_enable(const RID &p_light);
            void light_disable(const RID &p_light);
            /// total/lit/spot/omni/synthesized, for the scenery streaming debug panel
            Dictionary light_get_statistics() const;

            /// Spawn rate multiplier of every emitter of the instance, as the engine state drives
            /// it. 1.0 is the template's own rate - what a scenery chimney keeps.
            ///
            /// Only the rate. Nothing here may reach a particle that is already in the air: both
            /// the process material's colour and its initial colour ramp are read every frame, so
            /// driving either of them from the engine state made the whole plume step down
            /// together instead of thinning out (see FINDINGS.md). The original scales the
            /// opacity of a particle by dizel_fill in its spawn routine (particles.cpp:330);
            /// Godot's only spawn-time channel is the emission itself, so dizel_fill is folded
            /// into the rate by the caller.
            void instance_set_smoke_intensity(const RID &p_instance, float p_intensity);
            /// total/built, for the scenery streaming debug panel
            Dictionary smoke_get_statistics() const;

            /// Pushed by MaszynaEnvironmentNode; the first two drive the automatic light modes,
            /// the wind drifts the particles of every emitter
            void environment_set_time(double p_hours);
            void environment_set_light_level(double p_level);
            void environment_set_wind(float p_strength, const Vector3 &p_direction);

            void material_set_resolver(const Callable &p_material_resolver);
            void model_set_loader(const Callable &p_model_loader);
            /// The model of a file, through the loader (shared by every instance of it while it lives)
            Ref<E3DModel> model_load(const String &p_data_path, const String &p_model_filename);
            void smoke_set_source_resolver(const Callable &p_smoke_source_resolver);
    };
} // namespace godot

VARIANT_ENUM_CAST(E3DRenderingServer::Instancer)
VARIANT_ENUM_CAST(E3DRenderingServer::LightMode)
VARIANT_ENUM_CAST(E3DRenderingServer::InstanceKind)
VARIANT_ENUM_CAST(E3DRenderingServer::Translucency)
