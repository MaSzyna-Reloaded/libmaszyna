#include "RailVehicleRenderingServer.hpp"
#include "game_data/GameDataServer.hpp"
#include "legacy/e3d/E3DModel.hpp"
#include "legacy/e3d/E3DRenderingServer.hpp"
#include "resources/ResourceLazyLoader.hpp"
#include "scenery/SceneryHUDMouseServer.hpp"
#include "scenery/SceneryStreamingServer.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleLighting.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/physics_server3d.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/classes/world3d.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <array>

namespace godot {
    RailVehicleRenderingServer *RailVehicleRenderingServer::singleton = nullptr;
    const char *RailVehicleRenderingServer::vehicle_model_built_signal = "vehicle_model_built";

    namespace {
        /* Which submodel of the model shows which of the vehicle's lamps, read off the lighting
         * component - a renamed or removed lamp is a build error rather than a light that silently
         * stops working. The headlights' dimming reaches the headlamps only (DynObj.cpp:1212-1320). */
        enum LightDimming { LIGHT_DIMMING_NONE, LIGHT_DIMMING_HEADLIGHT };

        struct LightStateBinding {
                const char *light_name;
                bool (RailVehicleLighting::*is_enabled)() const;
                LightDimming dimming;
        };

        constexpr std::array<LightStateBinding, 10> LIGHT_STATE_BINDINGS = {{
                {"headlamp11", &RailVehicleLighting::get_front_headlight_upper_enabled, LIGHT_DIMMING_HEADLIGHT},
                {"headlamp12", &RailVehicleLighting::get_front_headlight_right_enabled, LIGHT_DIMMING_HEADLIGHT},
                {"headlamp13", &RailVehicleLighting::get_front_headlight_left_enabled, LIGHT_DIMMING_HEADLIGHT},
                {"headlamp21", &RailVehicleLighting::get_rear_headlight_upper_enabled, LIGHT_DIMMING_HEADLIGHT},
                {"headlamp22", &RailVehicleLighting::get_rear_headlight_right_enabled, LIGHT_DIMMING_HEADLIGHT},
                {"headlamp23", &RailVehicleLighting::get_rear_headlight_left_enabled, LIGHT_DIMMING_HEADLIGHT},
                {"endsignal12", &RailVehicleLighting::get_front_redmarker_right_enabled, LIGHT_DIMMING_NONE},
                {"endsignal13", &RailVehicleLighting::get_front_redmarker_left_enabled, LIGHT_DIMMING_NONE},
                {"endsignal22", &RailVehicleLighting::get_rear_redmarker_right_enabled, LIGHT_DIMMING_NONE},
                {"endsignal23", &RailVehicleLighting::get_rear_redmarker_left_enabled, LIGHT_DIMMING_NONE},
        }};

        /* The coupler and air hose submodels a model may have, each as _on, _off and _xon
         * (AirCoupler::Init(), DynObj.cpp:2170-2181, AirCoupler.cpp:54) */
        constexpr std::array<const char *, 10> COUPLER_SUBMODELS = {
                "coupler1",     "coupler2",   "cpneumatic1", "cpneumatic1r", "cpneumatic2",
                "cpneumatic2r", "pneumatic1", "pneumatic1r", "pneumatic2",   "pneumatic2r"};
        constexpr std::array<const char *, 3> COUPLER_SUFFIXES = {"_on", "_off", "_xon"};
        constexpr int COUPLER_PART_COUNT = 3;
        /* The rear cab's low-poly cab, cab2 (LowPolyIntCabs[2], DynObj.cpp:2391) */
        constexpr int LOW_POLY_MACHINE_ROOM = 0;
        constexpr int LOW_POLY_FRONT_CAB = 1;
        constexpr int LOW_POLY_REAR_CAB = 2;
        /* Wiper elements: arm 1, arm 2, blade (DynObj.cpp:5838-5870) */
        constexpr int WIPER_ELEMENTS = 3;
        constexpr int WIPER_BLADE = 2;
        /* Pantograph elements: lower arm, its pair, upper arm, its pair, slider (DynObj.cpp:5414) */
        constexpr int PANTOGRAPH_ELEMENTS = 5;
        constexpr int PANTOGRAPH_LOWER_ARM = 0;
        constexpr int PANTOGRAPH_UPPER_ARM = 2;
        constexpr int PANTOGRAPH_SLIDER = 4;
        /* Where a coupler adapter's model is looked for (TModelsManager::GetModel()) */
        constexpr const char *COUPLER_ADAPTER_MODELS = "models";
        /* TAnimPant's dimensions of a pantograph type: lower and upper arm, the slider's horizontal
         * offset and its height over its pivot - AKP_4E, and the DSAx, EC160_200 and WBL85 alike
         * (DynObj.cpp:90-160) */
        struct PantographDimensions {
                double lower_length;
                double upper_length;
                double horizontal;
                double slider_height;
        };
        constexpr PantographDimensions PANTOGRAPH_AKP_4E{1.22, 1.755, 0.535, 0.07};
        constexpr PantographDimensions PANTOGRAPH_DSA{1.98374, 2.14199, 0.142, 0.09353};
        /* common_pantograph_settings(): the lower arm's angle lowered (DynObj.cpp:74) */
        constexpr double PANTOGRAPH_LOWER_REST_ANGLE_DEGREES = 2.8547285515689267247882521833308;
        /* The slider's scale the model's own height is trusted at (DynObj.cpp:5459) */
        constexpr double PANTOGRAPH_SLIDER_SCALE_TOLERANCE = 0.001;
        /* pantfactors: - two places along, then two slider heights (DynObj.cpp:5580-5588) */
        constexpr int PANTOGRAPH_FACTOR_COUNT = 4;
        constexpr int PANTOGRAPH_FACTOR_HEIGHTS = 2;
        constexpr std::array<const char *, 2> PNEUMATIC_SUBMODELS = {"cpneumatic", "pneumatic"};
    } // namespace

    /* A vehicle far away is drawn from instances, a near one as nodes - shown and owned in the
     * editor when it is editable */
    static E3DRenderingServer::Instancer detail_instancer(const bool p_detailed, const bool p_editable) {
        if (!p_detailed) {
            return E3DRenderingServer::INSTANCER_OPTIMIZED;
        }
        return p_editable ? E3DRenderingServer::INSTANCER_EDITABLE_NODES : E3DRenderingServer::INSTANCER_NODES;
    }

    /* The low-poly cab of a kind of cabin: cab1 the front one, cab2 the rear one, cab0 the machine
     * room - and nobody's, as the original's CabOccupied 0 is (DynObj.cpp:1389-1397) */
    static int low_poly_cab(const RailVehicleCabinKind::Kind p_kind) {
        switch (p_kind) {
            case RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT:
                return LOW_POLY_FRONT_CAB;
            case RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR:
                return LOW_POLY_REAR_CAB;
            default:
                return LOW_POLY_MACHINE_ROOM;
        }
    }

    static RailVehicleCabinKind::Kind driver_cabin_kind(const RID &p_vehicle) {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        return server != nullptr ? server->cabin_get_kind(server->vehicle_get_driver_cabin(p_vehicle))
                                 : RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE;
    }

    template<typename T>
    static Ref<T> component(const RID &p_vehicle, const VehicleComponentType::Type p_type) {
        const VehicleServer *server = VehicleServer::get_instance();
        return server != nullptr ? Ref<T>(server->vehicle_component_get(p_vehicle, p_type)) : Ref<T>();
    }

    static Ref<RailVehicleBuffCoupl> couplers(const RID &p_vehicle) {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        return server != nullptr ? Ref<RailVehicleBuffCoupl>(server->vehicle_component_get(
                                           p_vehicle, RailVehicleComponentType::COMPONENT_BUFFERS))
                                 : Ref<RailVehicleBuffCoupl>();
    }

    RailVehicleRenderingServer *RailVehicleRenderingServer::get_instance() {
        return singleton;
    }

    RailVehicleRenderingServer::RailVehicleRenderingServer() {
        singleton = this;
        _on_project_settings_changed();
        ProjectSettings::get_singleton()->connect(
                "settings_changed", callable_mp(this, &RailVehicleRenderingServer::_on_project_settings_changed));
        if (VehicleServer *vehicle_server = VehicleServer::get_instance(); vehicle_server != nullptr) {
            vehicle_server->connect(
                    VehicleServer::vehicle_freed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_freed));
            vehicle_server->connect(
                    VehicleServer::vehicle_config_changed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_config_changed));
        }
        if (RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance(); rail_vehicles != nullptr) {
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_placed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_placed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_placement_changed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_placed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_driver_cabin_changed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_driver_cabin_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_trainset_changed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_trainset_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_attached_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_coupler_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_detached_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_coupler_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_adapter_attached_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_coupler_adapter_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_adapter_removed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_vehicle_coupler_adapter_changed));
        }
        if (E3DRenderingServer *models = E3DRenderingServer::get_instance(); models != nullptr) {
            models->connect(
                    E3DRenderingServer::instance_built_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_instance_built));
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_data_reload_requested));
        }
        if (SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance(); streaming != nullptr) {
            streaming->connect(
                    SceneryStreamingServer::streaming_camera_changed_signal,
                    callable_mp(this, &RailVehicleRenderingServer::_on_streaming_camera_changed));
        }
    }

    void RailVehicleRenderingServer::_on_project_settings_changed() {
        detail_distance =
                ProjectSettings::get_singleton()->get_setting(DETAIL_DISTANCE_SETTING, DEFAULT_DETAIL_DISTANCE);
    }

    /* Every vehicle's place against the new camera at once - the ones within the draw distance
     * wait for their build from now on (builds_get_pending_count()), not from the sweep's turn */
    void RailVehicleRenderingServer::_on_streaming_camera_changed() {
        for (const RID &vehicle: visit_order) {
            _update_detail(vehicle, vehicles[vehicle]);
        }
    }

    int RailVehicleRenderingServer::builds_get_pending_count() const {
        return static_cast<int>(pending_builds.size());
    }

    /* The models this server built from model files are built again from them; the ones handed
     * over are their owner's to build again. Building a model emits signals scripts answer, so the
     * vehicles are taken first. */
    void RailVehicleRenderingServer::_on_data_reload_requested() {
        Vector<RID> reloaded;
        for (const KeyValue<RID, Visual> &item: vehicles) {
            reloaded.push_back(item.key);
        }
        for (const RID &vehicle: reloaded) {
            const Visual *visual = vehicles.getptr(vehicle);
            if (visual == nullptr) {
                continue;
            }
            const Ref<RailVehicleAppearance> appearance = visual->appearance;
            const bool own_models = visual->own_models;
            const bool loaded = visual->load.is_valid();
            const String load_data_path = visual->load_data_path;
            const String load_model_filename = visual->load_model_filename;
            if (own_models) {
                vehicle_set_appearance(vehicle, appearance);
            }
            if (loaded) {
                vehicle_set_load_model(vehicle, load_data_path, load_model_filename);
            }
        }
    }

    RailVehicleRenderingServer::~RailVehicleRenderingServer() {
        while (!vehicles.is_empty()) {
            vehicle_detach(vehicles.begin()->key);
        }
        singleton = nullptr;
    }

    void RailVehicleRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("vehicle_attach", "vehicle", "node_id"), &RailVehicleRenderingServer::vehicle_attach);
        ClassDB::bind_method(D_METHOD("vehicle_detach", "vehicle"), &RailVehicleRenderingServer::vehicle_detach);
        ClassDB::bind_method(
                D_METHOD("vehicle_is_attached", "vehicle"), &RailVehicleRenderingServer::vehicle_is_attached);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_transform", "vehicle", "transform"),
                &RailVehicleRenderingServer::vehicle_set_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_transform", "vehicle"), &RailVehicleRenderingServer::vehicle_get_transform);
        ClassDB::bind_method(
                D_METHOD("vehicle_mount_node", "vehicle", "node_id"), &RailVehicleRenderingServer::vehicle_mount_node);
        ClassDB::bind_method(
                D_METHOD("vehicle_unmount_node", "vehicle", "node_id"),
                &RailVehicleRenderingServer::vehicle_unmount_node);
        ClassDB::bind_method(
                D_METHOD("detection_area_get_vehicle", "area"),
                &RailVehicleRenderingServer::detection_area_get_vehicle);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_scenario", "vehicle", "scenario"),
                &RailVehicleRenderingServer::vehicle_set_scenario);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_appearance", "vehicle", "appearance"),
                &RailVehicleRenderingServer::vehicle_set_appearance);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_appearance", "vehicle"), &RailVehicleRenderingServer::vehicle_get_appearance);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_models", "vehicle", "model", "low_poly"),
                &RailVehicleRenderingServer::vehicle_set_models);
        ClassDB::bind_method(D_METHOD("vehicle_get_model", "vehicle"), &RailVehicleRenderingServer::vehicle_get_model);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_load_model", "vehicle", "data_path", "model_filename"),
                &RailVehicleRenderingServer::vehicle_set_load_model);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_head_display_material", "vehicle", "material"),
                &RailVehicleRenderingServer::vehicle_set_head_display_material);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_visible_low_poly_cabins", "vehicle", "visible"),
                &RailVehicleRenderingServer::vehicle_set_visible_low_poly_cabins);
        ClassDB::bind_method(
                D_METHOD("cabin_set_light_level", "cabin", "level"),
                &RailVehicleRenderingServer::cabin_set_light_level);
        ClassDB::bind_method(
                D_METHOD("vehicle_is_detailed", "vehicle"), &RailVehicleRenderingServer::vehicle_is_detailed);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_editable", "vehicle", "editable"),
                &RailVehicleRenderingServer::vehicle_set_editable);
        ClassDB::bind_method(
                D_METHOD("vehicle_is_editable", "vehicle"), &RailVehicleRenderingServer::vehicle_is_editable);
        ClassDB::bind_method(
                D_METHOD("builds_get_pending_count"), &RailVehicleRenderingServer::builds_get_pending_count);
        ADD_SIGNAL(MethodInfo(vehicle_model_built_signal, PropertyInfo(Variant::RID, "vehicle")));
    }

    Node3D *RailVehicleRenderingServer::_node(const Visual &p_visual) {
        return Object::cast_to<Node3D>(ObjectDB::get_instance(p_visual.node));
    }

    void RailVehicleRenderingServer::vehicle_attach(const RID &p_vehicle, const uint64_t p_node_id) {
        ERR_FAIL_COND(!p_vehicle.is_valid());
        if (vehicles.has(p_vehicle)) {
            vehicle_detach(p_vehicle);
        }
        Visual &visual = vehicles[p_vehicle];
        visual.node = ObjectID(p_node_id);
        if (const Node3D *node = _node(visual); node != nullptr && node->is_inside_tree()) {
            visual.scenario = node->get_world_3d()->get_scenario();
        }
        visit_order.push_back(p_vehicle);
        _set_processing(true);
        // a vehicle already standing on its track is drawn there at once
        _place(p_vehicle, visual);
    }

    void RailVehicleRenderingServer::vehicle_detach(const RID &p_vehicle) {
        Visual *visual = vehicles.getptr(p_vehicle);
        if (visual == nullptr) {
            return;
        }
        _cancel_build(p_vehicle, *visual);
        _free_models(*visual);
        _free_model_resources(*visual);
        if (visual->load.is_valid()) {
            E3DRenderingServer::get_instance()->instance_free(visual->load);
        }
        if (SceneryHUDMouseServer *mouse = SceneryHUDMouseServer::get_instance(); mouse != nullptr) {
            mouse->pickable_free(visual->pickable);
        }
        if (Node *holder = Object::cast_to<Node>(ObjectDB::get_instance(visual->holder)); holder != nullptr) {
            holder->queue_free();
        }
        area_vehicles.erase(visual->detection_area);
        if (PhysicsServer3D *physics = PhysicsServer3D::get_singleton(); physics != nullptr) {
            if (visual->detection_area.is_valid()) {
                physics->free_rid(visual->detection_area);
            }
            if (visual->detection_shape.is_valid()) {
                physics->free_rid(visual->detection_shape);
            }
        }
        vehicles.erase(p_vehicle);
        visit_order.erase(p_vehicle);
        fading.erase(p_vehicle);
        _set_processing(!vehicles.is_empty());
    }

    bool RailVehicleRenderingServer::vehicle_is_attached(const RID &p_vehicle) const {
        return vehicles.has(p_vehicle);
    }

    void RailVehicleRenderingServer::vehicle_set_transform(const RID &p_vehicle, const Transform3D &p_transform) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->transform = p_transform;
        _move(p_vehicle, *visual);
    }

    Transform3D RailVehicleRenderingServer::vehicle_get_transform(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        return visual != nullptr ? visual->transform : Transform3D();
    }

    void RailVehicleRenderingServer::vehicle_mount_node(const RID &p_vehicle, const uint64_t p_node_id) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->mounts.push_back(ObjectID(p_node_id));
        if (Node3D *mount = Object::cast_to<Node3D>(ObjectDB::get_instance(ObjectID(p_node_id)));
            visual->placed && mount != nullptr && mount->is_inside_tree()) {
            mount->set_global_transform(visual->transform);
        }
    }

    /* A vehicle freed takes its mounts' places with it: nothing to take off then */
    void RailVehicleRenderingServer::vehicle_unmount_node(const RID &p_vehicle, const uint64_t p_node_id) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            visual->mounts.erase(ObjectID(p_node_id));
        }
    }

    RID RailVehicleRenderingServer::detection_area_get_vehicle(const RID &p_area) const {
        const RID *vehicle = area_vehicles.getptr(p_area);
        return vehicle != nullptr ? *vehicle : RID();
    }

    /* The models handed over are their owner's (E3DModelInstance), which moves them itself */
    void RailVehicleRenderingServer::vehicle_set_scenario(const RID &p_vehicle, const RID &p_scenario) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->scenario = p_scenario;
        if (visual->placed) {
            _show_models(*visual, p_scenario);
        }
    }

    /* The models this server built, and the cargo, drawn in p_scenario - in none for an empty one */
    void RailVehicleRenderingServer::_show_models(const Visual &p_visual, const RID &p_scenario) const {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        if (p_visual.own_models) {
            for (const RID &instance: {p_visual.model, p_visual.low_poly, p_visual.passengers}) {
                if (instance.is_valid()) {
                    models->instance_set_scenario(instance, p_scenario);
                }
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_set_scenario(instance, p_scenario);
            }
            for (const RID &instance: p_visual.coupler_adapters) {
                if (instance.is_valid()) {
                    models->instance_set_scenario(instance, p_scenario);
                }
            }
        }
        if (p_visual.load.is_valid()) {
            models->instance_set_scenario(p_visual.load, p_scenario);
        }
    }

    void RailVehicleRenderingServer::vehicle_set_appearance(
            const RID &p_vehicle, const Ref<RailVehicleAppearance> &p_appearance) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->appearance = p_appearance;
        visual->model_missing = false;
        // models of its own replace the ones built from the last appearance, built once the
        // vehicle is within the draw distance (_update_detail()); handed-over ones stay. Its files
        // are registered at once - without lazy loading, that loads them
        if (p_appearance.is_valid() && !p_appearance->get_model_filename().is_empty()) {
            _free_models(*visual);
            _free_model_resources(*visual);
            E3DRenderingServer *models = E3DRenderingServer::get_instance();
            ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
            ERR_FAIL_NULL(models);
            ERR_FAIL_NULL(lazy_loader);
            const String data_path = p_appearance->get_data_path();
            PackedStringArray filenames = p_appearance->get_attachment_model_filenames();
            filenames.push_back(p_appearance->get_model_filename());
            filenames.push_back(p_appearance->get_low_poly_model_filename());
            filenames.push_back(p_appearance->get_passengers_model_filename());
            for (const String &filename: filenames) {
                if (filename.is_empty() || visual->model_resources.has(filename)) {
                    continue;
                }
                // the key a scenery placement of the same file has (E3DRenderingServer::instance_register())
                visual->model_resources[filename] = lazy_loader->resource_register(
                        data_path.path_join(filename),
                        callable_mp(models, &E3DRenderingServer::model_load).bind(data_path, filename));
            }
            _build_load(p_vehicle, *visual);
        }
        _bind_parts(p_vehicle, *visual);
        _publish_pantographs(p_vehicle, *visual);
        _update_detail(p_vehicle, *visual);
    }

    Ref<RailVehicleAppearance> RailVehicleRenderingServer::vehicle_get_appearance(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, Ref<RailVehicleAppearance>());
        return visual->appearance;
    }

    void
    RailVehicleRenderingServer::vehicle_set_models(const RID &p_vehicle, const RID &p_model, const RID &p_low_poly) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        _free_models(*visual);
        visual->model = p_model;
        visual->low_poly = p_low_poly;
        visual->own_models = false;
        // handed over, the models are their owner's nodes already - posed from the start, and
        // drawn as instances only once the camera is known to be far (_update_detail())
        visual->detailed = true;
        model_vehicles[p_model] = p_vehicle;
        _bind_parts(p_vehicle, *visual);
        // a model handed over is one built - by whoever owns it, before this server could hear it
        emit_signal(vehicle_model_built_signal, p_vehicle);
    }

    RID RailVehicleRenderingServer::vehicle_get_model(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, RID());
        return visual->model;
    }

    void RailVehicleRenderingServer::vehicle_set_load_model(
            const RID &p_vehicle, const String &p_data_path, const String &p_model_filename) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->load_data_path = p_data_path;
        visual->load_model_filename = p_model_filename;
        _build_load(p_vehicle, *visual);
    }

    /* The cargo is built while the vehicle's models are, and goes with them */
    void RailVehicleRenderingServer::_build_load(const RID &p_vehicle, Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        if (p_visual.load.is_valid()) {
            models->instance_free(p_visual.load);
            p_visual.load = RID();
        }
        const Ref<E3DModel> model = !p_visual.model.is_valid() || p_visual.load_model_filename.is_empty()
                                            ? Ref<E3DModel>()
                                            : models->model_load(p_visual.load_data_path, p_visual.load_model_filename);
        if (model.is_null()) {
            return;
        }
        // nobody looks into the cargo, so it needs no node tree
        p_visual.load = models->instance_create(
                model, E3DRenderingServer::INSTANCER_OPTIMIZED, E3DRenderingServer::INSTANCE_KIND_DYNAMIC);
        models->instance_set_options(
                p_visual.load, p_visual.load_data_path, PackedStringArray(), Array(), false, {}, 0);
        models->instance_set_scenario(p_visual.load, p_visual.placed ? p_visual.scenario : RID());
        models->instance_build(p_visual.load);
        _update_load(p_vehicle, p_visual);
    }

    void RailVehicleRenderingServer::vehicle_set_head_display_material(
            const RID &p_vehicle, const Ref<Material> &p_material) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->head_display_material = p_material;
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        if (models == nullptr || !visual->model.is_valid() || visual->appearance.is_null()) {
            return;
        }
        const String submodel = visual->appearance->get_head_display_submodel();
        if (models->instance_has_submodel(visual->model, submodel)) {
            models->instance_set_submodel_material_override(visual->model, submodel, p_material);
        }
    }

    void RailVehicleRenderingServer::vehicle_set_visible_low_poly_cabins(const RID &p_vehicle, const bool p_visible) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        visual->low_poly_cabs_visible = p_visible;
        _update_low_poly_cabs(p_vehicle, *visual);
    }

    /* Another cab hidden while the low-poly cabs are not all visible */
    void RailVehicleRenderingServer::_on_vehicle_driver_cabin_changed(const RID &p_vehicle, const RID & /* p_cabin */) {
        if (const Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _update_low_poly_cabs(p_vehicle, *visual);
        }
    }

    void RailVehicleRenderingServer::cabin_set_light_level(const RID &p_cabin, const double p_level) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_NULL(rail_vehicles);
        const RID vehicle = vehicle_server->cabin_get_vehicle(p_cabin);
        // a vehicle not drawn (never attached) has no low-poly cab to light
        Visual *visual = vehicles.getptr(vehicle);
        if (visual == nullptr) {
            return;
        }
        visual->cab_light_levels[low_poly_cab(rail_vehicles->cabin_get_kind(p_cabin))] = p_level;
        if (!fading.has(vehicle)) {
            fading.push_back(vehicle);
        }
    }

    bool RailVehicleRenderingServer::vehicle_is_detailed(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, false);
        return visual->detailed;
    }

    /* The nodes are built anew: the holder goes with the detail, and comes back shown in the Scene
     * dock or hidden, with the instancer that gives its nodes owners or none */
    void RailVehicleRenderingServer::vehicle_set_editable(const RID &p_vehicle, const bool p_editable) {
        Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        if (visual->editable == p_editable) {
            return;
        }
        visual->editable = p_editable;
        if (visual->detailed) {
            _set_detailed(p_vehicle, *visual, false);
        }
        _update_detail(p_vehicle, *visual);
    }

    bool RailVehicleRenderingServer::vehicle_is_editable(const RID &p_vehicle) const {
        const Visual *visual = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(visual, false);
        return visual->editable;
    }

    void RailVehicleRenderingServer::_free_models(Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        model_vehicles.erase(p_visual.model);
        if (p_visual.own_models && models != nullptr) {
            for (const RID &instance: {p_visual.model, p_visual.low_poly, p_visual.passengers}) {
                if (instance.is_valid()) {
                    models->instance_free(instance);
                }
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_free(instance);
            }
            for (const RID &instance: p_visual.coupler_adapters) {
                if (instance.is_valid()) {
                    models->instance_free(instance);
                }
            }
        }
        p_visual.coupler_adapters[0] = RID();
        p_visual.coupler_adapters[1] = RID();
        p_visual.model = RID();
        p_visual.low_poly = RID();
        p_visual.passengers = RID();
        p_visual.attachments.clear();
        p_visual.own_models = false;
        if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
            for (const RID &resource: p_visual.held_models) {
                lazy_loader->resource_release(resource);
            }
        }
        p_visual.held_models.clear();
    }

    void RailVehicleRenderingServer::_free_model_resources(Visual &p_visual) {
        if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
            for (const KeyValue<String, RID> &resource: p_visual.model_resources) {
                lazy_loader->resource_free(resource.value);
            }
        }
        p_visual.model_resources.clear();
    }

    /* Every MaSzyna-authored model of a vehicle lives in one vehicle-local frame (the original
     * draws all of them under TDynamicObject::mMatrix, DynObj.cpp:2506-2508); the appearance says
     * where that frame sits in the node. The exterior and the low-poly interior are nodes near the
     * camera - their submodels are posed and hidden - the passengers and the attachments never are. */
    void RailVehicleRenderingServer::_create_models(const RID &p_vehicle, Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ERR_FAIL_NULL(models);
        ERR_FAIL_NULL(lazy_loader);
        const Ref<RailVehicleAppearance> &appearance = p_visual.appearance;
        const String data_path = appearance->get_data_path();
        // models built again while the vehicle is drawn in detail go under the holder it has
        Node3D *holder = Object::cast_to<Node3D>(ObjectDB::get_instance(p_visual.holder));
        const auto create = [&](const String &p_filename, const PackedStringArray &p_skins,
                                const E3DRenderingServer::Instancer p_instancer) {
            // held while built, as a streamed scenery placement holds its model
            const RID *resource = p_visual.model_resources.getptr(p_filename);
            const Ref<Resource> loaded = resource != nullptr ? lazy_loader->resource_load(*resource) : Ref<Resource>();
            if (loaded.is_null()) {
                return RID();
            }
            const Ref<E3DModel> model = lazy_loader->resource_hold(*resource, loaded);
            p_visual.held_models.push_back(*resource);
            if (model.is_null()) {
                return RID();
            }
            const RID instance = models->instance_create(model, p_instancer, E3DRenderingServer::INSTANCE_KIND_DYNAMIC);
            // a vehicle's translucent submodels - its windows - are drawn as the original's alpha
            // pass draws them: blended near the camera, opaque by the optimized instancer far away;
            // a cutout leaves holes in the glass
            constexpr bool FORCE_ALPHA = true;
            models->instance_set_options(instance, data_path, p_skins, Array(), FORCE_ALPHA, {}, 0);
            if (p_instancer != E3DRenderingServer::INSTANCER_OPTIMIZED) {
                models->instance_attach_object_instance_id(instance, holder->get_instance_id());
            }
            models->instance_set_node_transform(instance, appearance->get_model_transform());
            models->instance_set_scenario(instance, p_visual.placed ? p_visual.scenario : RID());
            models->instance_set_transform(instance, p_visual.transform * appearance->get_model_transform());
            models->instance_build(instance);
            return instance;
        };
        const E3DRenderingServer::Instancer detail = detail_instancer(p_visual.detailed, p_visual.editable);
        p_visual.own_models = true;
        p_visual.model = create(appearance->get_model_filename(), appearance->get_skins(), detail);
        p_visual.model_missing = !p_visual.model.is_valid();
        model_vehicles[p_visual.model] = p_vehicle;
        p_visual.low_poly = create(appearance->get_low_poly_model_filename(), appearance->get_skins(), detail);
        // the passengers carry no skin of their own
        p_visual.passengers =
                create(appearance->get_passengers_model_filename(), PackedStringArray(),
                       E3DRenderingServer::INSTANCER_OPTIMIZED);
        // drawn as the exterior is, never posed (DynObj.cpp:5384, opengl33renderer.cpp:3195)
        for (const String &filename: appearance->get_attachment_model_filenames()) {
            const RID attachment = create(filename, appearance->get_skins(), E3DRenderingServer::INSTANCER_OPTIMIZED);
            if (attachment.is_valid()) {
                p_visual.attachments.push_back(attachment);
            }
        }
        _update_coupler_adapters(p_vehicle, p_visual);
        _register_pickable(p_vehicle, p_visual);
    }

    Transform3D RailVehicleRenderingServer::_coupler_adapter_transform(
            const RID &p_vehicle, const Visual &p_visual, const int p_end) const {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (server == nullptr || vehicle_server == nullptr) {
            return p_visual.transform * p_visual.model_transform;
        }
        const auto end = static_cast<RailVehicleController::CouplerEnd>(p_end);
        const double sign = end == RailVehicleController::COUPLER_END_FRONT ? 1.0 : -1.0;
        // in the model's own frame: at the coupler's height, the adapter's length beyond the end,
        // the rear one turned about
        const double length = vehicle_server->vehicle_get_dimensions(p_vehicle).z;
        const Transform3D placement(
                end == RailVehicleController::COUPLER_END_FRONT ? Basis() : Basis(Vector3(0.0, 1.0, 0.0), Math::PI),
                Vector3(0.0, static_cast<real_t>(server->vehicle_get_coupler_adapter_height(p_vehicle, end)),
                        static_cast<real_t>(
                                (server->vehicle_get_coupler_adapter_length(p_vehicle, end) + (length * 0.5)) * sign)));
        return p_visual.transform * p_visual.model_transform * placement;
    }

    void RailVehicleRenderingServer::_update_coupler_adapters(const RID &p_vehicle, Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        if (models == nullptr || !p_visual.own_models) {
            return;
        }
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        for (int end = 0; end < 2; ++end) {
            if (p_visual.coupler_adapters[end].is_valid()) {
                models->instance_free(p_visual.coupler_adapters[end]);
                p_visual.coupler_adapters[end] = RID();
            }
            const String model_name = server != nullptr
                                              ? server->vehicle_get_coupler_adapter_model(
                                                        p_vehicle, static_cast<RailVehicleController::CouplerEnd>(end))
                                              : String();
            // the original's model manager takes it under models/ (TModelsManager::GetModel())
            const Ref<E3DModel> model =
                    model_name.is_empty() ? Ref<E3DModel>() : models->model_load(COUPLER_ADAPTER_MODELS, model_name);
            if (model.is_null()) {
                continue;
            }
            const RID instance = models->instance_create(
                    model, E3DRenderingServer::INSTANCER_OPTIMIZED, E3DRenderingServer::INSTANCE_KIND_DYNAMIC);
            models->instance_set_options(
                    instance, COUPLER_ADAPTER_MODELS, p_visual.appearance->get_skins(), Array(), true, {}, 0);
            models->instance_set_scenario(instance, p_visual.placed ? p_visual.scenario : RID());
            models->instance_set_transform(instance, _coupler_adapter_transform(p_vehicle, p_visual, end));
            models->instance_build(instance);
            p_visual.coupler_adapters[end] = instance;
        }
    }

    RailVehicleRenderingServer::Part
    RailVehicleRenderingServer::_part(const Visual &p_visual, const String &p_submodel) const {
        const E3DRenderingServer *models = E3DRenderingServer::get_instance();
        if (p_submodel.is_empty() || models == nullptr || !p_visual.model.is_valid() ||
            !models->instance_has_submodel(p_visual.model, p_submodel)) {
            return Part();
        }
        return Part{
                p_submodel.to_lower(),
                p_visual.model_transform * models->instance_get_submodel_transform(p_visual.model, p_submodel)};
    }

    Vector<RailVehicleRenderingServer::Part>
    RailVehicleRenderingServer::_parts(const Visual &p_visual, const PackedStringArray &p_submodels) const {
        Vector<Part> parts;
        for (const String &submodel: p_submodels) {
            parts.push_back(_part(p_visual, submodel));
        }
        return parts;
    }

    /* The submodels of the exterior that move, found in the model the vehicle is drawn with. */
    void RailVehicleRenderingServer::_bind_parts(const RID &p_vehicle, Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const Ref<RailVehicleAppearance> &appearance = p_visual.appearance;
        p_visual.poses.clear();
        p_visual.coupler_submodels.clear();
        p_visual.coupler_state = -1;
        p_visual.lights.clear();
        p_visual.wiper_positions.clear();
        p_visual.mirror_left = -1.0;
        if (appearance.is_null() || models == nullptr || !p_visual.model.is_valid()) {
            return;
        }
        p_visual.model_transform = appearance->get_model_transform();
        p_visual.bogies[RailVehicleWheels::BOGIE_FRONT] = _part(p_visual, appearance->get_front_bogie());
        p_visual.bogies[RailVehicleWheels::BOGIE_REAR] = _part(p_visual, appearance->get_rear_bogie());
        p_visual.wheels[0] = _parts(p_visual, appearance->get_front_rolling_wheels());
        p_visual.wheels[1] = _parts(p_visual, appearance->get_powered_wheels());
        p_visual.wheels[2] = _parts(p_visual, appearance->get_rear_rolling_wheels());
        p_visual.pantograph_arms[RailVehicleEnginePowerSource::PANTOGRAPH_FIRST] =
                _parts(p_visual, appearance->get_pantograph_front_arms());
        p_visual.pantograph_arms[RailVehicleEnginePowerSource::PANTOGRAPH_SECOND] =
                _parts(p_visual, appearance->get_pantograph_rear_arms());
        p_visual.wiper_arms = _parts(p_visual, appearance->get_wiper_arms());
        p_visual.mirrors = _parts(p_visual, appearance->get_mirrors());
        p_visual.doors = _parts(p_visual, appearance->get_doors());
        p_visual.door_steps = _parts(p_visual, appearance->get_door_steps());
        p_visual.pendulums = _parts(p_visual, appearance->get_pendulums());
        p_visual.pendulum_amplitude = appearance->get_pendulum_amplitude();
        p_visual.door_positions[0] = p_visual.door_positions[1] = -1.0;
        p_visual.door_step_positions[0] = p_visual.door_step_positions[1] = -1.0;
        for (const char *coupler: COUPLER_SUBMODELS) {
            for (const char *suffix: COUPLER_SUFFIXES) {
                const String name = String(coupler) + suffix;
                if (models->instance_has_submodel(p_visual.model, name)) {
                    p_visual.coupler_submodels.insert(name);
                }
            }
        }
        // only the lamps the model has, as E3DModelInstance merges them
        const Ref<E3DModel> model = models->instance_get_model(p_visual.model);
        const TypedDictionary<String, E3DModelLightDefinition> model_lights =
                model.is_valid() ? model->get_lights() : TypedDictionary<String, E3DModelLightDefinition>();
        for (const LightStateBinding &binding: LIGHT_STATE_BINDINGS) {
            if (model_lights.has(binding.light_name)) {
                p_visual.lights[binding.light_name] = false;
            }
        }
        p_visual.headlights_dimmed = false;
        models->instance_set_lights_state(p_visual.model, p_visual.lights);
        vehicle_set_head_display_material(p_vehicle, p_visual.head_display_material);
        _update_detection_area(p_vehicle, p_visual);
        _update_low_poly_cabs(p_vehicle, p_visual);
        // the interior is lit only by the lights of its cabs (cabin_set_light_level())
        if (p_visual.low_poly.is_valid()) {
            models->instance_set_emission_energy(p_visual.low_poly, 0.0);
            if (!fading.has(p_vehicle)) {
                fading.push_back(p_vehicle);
            }
        }
        _place(p_vehicle, p_visual);
        _update_couplers(p_vehicle, p_visual);
        _update_lights(p_vehicle, p_visual);
    }

    /* The pantograph as the model builds it, handed to RailVehicleServer, which raises it -
     * TAnimPant's lengths and angles (DynObj.cpp:5508-5549). Where it sits on the vehicle is read
     * the same way the original reads it off the submodel's matrix (TAnimPant::vPos): without it
     * both pantographs of a vehicle sampled the wire at the vehicle's origin. Read off the model
     * file, not off its drawing - a vehicle far from the camera has none - and only for a vehicle
     * with a power source, whose pantographs RailVehicleServer raises (vehicle_collect_current()):
     * an EMU's car without an engine carries its unit's (FizTrainPowerParser). */
    void RailVehicleRenderingServer::_publish_pantographs(const RID &p_vehicle, const Visual &p_visual) const {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        const Ref<RailVehicleEnginePowerSource> source =
                server != nullptr ? Ref<RailVehicleEnginePowerSource>(server->vehicle_component_get(
                                            p_vehicle, RailVehicleComponentType::COMPONENT_ENGINE_POWER_SOURCE))
                                  : Ref<RailVehicleEnginePowerSource>();
        const Ref<RailVehicleAppearance> &appearance = p_visual.appearance;
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        if (models == nullptr || lazy_loader == nullptr || appearance.is_null() || source.is_null()) {
            return;
        }
        // the model this server builds from the appearance, or the one handed over
        Ref<E3DModel> model;
        if (const RID *resource = p_visual.model_resources.getptr(appearance->get_model_filename());
            resource != nullptr) {
            model = lazy_loader->resource_load(*resource);
        } else if (p_visual.model.is_valid()) {
            model = models->instance_get_model(p_visual.model);
        }
        if (model.is_null()) {
            return;
        }
        _publish_pantograph_geometry(p_vehicle, p_visual, model, RailVehicleEnginePowerSource::PANTOGRAPH_FIRST);
        _publish_pantograph_geometry(p_vehicle, p_visual, model, RailVehicleEnginePowerSource::PANTOGRAPH_SECOND);
    }

    /* The submodel of the model by its lowered name, and where it rests in the model's frame - the
     * chain of its parents' transforms; null when the model has none of that name */
    static Ref<E3DSubModel> find_submodel(
            const TypedArray<E3DSubModel> &p_submodels, const String &p_name, const Transform3D &p_parent,
            Transform3D &p_r_transform) {
        for (int index = 0; index < p_submodels.size(); ++index) {
            Ref<E3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent * submodel->get_transform();
            if (submodel->get_name().to_lower() == p_name) {
                p_r_transform = transform;
                return submodel;
            }
            if (Ref<E3DSubModel> found = find_submodel(submodel->get_submodels(), p_name, transform, p_r_transform);
                found.is_valid()) {
                return found;
            }
        }
        return {};
    }

    /* The pantograph as TAnimPant builds it (DynObj.cpp:5404-5480, 5577-5633): the arms' dimensions of
     * the type the FIZ names (PantType=, DynObj.cpp:90-194), or - QUIRK, as long as the FIZ files do
     * not name their type - measured from the model: the lower arm's place, the arms' lengths and
     * rest angles from the lower arm to the upper one and the slider, the slider's height over its
     * pivot and its place along the vehicle. A slider the model cannot be measured by takes its
     * place and height from the MMD's pantfactors:, and with it a pantograph with no lower arm stands
     * on top of the vehicle's box. Without a slider nothing is animated. */
    void RailVehicleRenderingServer::_publish_pantograph_geometry(
            const RID &p_vehicle, const Visual &p_visual, const Ref<E3DModel> &p_model,
            const RailVehicleEnginePowerSource::PantographSelector p_pantograph) const {
        RailVehicleServer *server = RailVehicleServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const PackedStringArray names = p_pantograph == RailVehicleEnginePowerSource::PANTOGRAPH_FIRST
                                                ? p_visual.appearance->get_pantograph_front_arms()
                                                : p_visual.appearance->get_pantograph_rear_arms();
        if (server == nullptr || vehicle_server == nullptr || names.size() != PANTOGRAPH_ELEMENTS) {
            return;
        }
        // each arm where it rests in the vehicle's own frame, and the slider's own submodel
        const Transform3D model_transform = p_visual.appearance->get_model_transform();
        Part arms[PANTOGRAPH_ELEMENTS];
        Ref<E3DSubModel> slider_model;
        for (int element = 0; element < PANTOGRAPH_ELEMENTS; ++element) {
            Transform3D rest;
            const String name = names[element].to_lower();
            const Ref<E3DSubModel> submodel =
                    name.is_empty() ? Ref<E3DSubModel>()
                                    : find_submodel(p_model->get_submodels(), name, Transform3D(), rest);
            if (submodel.is_valid()) {
                arms[element] = Part{name, model_transform * rest};
            }
            if (element == PANTOGRAPH_SLIDER) {
                slider_model = submodel;
            }
        }
        if (slider_model.is_null()) {
            return;
        }
        const Ref<RailVehicleEnginePowerSource> source =
                server->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_ENGINE_POWER_SOURCE);
        const RailVehicleEnginePowerSource::PantographType type =
                source.is_valid() ? source->get_current_collector_pantograph_type()
                                  : RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_NONE;
        const PantographDimensions &dimensions =
                type == RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_NONE ||
                                type == RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_AKP_4E
                        ? PANTOGRAPH_AKP_4E
                        : PANTOGRAPH_DSA;
        double lower_length = dimensions.lower_length;
        double upper_length = dimensions.upper_length;
        double horizontal = dimensions.horizontal;
        double slider_height = dimensions.slider_height;
        double lower_rest_angle = Math::deg_to_rad(PANTOGRAPH_LOWER_REST_ANGLE_DEGREES);
        double upper_rest_angle =
                Math::acos(((lower_length * Math::cos(lower_rest_angle)) + horizontal) / upper_length);
        const bool measured = type == RailVehicleEnginePowerSource::PANTOGRAPH_TYPE_NONE;
        const Part &lower = arms[PANTOGRAPH_LOWER_ARM];
        const Part &upper = arms[PANTOGRAPH_UPPER_ARM];
        const Part &slider = arms[PANTOGRAPH_SLIDER];
        Vector3 position;
        if (lower.submodel.is_empty()) {
            slider_height = 0.0;
        } else {
            position.x = lower.rest.origin.x;
            position.y = lower.rest.origin.y;
            if (measured && !upper.submodel.is_empty()) {
                const Vector3 lower_to_upper = lower.rest.basis.inverse().xform(upper.rest.origin - lower.rest.origin);
                const Vector3 upper_to_slider =
                        upper.rest.basis.inverse().xform(slider.rest.origin - upper.rest.origin);
                lower_length = Vector2(lower_to_upper.y, lower_to_upper.z).length();
                upper_length = Vector2(upper_to_slider.y, upper_to_slider.z).length();
                horizontal = Math::abs(upper_to_slider.y) - Math::abs(lower_to_upper.y);
                lower_rest_angle = Math::atan2(Math::abs(lower_to_upper.z), Math::abs(lower_to_upper.y));
                upper_rest_angle = Math::atan2(Math::abs(upper_to_slider.z), Math::abs(upper_to_slider.y));
                // the slider's own top over its pivot and its place along, while its scale holds
                // (DynObj.cpp:5455-5475); a slider with no mesh has no top over it
                if (Math::abs(slider.rest.basis.determinant() - 1.0) < PANTOGRAPH_SLIDER_SCALE_TOLERANCE) {
                    slider_height = slider_model->get_mesh().is_valid()
                                            ? slider.rest.xform(slider_model->get_mesh()->get_aabb()).get_end().y -
                                                      slider.rest.origin.y
                                            : 0.0;
                    position.z = slider.rest.origin.z;
                } else {
                    slider_height = 0.0;
                }
            }
        }
        const PackedFloat64Array factors = p_visual.appearance->get_pantograph_factors();
        if (factors.size() == PANTOGRAPH_FACTOR_COUNT) {
            const int index = p_pantograph == RailVehicleEnginePowerSource::PANTOGRAPH_FIRST ? 0 : 1;
            if (slider_height == 0.0) {
                // the MMD's place along is the model's own, the vehicle frame turns it (DynObj.cpp:5605-5614)
                position.z = model_transform.basis.xform(Vector3(0.0, 0.0, real_t(factors[index]))).z;
                slider_height = factors[PANTOGRAPH_FACTOR_HEIGHTS + index];
            }
            if (position.y == 0.0) {
                const double raised = (lower_length * Math::sin(lower_rest_angle)) +
                                      (upper_length * Math::sin(upper_rest_angle)) + slider_height;
                position.y = static_cast<real_t>(
                        vehicle_server->vehicle_get_dimensions(p_vehicle).y - slider_height - raised);
            }
        }
        server->vehicle_set_pantograph_geometry(
                p_vehicle, p_pantograph, position, lower_length, upper_length, horizontal, lower_rest_angle,
                upper_rest_angle, slider_height);
    }

    /* The vehicle stands on a track: it is drawn where RailVehicleServer placed it. The body's
     * transform is RailVehicleServer's answer and nothing else - it composes it from the bogies. */
    void RailVehicleRenderingServer::_place(const RID &p_vehicle, Visual &p_visual) {
        RailVehicleServer *server = RailVehicleServer::get_instance();
        const Dictionary position = server != nullptr ? server->vehicle_get_track_position(p_vehicle) : Dictionary();
        if (!RID(position.get("track_rid", RID())).is_valid()) {
            return;
        }
        p_visual.transform = server->vehicle_get_transform(p_vehicle);
        _move(p_vehicle, p_visual);
    }

    /* Everything of the vehicle goes where it stands: the nodes mounted on it, its models - built
     * and in their world from its first place on - its cargo and its detection area; the running gear
     * follows where the vehicle is drawn in detail. */
    void RailVehicleRenderingServer::_move(const RID &p_vehicle, Visual &p_visual) {
        if (!p_visual.placed) {
            p_visual.placed = true;
            _show_models(p_visual, p_visual.scenario);
            _update_detection_area(p_vehicle, p_visual);
            // its models are built where it stands
            _update_detail(p_vehicle, p_visual);
        }
        for (const ObjectID &mount_id: p_visual.mounts) {
            if (Node3D *mount = Object::cast_to<Node3D>(ObjectDB::get_instance(mount_id));
                mount != nullptr && mount->is_inside_tree()) {
                mount->set_global_transform(p_visual.transform);
            }
        }
        if (Node3D *holder = Object::cast_to<Node3D>(ObjectDB::get_instance(p_visual.holder)); holder != nullptr) {
            holder->set_global_transform(p_visual.transform);
        }
        const Transform3D model_transform = p_visual.transform * p_visual.model_transform;
        if (E3DRenderingServer *models = E3DRenderingServer::get_instance(); models != nullptr && p_visual.own_models) {
            for (const RID &instance: {p_visual.model, p_visual.low_poly, p_visual.passengers}) {
                if (instance.is_valid()) {
                    models->instance_set_transform(instance, model_transform);
                }
            }
            for (const RID &instance: p_visual.attachments) {
                models->instance_set_transform(instance, model_transform);
            }
            for (int end = 0; end < 2; ++end) {
                if (p_visual.coupler_adapters[end].is_valid()) {
                    models->instance_set_transform(
                            p_visual.coupler_adapters[end], _coupler_adapter_transform(p_vehicle, p_visual, end));
                }
            }
        }
        _update_load(p_vehicle, p_visual);
        if (PhysicsServer3D *physics = PhysicsServer3D::get_singleton();
            physics != nullptr && p_visual.detection_area.is_valid()) {
            physics->area_set_transform(p_visual.detection_area, model_transform);
        }
        if (p_visual.detailed) {
            _pose_running_gear(p_vehicle, p_visual);
            _send_poses(p_visual);
        }
    }

    void RailVehicleRenderingServer::_pose(Visual &p_visual, const Part &p_part, const Basis &p_pose) {
        if (!p_part.submodel.is_empty()) {
            p_visual.poses[p_part.submodel] = Transform3D(p_pose);
        }
    }

    void RailVehicleRenderingServer::_pose(Visual &p_visual, const Part &p_part, const Transform3D &p_pose) {
        if (!p_part.submodel.is_empty()) {
            p_visual.poses[p_part.submodel] = p_pose;
        }
    }

    void RailVehicleRenderingServer::_send_poses(Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        if (models != nullptr && p_visual.model.is_valid() && !p_visual.poses.is_empty()) {
            models->instance_set_submodel_poses(p_visual.model, p_visual.poses);
        }
    }

    /* The bogies turned towards their own track (where each sits on it is RailVehicleServer's,
     * by the wheels' pivot spacing), and the wheels turned by the distance rolled - the same
     * sign as the original's UpdateAxle() (DynObj.cpp:489). A bogie turns about the vehicle's
     * vertical, which in its own frame is its rest basis seen from the vehicle. */
    void RailVehicleRenderingServer::_pose_running_gear(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleWheels> wheels =
                component<RailVehicleWheels>(p_vehicle, VehicleComponentType::COMPONENT_WHEELS);
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        if (wheels.is_null() || server == nullptr) {
            return;
        }
        const Transform3D bogie_transforms[] = {
                server->vehicle_get_bogie_transform(p_vehicle, RailVehicleWheels::BOGIE_FRONT),
                server->vehicle_get_bogie_transform(p_vehicle, RailVehicleWheels::BOGIE_REAR)};
        Vector3 body_forward = bogie_transforms[0].origin - bogie_transforms[1].origin;
        if (!body_forward.is_zero_approx()) {
            body_forward.normalize();
            const double body_yaw = Math::atan2(-body_forward.x, body_forward.z);
            for (int index = 0; index < static_cast<int>(std::size(p_visual.bogies)); ++index) {
                const Vector3 bogie_forward = -bogie_transforms[index].basis.get_column(2).normalized();
                const double bogie_yaw = Math::atan2(-bogie_forward.x, bogie_forward.z);
                const Basis &rest = p_visual.bogies[index].rest.basis;
                _pose(p_visual, p_visual.bogies[index],
                      rest.inverse() * Basis(Vector3(0.0, 1.0, 0.0), static_cast<real_t>(body_yaw - bogie_yaw)) * rest);
            }
        }
        const double angles[] = {
                wheels->get_angle_front_deg(), wheels->get_angle_powered_deg(), wheels->get_angle_rear_deg()};
        for (int group = 0; group < static_cast<int>(std::size(p_visual.wheels)); ++group) {
            const Basis pose(Vector3(1.0, 0.0, 0.0), static_cast<real_t>(Math::deg_to_rad(angles[group])));
            for (const Part &wheel: p_visual.wheels[group]) {
                _pose(p_visual, wheel, pose);
            }
        }
    }

    /* The arms drawn as far as RailVehicleServer has raised them: lower arm, its pair, upper arm,
     * its pair, slider */
    void RailVehicleRenderingServer::_pose_pantographs(const RID &p_vehicle, Visual &p_visual) {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        if (server == nullptr) {
            return;
        }
        for (const RailVehicleEnginePowerSource::PantographSelector pantograph:
             {RailVehicleEnginePowerSource::PANTOGRAPH_FIRST, RailVehicleEnginePowerSource::PANTOGRAPH_SECOND}) {
            const Vector<Part> &arms = p_visual.pantograph_arms[pantograph];
            if (arms.size() != PANTOGRAPH_ELEMENTS) {
                continue;
            }
            const Vector2 raise = server->vehicle_get_pantograph_raise(p_vehicle, pantograph);
            const double lower = raise.x;
            const double upper = raise.y;
            const double angles[PANTOGRAPH_ELEMENTS] = {-lower, lower, lower + upper, -(lower + upper), -upper};
            for (int index = 0; index < PANTOGRAPH_ELEMENTS; ++index) {
                _pose(p_visual, arms[index], Basis(Vector3(1.0, 0.0, 0.0), static_cast<real_t>(angles[index])));
            }
        }
    }

    // TDynamicObject::UpdateWiper() (DynObj.cpp:716-731): both arms swing by the wiper angle, the
    // blade swings back by it to stay upright; every other wiper is mirrored.
    void RailVehicleRenderingServer::_pose_wipers(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleWipers> wipers =
                component<RailVehicleWipers>(p_vehicle, VehicleComponentType::COMPONENT_WIPERS);
        if (p_visual.wiper_arms.is_empty() || wipers.is_null()) {
            return;
        }
        const PackedFloat64Array positions = wipers->get_sweep_positions();
        if (positions == p_visual.wiper_positions) {
            return;
        }
        p_visual.wiper_positions = positions;
        const double wiper_angle = Math::deg_to_rad(wipers->get_angle());
        for (int wiper = 0; wiper < positions.size() &&
                            (static_cast<int64_t>(wiper) + 1) * WIPER_ELEMENTS <= p_visual.wiper_arms.size();
             ++wiper) {
            // the state tells the way out (0..1) from the way back (1..2)
            double sweep = positions[wiper] > 1.0 ? positions[wiper] - 1.0 : positions[wiper];
            // smoothInterpolate() (utilities.h:324)
            sweep = sweep * sweep * (3.0 - (2.0 * sweep));
            const double angle = (wiper % 2 == 1 ? -wiper_angle : wiper_angle) * sweep;
            for (int element = 0; element < WIPER_ELEMENTS; ++element) {
                _pose(p_visual, p_visual.wiper_arms[(wiper * WIPER_ELEMENTS) + element],
                      Basis(Vector3(0.0, 1.0, 0.0), static_cast<real_t>(element == WIPER_BLADE ? -angle : angle)));
            }
        }
    }

    // TDynamicObject::UpdateMirror() (DynObj.cpp:748-766): the mirrors at the end of the occupied
    // cab turn out about their vertical axis by MirrorMaxShift, as far as their side is unfolded.
    // Odd mirrors are on the left, even on the right, and one at the front end of the model is
    // the original's offset().z > 0 (DynObj.cpp:5903) - in the model's own frame.
    void RailVehicleRenderingServer::_pose_mirrors(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleDoors> doors =
                component<RailVehicleDoors>(p_vehicle, VehicleComponentType::COMPONENT_DOORS);
        if (p_visual.mirrors.is_empty() || doors.is_null()) {
            return;
        }
        const double left = doors->get_mirror_left_position();
        const double right = doors->get_mirror_right_position();
        const RailVehicleCabinKind::Kind driver_cabin = driver_cabin_kind(p_vehicle);
        if (left == p_visual.mirror_left && right == p_visual.mirror_right && driver_cabin == p_visual.mirror_cabin) {
            return;
        }
        p_visual.mirror_left = left;
        p_visual.mirror_right = right;
        p_visual.mirror_cabin = driver_cabin;
        const double max_shift = Math::deg_to_rad(doors->get_mirror_max_shift());
        const Transform3D model_frame = p_visual.model_transform.affine_inverse();
        for (int index = 0; index < p_visual.mirrors.size(); ++index) {
            const Part &mirror = p_visual.mirrors[index];
            const bool front = model_frame.xform(mirror.rest.origin).z > 0.0;
            const bool active = driver_cabin == (front ? RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT
                                                       : RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR);
            const double angle = active ? max_shift * (index % 2 == 1 ? right : left) : 0.0;
            _pose(p_visual, mirror, Basis(Vector3(0.0, 1.0, 0.0), static_cast<real_t>(angle)));
        }
    }

    // TDynamicObject::UpdateDoorTranslate/Rotate/Fold/Plug and UpdatePlatformTranslate/Rotate
    // (DynObj.cpp:561-693): door n+1 is on the left when odd, on the right when even; a door slides
    // along its z, turns about its x by its position in degrees, folds - itself and its first submodel
    // the other way twice as far about their z, that one's about y - or plugs out and slides; a step
    // slides along its x or turns about its y, by its share of the step range.
    void RailVehicleRenderingServer::_pose_doors(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleDoors> doors =
                component<RailVehicleDoors>(p_vehicle, VehicleComponentType::COMPONENT_DOORS);
        if ((p_visual.doors.is_empty() && p_visual.door_steps.is_empty()) || doors.is_null()) {
            return;
        }
        const double positions[2] = {doors->get_left_position(), doors->get_right_position()};
        const double step_positions[2] = {doors->get_left_step_position(), doors->get_right_step_position()};
        if (positions[0] == p_visual.door_positions[0] && positions[1] == p_visual.door_positions[1] &&
            step_positions[0] == p_visual.door_step_positions[0] &&
            step_positions[1] == p_visual.door_step_positions[1]) {
            return;
        }
        for (int side = 0; side < 2; ++side) {
            p_visual.door_positions[side] = positions[side];
            p_visual.door_step_positions[side] = step_positions[side];
        }
        const double range_out = doors->get_max_shift_plug();
        for (int64_t first = 0; first < p_visual.doors.size(); first += DOOR_ELEMENTS) {
            // door n+1 odd: the left side, positions[0]
            const auto position = static_cast<real_t>(positions[(first / DOOR_ELEMENTS) % 2]);
            const Part &leaf = p_visual.doors[first];
            switch (doors->get_type()) {
                case RailVehicleDoors::TYPE_SHIFT:
                    _pose(p_visual, leaf, Transform3D(Basis(), Vector3(0.0, 0.0, position)));
                    break;
                case RailVehicleDoors::TYPE_ROTATE:
                    _pose(p_visual, leaf, Basis(Vector3(1.0, 0.0, 0.0), Math::deg_to_rad(position)));
                    break;
                case RailVehicleDoors::TYPE_FOLD:
                    _pose(p_visual, leaf, Basis(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(position)));
                    _pose(p_visual, p_visual.doors[first + 1],
                          Basis(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(-2 * position)));
                    _pose(p_visual, p_visual.doors[first + 2],
                          Basis(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(position)));
                    break;
                case RailVehicleDoors::TYPE_PLUG:
                    _pose(p_visual, leaf,
                          Transform3D(
                                  Basis(), Vector3(static_cast<real_t>(MIN(position * 2.0, range_out)), 0.0,
                                                   static_cast<real_t>(MAX(0.0, position - (range_out * 0.5))))));
                    break;
            }
        }
        const double step_range = doors->get_platform_max_shift();
        for (int step = 0; step < p_visual.door_steps.size(); ++step) {
            const auto shift = static_cast<real_t>(step_range * step_positions[step % 2]);
            const Part &platform = p_visual.door_steps[step];
            if (doors->get_platform_type() == RailVehicleDoors::PLATFORM_TYPE_ROTATE) {
                _pose(p_visual, platform, Basis(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(shift)));
            } else {
                _pose(p_visual, platform, Transform3D(Basis(), Vector3(shift, 0.0, 0.0)));
            }
        }
    }

    // The pendulums swing about their x by the amplitude [deg] times the cosine of the engine's turn
    // (DynObj.cpp:1121-1125)
    void RailVehicleRenderingServer::_pose_pendulums(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleEngine> engine =
                component<RailVehicleEngine>(p_vehicle, VehicleComponentType::COMPONENT_ENGINE);
        if (p_visual.pendulums.is_empty() || engine.is_null()) {
            return;
        }
        const Basis swing(
                Vector3(1.0, 0.0, 0.0),
                static_cast<real_t>(Math::deg_to_rad(p_visual.pendulum_amplitude * Math::cos(engine->get_angle()))));
        for (const Part &pendulum: p_visual.pendulums) {
            _pose(p_visual, pendulum, swing);
        }
    }

    // Original engine: TDynamicObject::GetPneumatic() (DynObj.cpp:395) - which hoses the model has
    // at that end: 1 left, 2 right (the "r" variant), 3 both; AirCoupler::GetStatus()
    // (AirCoupler.cpp:30) tells a slanted (_xon) from a straight (_on) connected submodel
    RailVehicleRenderingServer::PneumaticLayout RailVehicleRenderingServer::_pneumatic_layout(
            const Visual &p_visual, const RailVehicleController::CouplerEnd p_end,
            const RailVehicleRenderingServer::PneumaticLine p_line) const {
        const auto has_connected = [&](const String &p_name) {
            return p_visual.coupler_submodels.has(p_name + String("_xon")) ||
                   p_visual.coupler_submodels.has(p_name + String("_on"));
        };
        const String name = String(PNEUMATIC_SUBMODELS[p_line]) + itos(p_end + 1);
        const bool left = has_connected(name);
        const bool right = has_connected(name + String("r"));
        if (left && right) {
            return PNEUMATIC_LAYOUT_BOTH;
        }
        if (left) {
            return PNEUMATIC_LAYOUT_LEFT;
        }
        return right ? PNEUMATIC_LAYOUT_RIGHT : PNEUMATIC_LAYOUT_NONE;
    }

    // Original engine: TDynamicObject::SetPneumatic() (DynObj.cpp:430) - picks the hose submodel
    // matching the layout of the vehicle coupled at that end: 1 straight, 2 slanted, 3 slanted "r",
    // 4 straight "r"
    RailVehicleRenderingServer::CouplerVariant RailVehicleRenderingServer::_pneumatic_variant(
            const RID &p_vehicle, const Visual &p_visual, const RailVehicleController::CouplerEnd p_end,
            const RailVehicleRenderingServer::PneumaticLine p_line) const {
        const Ref<RailVehicleBuffCoupl> coupler = couplers(p_vehicle);
        RailVehicleServer *server = RailVehicleServer::get_instance();
        if (coupler.is_null() || server == nullptr) {
            return COUPLER_VARIANT_OFF;
        }
        const PneumaticLayout own = _pneumatic_layout(p_visual, p_end, p_line);
        PneumaticLayout other = PNEUMATIC_LAYOUT_NONE;
        // the vehicles coupled beyond p_end, from the farthest one back through this one: the
        // neighbour is the one just before it
        const TypedArray<RID> coupled =
                server->vehicle_get_coupled(p_vehicle, p_end, RailVehicleController::COUPLING_FLAG_COUPLER);
        if (const int64_t own_index = coupled.find(p_vehicle); own_index > 0) {
            if (const Visual *neighbour = vehicles.getptr(coupled[own_index - 1]); neighbour != nullptr) {
                other = _pneumatic_layout(*neighbour, coupler->get_connected_end(p_end), p_line);
            }
        }
        if (own == other) {
            switch (own) {
                case PNEUMATIC_LAYOUT_LEFT:
                    return COUPLER_VARIANT_XON;
                case PNEUMATIC_LAYOUT_RIGHT:
                    return COUPLER_VARIANT_RIGHT_XON;
                case PNEUMATIC_LAYOUT_BOTH:
                    return coupler->is_coupling_owner(p_end) ? COUPLER_VARIANT_ON : COUPLER_VARIANT_RIGHT_ON;
                default:
                    return COUPLER_VARIANT_OFF;
            }
        }
        if (own == PNEUMATIC_LAYOUT_BOTH) {
            return other == PNEUMATIC_LAYOUT_LEFT ? COUPLER_VARIANT_RIGHT_ON : COUPLER_VARIANT_ON;
        }
        if (own == PNEUMATIC_LAYOUT_RIGHT) {
            return COUPLER_VARIANT_RIGHT_ON;
        }
        return own == PNEUMATIC_LAYOUT_LEFT ? COUPLER_VARIANT_ON : COUPLER_VARIANT_OFF;
    }

    // Original engine: AirCoupler::Update() (AirCoupler.cpp:83)
    void RailVehicleRenderingServer::_show_air_coupler(
            const Visual &p_visual, const String &p_name, const bool p_on, const bool p_xon) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const bool states[] = {p_on, !(p_on || p_xon), p_xon};
        for (int index = 0; index < static_cast<int>(COUPLER_SUFFIXES.size()); ++index) {
            const String name = p_name + String(COUPLER_SUFFIXES[index]);
            if (p_visual.coupler_submodels.has(name)) {
                models->instance_set_submodel_visible(p_visual.model, name, states[index]);
            }
        }
    }

    // Original engine: coupler and hose submodel visibility (DynObj.cpp:758-925, bnewAirCouplers branch)
    void RailVehicleRenderingServer::_update_couplers(const RID &p_vehicle, Visual &p_visual) {
        const Ref<RailVehicleBuffCoupl> coupler = couplers(p_vehicle);
        if (p_visual.coupler_submodels.is_empty() || coupler.is_null()) {
            return;
        }
        CouplerVariant variants[2][COUPLER_PART_COUNT];
        int64_t state = 0;
        for (const RailVehicleController::CouplerEnd end:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            // _on for the vehicle that draws the coupler, _xon (or _off without it) for the other
            if (!coupler->is_coupled(end)) {
                variants[end][0] = COUPLER_VARIANT_OFF;
            } else if (coupler->is_coupling_owner(end)) {
                variants[end][0] = COUPLER_VARIANT_ON;
            } else {
                variants[end][0] = COUPLER_VARIANT_XON;
            }
            variants[end][1] = coupler->is_brake_hose_connected(end)
                                       ? _pneumatic_variant(p_vehicle, p_visual, end, PNEUMATIC_LINE_BRAKE)
                                       : COUPLER_VARIANT_OFF;
            variants[end][2] = coupler->is_main_hose_connected(end)
                                       ? _pneumatic_variant(p_vehicle, p_visual, end, PNEUMATIC_LINE_MAIN)
                                       : COUPLER_VARIANT_OFF;
            for (const CouplerVariant variant: variants[end]) {
                state = (state * COUPLER_VARIANT_COUNT) + variant;
            }
        }
        if (state == p_visual.coupler_state) {
            return;
        }
        p_visual.coupler_state = state;
        for (const RailVehicleController::CouplerEnd end:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            // the original numbers the couplers from 1 (coupler1, coupler2)
            const String number = itos(end + 1);
            _show_air_coupler(
                    p_visual, "coupler" + number, variants[end][0] == COUPLER_VARIANT_ON,
                    variants[end][0] == COUPLER_VARIANT_XON &&
                            p_visual.coupler_submodels.has("coupler" + number + "_xon"));
            for (const PneumaticLine line: {PNEUMATIC_LINE_BRAKE, PNEUMATIC_LINE_MAIN}) {
                const String name = String(PNEUMATIC_SUBMODELS[line]) + number;
                const CouplerVariant variant = variants[end][line + 1];
                _show_air_coupler(p_visual, name, variant == COUPLER_VARIANT_ON, variant == COUPLER_VARIANT_XON);
                _show_air_coupler(
                        p_visual, name + String("r"), variant == COUPLER_VARIANT_RIGHT_ON,
                        variant == COUPLER_VARIANT_RIGHT_XON);
            }
        }
    }

    void RailVehicleRenderingServer::_update_lights(const RID &p_vehicle, Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const Ref<RailVehicleLighting> lighting =
                component<RailVehicleLighting>(p_vehicle, VehicleComponentType::COMPONENT_LIGHTING);
        if (models == nullptr || lighting.is_null() || !p_visual.model.is_valid()) {
            return;
        }
        bool changed = false;
        for (const LightStateBinding &binding: LIGHT_STATE_BINDINGS) {
            if (!p_visual.lights.has(binding.light_name)) {
                continue;
            }
            const bool enabled = (lighting.ptr()->*binding.is_enabled)();
            if (bool(p_visual.lights[binding.light_name]) != enabled) {
                p_visual.lights[binding.light_name] = enabled;
                changed = true;
            }
        }
        if (changed) {
            models->instance_set_lights_state(p_visual.model, p_visual.lights);
        }
        // Headlights dimmed (DynamicObject->DimHeadlights)
        if (const bool dimmed = lighting->get_headlights_dimmed(); dimmed != p_visual.headlights_dimmed) {
            p_visual.headlights_dimmed = dimmed;
            Dictionary lights_dimmed;
            for (const LightStateBinding &binding: LIGHT_STATE_BINDINGS) {
                if (binding.dimming == LIGHT_DIMMING_HEADLIGHT) {
                    lights_dimmed[binding.light_name] = dimmed;
                }
            }
            models->instance_set_lights_dimmed(
                    p_visual.model, lights_dimmed, static_cast<float>(lighting->get_head_light_dimmed_multiplier()));
        }
    }

    /// Drives the particle emitters the model carries. The rate follows the original
    /// (smoke_source::update(), particles.cpp:172-211) and the opacity its dizel_fill
    /// (particles.cpp:330), but only for a diesel: the original runs these branches for every
    /// engine type and reads the diesel-electric characteristic even on an electric. A vehicle
    /// without a diesel engine gets the template's own rate, like a scenery chimney - the server
    /// starts a vehicle's emitters silent, so even that has to be said.
    void RailVehicleRenderingServer::_update_smoke(const RID &p_vehicle, const Visual &p_visual) const {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const Ref<RailVehicleController> vehicle =
                vehicle_server != nullptr
                        ? Ref<RailVehicleController>(vehicle_server->vehicle_get_controller(p_vehicle))
                        : Ref<RailVehicleController>();
        if (models == nullptr || vehicle.is_null() || !p_visual.model.is_valid()) {
            return;
        }
        const Ref<RailVehicleEngine> engine =
                component<RailVehicleEngine>(p_vehicle, VehicleComponentType::COMPONENT_ENGINE);
        const Ref<RailVehicleDieselEngine> diesel_engine = engine;
        const Ref<RailVehicleElectricEngine> electric_engine = engine;
        const int engine_type = engine.is_valid() ? engine->get_type() : RailVehicleEngine::NONE;
        if (engine_type != RailVehicleEngine::DIESEL && engine_type != RailVehicleEngine::DIESEL_ELECTRIC) {
            models->instance_set_smoke_intensity(p_visual.model, 1.0);
            return;
        }
        // particles.cpp:188-205
        static constexpr double SPINUP_RATE = 4.0;
        static constexpr double SMOKE_SCALE = 0.01;
        static constexpr double LOAD_SCALE = 0.005;
        static constexpr double RATE_SCALE = 0.02;
        const double revolutions = engine->get_rpm_count(); // rev/s, as the Mover keeps enrot
        const double max_rpm = diesel_engine.is_valid() ? diesel_engine->get_max_rpm() : 0.0;
        const double power = engine->get_power(); // kW
        const double current = electric_engine.is_valid() ? electric_engine->get_motor_current() : 0.0;
        const double direction = static_cast<double>(vehicle->get_direction_absolute());

        double intensity;
        if (diesel_engine.is_valid() && diesel_engine->get_spinup()) {
            intensity = revolutions / SPINUP_RATE * SMOKE_SCALE;
        } else {
            // The original compares rev/min against rev/s (particles.cpp:196), which leaves the
            // deficit nearly constant and makes the rate track the engine power. Kept as it is:
            // reading both in rev/min would stop a diesel from smoking at full revs, which is
            // where it smokes most.
            const double revolutions_deficit = (max_rpm - revolutions) / LibMaszynaUnits::SECONDS_PER_MINUTE;
            const double load = power * LOAD_SCALE;
            if (Math::is_zero_approx(direction) || Math::is_zero_approx(current)) {
                intensity = revolutions_deficit * RATE_SCALE * load;
            } else {
                intensity = revolutions_deficit * (Math::sqrt(Math::abs(current)) * SMOKE_SCALE) * RATE_SCALE * load;
            }
        }
        // dizel_fill scales the opacity of a newly born particle in the original
        // (particles.cpp:330). Godot has no channel for that which does not also reach the
        // particles already in the air, so it scales how many are born instead - the plume thins
        // out rather than stepping down as a whole (see FINDINGS.md).
        const double fill = CLAMP(diesel_engine.is_valid() ? diesel_engine->get_fill() : 0.0, 0.0, 1.0);
        models->instance_set_smoke_intensity(p_visual.model, static_cast<float>(CLAMP(intensity, 0.0, 1.0) * fill));
    }

    /// A vehicle far from the camera is rendered from RenderingServer instances instead of a node
    /// hierarchy: nothing animates at that distance, and the hierarchy is what costs - hundreds of
    /// Node3Ds per vehicle to walk, notify and propagate a transform through, times the hundreds of
    /// vehicles a scenery runs. Its simulation is untouched. Note the OPTIMIZED backend does not
    /// render SUBMODEL_FREE_SPOTLIGHT (see TODO.md), so a distant vehicle loses its lights.
    ///
    /// Beyond the streaming's draw distance a vehicle is not drawn at all: the models built from its
    /// appearance are built when it comes within it - a scenery's hundreds of vehicles all built at
    /// load filled it with the materials and textures of vehicles nobody saw - and freed when it
    /// leaves. Without the streaming's camera - while a scenery loads - nothing is decided; the
    /// editor builds them at once.
    void RailVehicleRenderingServer::_update_detail(const RID &p_vehicle, Visual &p_visual) {
        const SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        const Node3D *node = _node(p_visual);
        // the nodes of a vehicle drawn in detail are built under its scene node
        if (node == nullptr || !node->is_inside_tree() || !p_visual.placed) {
            return;
        }
        const bool has_camera = streaming != nullptr && streaming->streaming_has_camera();
        const double distance =
                has_camera ? p_visual.transform.origin.distance_to(streaming->streaming_get_camera_position()) : 0.0;
        if (p_visual.appearance.is_valid() && !p_visual.appearance->get_model_filename().is_empty()) {
            const bool at_once = p_visual.editable || Engine::get_singleton()->is_editor_hint();
            bool drawn = at_once || p_visual.model.is_valid();
            if (!at_once && has_camera) {
                const float draw_distance = streaming->streaming_get_draw_distance();
                const float hysteresis = MAX(DETAIL_HYSTERESIS_MIN, draw_distance * DETAIL_HYSTERESIS);
                drawn = p_visual.model.is_valid() ? distance <= draw_distance + hysteresis : distance <= draw_distance;
            }
            // a model that could not be loaded is not loaded again - it would stay a pending build,
            // and the loading screen waits for the builds within the draw distance
            if (drawn && !p_visual.model.is_valid() && !p_visual.model_missing) {
                if (at_once) {
                    _build_models(p_vehicle, p_visual);
                } else if (!p_visual.build_pending) {
                    int index = 0;
                    while (index < pending_builds.size() && pending_builds[index].distance <= distance) {
                        ++index;
                    }
                    pending_builds.insert(index, PendingBuild{p_vehicle, distance});
                    p_visual.build_pending = true;
                }
                return;
            }
            if (!drawn) {
                _cancel_build(p_vehicle, p_visual);
            }
            if (!drawn && p_visual.model.is_valid()) {
                if (p_visual.detailed) {
                    _set_detailed(p_vehicle, p_visual, false);
                }
                _free_models(p_visual);
                _build_load(p_vehicle, p_visual);
                _bind_parts(p_vehicle, p_visual);
                _register_pickable(p_vehicle, p_visual);
                return;
            }
        }
        if (!p_visual.model.is_valid()) {
            return;
        }
        bool detailed = p_visual.editable;
        if (!detailed) {
            if (!has_camera) {
                return;
            }
            const float hysteresis = MAX(DETAIL_HYSTERESIS_MIN, detail_distance * DETAIL_HYSTERESIS);
            detailed = p_visual.detailed ? distance <= detail_distance : distance <= detail_distance - hysteresis;
        }
        if (detailed != p_visual.detailed) {
            _set_detailed(p_vehicle, p_visual, detailed);
        }
    }

    /* Built, the vehicle takes its detail at once: one near the camera is drawn as nodes before its
     * build counts as done (builds_get_pending_count()), not at the sweep's next turn */
    void RailVehicleRenderingServer::_build_models(const RID &p_vehicle, Visual &p_visual) {
        // in the game's log before and after: a crash with no message is found by the last line
        const String model_path =
                p_visual.appearance->get_data_path().path_join(p_visual.appearance->get_model_filename());
        UtilityFunctions::print("[RailVehicleRendering] building ", model_path);
        _create_models(p_vehicle, p_visual);
        _bind_parts(p_vehicle, p_visual);
        _build_load(p_vehicle, p_visual);
        _update_detail(p_vehicle, p_visual);
        UtilityFunctions::print("[RailVehicleRendering] built ", model_path);
    }

    void RailVehicleRenderingServer::_cancel_build(const RID &p_vehicle, Visual &p_visual) {
        if (!p_visual.build_pending) {
            return;
        }
        p_visual.build_pending = false;
        for (int index = 0; index < pending_builds.size(); ++index) {
            if (pending_builds[index].vehicle == p_vehicle) {
                pending_builds.remove_at(index);
                return;
            }
        }
    }

    void RailVehicleRenderingServer::_set_detailed(const RID &p_vehicle, Visual &p_visual, const bool p_detailed) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        Node3D *node = _node(p_visual);
        ERR_FAIL_NULL(models);
        ERR_FAIL_NULL(node);
        p_visual.detailed = p_detailed;
        // the models this server built get a node of its own to be built under, there only while
        // the vehicle is drawn in detail; the ones handed over have their owner's. An editable
        // vehicle's holder is shown in the Scene dock - a node's internal mode is chosen only when
        // it is added - and owned as the scene node is, which its nodes take (E3DNodesBackend)
        if (p_detailed && p_visual.own_models) {
            Node3D *holder = memnew(Node3D);
            holder->set_name(p_visual.appearance->get_model_filename().get_file().get_basename());
            node->add_child(holder, false, p_visual.editable ? Node::INTERNAL_MODE_DISABLED : Node::INTERNAL_MODE_BACK);
            if (p_visual.editable) {
                holder->set_owner(node->get_owner() != nullptr ? node->get_owner() : node);
            }
            holder->set_global_transform(p_visual.transform);
            p_visual.holder = ObjectID(holder->get_instance_id());
            models->instance_attach_object_instance_id(p_visual.model, p_visual.holder);
            if (p_visual.low_poly.is_valid()) {
                models->instance_attach_object_instance_id(p_visual.low_poly, p_visual.holder);
            }
        }
        const E3DRenderingServer::Instancer instancer = detail_instancer(p_visual.detailed, p_visual.editable);
        models->instance_set_instancer(p_visual.model, instancer);
        if (p_visual.low_poly.is_valid()) {
            models->instance_set_instancer(p_visual.low_poly, instancer);
        }
        if (Node *holder = Object::cast_to<Node>(ObjectDB::get_instance(p_visual.holder));
            !p_detailed && holder != nullptr) {
            // the nodes built under it went with the instancer that built them; out of the tree at
            // once, so that the holder made next does not take its name
            node->remove_child(holder);
            holder->queue_free();
            p_visual.holder = ObjectID();
        }
        _register_pickable(p_vehicle, p_visual);
        if (p_detailed) {
            _place(p_vehicle, p_visual);
        }
    }

    // Original engine: every low-poly cab is drawn, but in the view from inside the player's
    // vehicle, where the "cabN" of the occupied cab is hidden - or all of them with jointcabs: -
    // so the hi-fi cab doesn't overlap it; a cab without a hi-fi model hides none
    // (DynObj.cpp:1389-1397). Which view that is belongs to whoever shows the cab's interior
    // (vehicle_set_visible_low_poly_cabins()).
    void RailVehicleRenderingServer::_update_low_poly_cabs(const RID &p_vehicle, const Visual &p_visual) const {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        if (models == nullptr || !p_visual.low_poly.is_valid() || p_visual.appearance.is_null()) {
            return;
        }
        const int hidden = low_poly_cab(driver_cabin_kind(p_vehicle));
        const bool joint_cabs = p_visual.appearance->get_joint_cabs();
        for (int cab = 0; cab < static_cast<int>(LOW_POLY_CABS.size()); ++cab) {
            const String name = LOW_POLY_CABS[cab];
            if (models->instance_has_submodel(p_visual.low_poly, name)) {
                models->instance_set_submodel_visible(
                        p_visual.low_poly, name, p_visual.low_poly_cabs_visible || (!joint_cabs && cab != hidden));
            }
        }
    }

    /* Where the cargo sits: the original sinks it into the body as the vehicle empties, lerping
     * from the cargo's own offset_min to zero with how full it is (DynObj.cpp:3070-3080), and
     * leaves it alone when that cargo declares no offset. Both numbers are the vehicle's
     * configuration, so they are taken again when it changes. */
    void RailVehicleRenderingServer::_update_load(const RID &p_vehicle, Visual &p_visual) {
        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (models == nullptr || vehicle_server == nullptr || !p_visual.load.is_valid()) {
            return;
        }
        const Ref<RailVehicleController> vehicle = vehicle_server->vehicle_get_controller(p_vehicle);
        const Ref<RailVehicleLoad> load = component<RailVehicleLoad>(p_vehicle, VehicleComponentType::COMPONENT_LOAD);
        p_visual.load_height = 0.0;
        if (vehicle.is_valid() && load.is_valid()) {
            const TypedArray<String> accepted = load->get_accepted_loads();
            const TypedArray<float> offsets = load->get_minimum_load_offsets();
            const String cargo = vehicle->get_load_name().to_lower();
            for (int index = 0; index < accepted.size() && index < offsets.size(); ++index) {
                if (String(accepted[index]).to_lower() == cargo) {
                    const double max_load = load->get_max_load();
                    const double fill = max_load > 0.0 ? CLAMP(vehicle->get_load_amount() / max_load, 0.0, 1.0) : 0.0;
                    p_visual.load_height = Math::lerp(double(offsets[index]), 0.0, fill);
                    break;
                }
            }
        }
        models->instance_set_transform(
                p_visual.load,
                p_visual.transform * p_visual.model_transform *
                        Transform3D(Basis(), Vector3(0.0, static_cast<real_t>(p_visual.load_height), 0.0)));
    }

    /* The space the player finds the vehicle in - a box the size of the model, in the space of its
     * scene node's world; whose it is, detection_area_get_vehicle() says */
    void RailVehicleRenderingServer::_update_detection_area(const RID &p_vehicle, Visual &p_visual) {
        PhysicsServer3D *physics = PhysicsServer3D::get_singleton();
        const E3DRenderingServer *models = E3DRenderingServer::get_instance();
        const Node3D *node = _node(p_visual);
        if (Engine::get_singleton()->is_editor_hint() || physics == nullptr || models == nullptr || node == nullptr ||
            !node->is_inside_tree() || !p_visual.model.is_valid() || !p_visual.placed) {
            return;
        }
        const AABB aabb = models->instance_get_aabb(p_visual.model);
        if (aabb.size == Vector3()) {
            return;
        }
        if (!p_visual.detection_area.is_valid()) {
            p_visual.detection_area = physics->area_create();
            p_visual.detection_shape = physics->box_shape_create();
            physics->area_add_shape(p_visual.detection_area, p_visual.detection_shape);
            physics->area_set_monitorable(p_visual.detection_area, true);
            area_vehicles[p_visual.detection_area] = p_vehicle;
            physics->area_set_space(p_visual.detection_area, node->get_world_3d()->get_space());
        }
        physics->shape_set_data(p_visual.detection_shape, aabb.size * 0.5);
        physics->area_set_shape_transform(p_visual.detection_area, 0, Transform3D(Basis(), aabb.get_center()));
        physics->area_set_transform(p_visual.detection_area, p_visual.transform * p_visual.model_transform);
    }

    /// The model is clicked in free camera (SceneryHUDMouseServer) while it is detailed, so only
    /// the vehicles near the camera are tested under the cursor
    void RailVehicleRenderingServer::_register_pickable(const RID &p_vehicle, Visual &p_visual) {
        SceneryHUDMouseServer *mouse = SceneryHUDMouseServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (Engine::get_singleton()->is_editor_hint() || mouse == nullptr || vehicle_server == nullptr) {
            return;
        }
        mouse->pickable_free(p_visual.pickable);
        p_visual.pickable = RID();
        if (p_visual.detailed && p_visual.model.is_valid()) {
            p_visual.pickable = mouse->vehicle_pickable_create(
                    p_visual.model, vehicle_server->vehicle_get_name(p_vehicle), p_vehicle);
        }
    }

    /// Placed - on loading, its first place on a track - its detail is decided where it stands
    void RailVehicleRenderingServer::_on_vehicle_placed(const RID &p_vehicle) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _place(p_vehicle, *visual);
            _update_detail(p_vehicle, *visual);
        }
    }

    /* A coupling changed at either end: the couplers are drawn again, and the neighbours', whose
     * hoses match this vehicle's (SetPneumatic(), DynObj.cpp:430) */
    void RailVehicleRenderingServer::_on_vehicle_trainset_changed(const RID &p_vehicle) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _update_couplers(p_vehicle, *visual);
        }
        RailVehicleServer *server = RailVehicleServer::get_instance();
        if (server == nullptr) {
            return;
        }
        for (const RailVehicleController::CouplerEnd end:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            const TypedArray<RID> coupled =
                    server->vehicle_get_coupled(p_vehicle, end, RailVehicleController::COUPLING_FLAG_COUPLER);
            const int64_t vehicle_index = coupled.find(p_vehicle);
            if (vehicle_index < 1) {
                continue;
            }
            const RID neighbour = coupled[vehicle_index - 1];
            if (Visual *visual = vehicles.getptr(neighbour); visual != nullptr) {
                _update_couplers(neighbour, *visual);
            }
        }
    }

    void RailVehicleRenderingServer::_on_vehicle_coupler_changed(const RID &p_vehicle, const int64_t p_flag) {
        _on_vehicle_trainset_changed(p_vehicle);
    }

    void RailVehicleRenderingServer::_on_vehicle_coupler_adapter_changed(const RID &p_vehicle, const int64_t p_end) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _update_coupler_adapters(p_vehicle, *visual);
        }
    }

    void RailVehicleRenderingServer::_on_vehicle_config_changed(const RID &p_vehicle) {
        if (Visual *visual = vehicles.getptr(p_vehicle); visual != nullptr) {
            _update_load(p_vehicle, *visual);
            _publish_pantographs(p_vehicle, *visual);
            _place(p_vehicle, *visual);
        }
    }

    void RailVehicleRenderingServer::_on_vehicle_freed(const RID &p_vehicle) {
        vehicle_detach(p_vehicle);
    }

    /* A model built again - another instancer, a stream - is found and drawn anew: its size for
     * the detection area, its lamps as the vehicle has them */
    void RailVehicleRenderingServer::_on_instance_built(const RID &p_instance) {
        const RID *vehicle = model_vehicles.getptr(p_instance);
        Visual *visual = vehicle != nullptr ? vehicles.getptr(*vehicle) : nullptr;
        if (visual == nullptr) {
            return;
        }
        _bind_parts(*vehicle, *visual);
        emit_signal(vehicle_model_built_signal, *vehicle);
    }

    void RailVehicleRenderingServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (tree == nullptr) {
            return;
        }
        processing = p_processing;
        if (p_processing) {
            tree->connect("process_frame", callable_mp(this, &RailVehicleRenderingServer::_process_frame));
            return;
        }
        tree->disconnect("process_frame", callable_mp(this, &RailVehicleRenderingServer::_process_frame));
    }

    /* Every vehicle's detail, lamps and smoke a few times a second, a budget of them a frame; the
     * pantographs, wipers and mirrors of the vehicles drawn in detail every frame, a budget of
     * them a frame; the low-poly interiors still following their roof light. */
    void RailVehicleRenderingServer::_process_frame() {
        if (Engine::get_singleton()->is_editor_hint() || visit_order.is_empty()) {
            return;
        }
        const SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        const double delta = tree != nullptr ? tree->get_root()->get_process_delta_time() : 0.0;
        const int count = static_cast<int>(visit_order.size());

        // the models of the vehicles that came within the draw distance, the nearest first
        const uint64_t deadline = Time::get_singleton()->get_ticks_msec() + BUILD_BUDGET_MSEC;
        while (!pending_builds.is_empty() && Time::get_singleton()->get_ticks_msec() < deadline) {
            const RID vehicle = pending_builds[0].vehicle;
            pending_builds.remove_at(0);
            Visual &visual = vehicles[vehicle];
            visual.build_pending = false;
            _build_models(vehicle, visual);
        }

        slow_elapsed += delta;
        const int slow_budget =
                MIN(count, MAX(1, MIN(MAX_SLOW_UPDATES_PER_FRAME,
                                      static_cast<int>(Math::ceil(count * delta / SLOW_UPDATE_PERIOD)))));
        for (int visited = 0; visited < slow_budget; ++visited) {
            slow_cursor = (slow_cursor + 1) % count;
            const RID vehicle = visit_order[slow_cursor];
            Visual &visual = vehicles[vehicle];
            _update_detail(vehicle, visual);
            _update_lights(vehicle, visual);
            _update_smoke(vehicle, visual);
        }

        for (int visited = 0, looked = 0; visited < MAX_DETAILED_UPDATES_PER_FRAME && looked < count; ++looked) {
            detailed_cursor = (detailed_cursor + 1) % count;
            const RID vehicle = visit_order[detailed_cursor];
            Visual &visual = vehicles[vehicle];
            if (!visual.detailed || !visual.model.is_valid()) {
                continue;
            }
            ++visited;
            _pose_pantographs(vehicle, visual);
            _pose_wipers(vehicle, visual);
            _pose_mirrors(vehicle, visual);
            _pose_doors(vehicle, visual);
            _pose_pendulums(vehicle, visual);
            _update_lights(vehicle, visual);
            _send_poses(visual);
        }

        E3DRenderingServer *models = E3DRenderingServer::get_instance();
        for (int index = static_cast<int>(fading.size()) - 1; index >= 0 && models != nullptr; --index) {
            Visual *visual = vehicles.getptr(fading[index]);
            if (visual == nullptr || visual->appearance.is_null() || !visual->low_poly.is_valid()) {
                fading.remove_at(index);
                continue;
            }
            const double full_energy = visual->appearance->get_low_poly_emission_energy();
            const double fade_time = visual->appearance->get_low_poly_emission_fade_time();
            // jointcabs: one light for every cab (Train.cpp:5249-5256)
            const bool joint_cabs = visual->appearance->get_joint_cabs();
            double joint_level = 0.0;
            for (const double level: visual->cab_light_levels) {
                joint_level = MAX(joint_level, level);
            }
            bool faded = true;
            for (int cab = 0; cab < static_cast<int>(LOW_POLY_CABS.size()); ++cab) {
                const double target = (joint_cabs ? joint_level : visual->cab_light_levels[cab]) * full_energy;
                double &energy = visual->cab_light_energies[cab];
                const double step = fade_time > 0.0 ? full_energy * delta / fade_time : Math::abs(target - energy);
                energy += CLAMP(target - energy, -step, step);
                models->instance_set_submodel_emission_energy(
                        visual->low_poly, LOW_POLY_CABS[cab], static_cast<float>(energy));
                faded = faded && Math::is_equal_approx(energy, target);
            }
            if (faded) {
                fading.remove_at(index);
            }
        }
    }
} // namespace godot
