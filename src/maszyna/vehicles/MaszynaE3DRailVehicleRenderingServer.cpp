#include "MaszynaE3DRailVehicleRenderingServer.hpp"
#include "game_data/GameDataServer.hpp"
#include "maszyna/e3d/MaszynaE3DRenderingServer.hpp"
#include "maszyna/e3d/MaszynaE3DSubModel.hpp"
#include "maszyna/vehicles/MaszynaRailVehicleAppearance.hpp"
#include "rendering/ModelRenderingServer.hpp"
#include "resources/ResourceLazyLoader.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleLighting.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>

namespace godot {
    MaszynaE3DRailVehicleRenderingServer *MaszynaE3DRailVehicleRenderingServer::singleton = nullptr;

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

    static Ref<RailVehicleBuffCoupl> couplers(const RID &p_vehicle) {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        return server != nullptr ? Ref<RailVehicleBuffCoupl>(server->vehicle_component_get(
                                           p_vehicle, RailVehicleComponentType::COMPONENT_BUFFERS))
                                 : Ref<RailVehicleBuffCoupl>();
    }

    template<typename T>
    static Ref<T> component(const RID &p_vehicle, const VehicleComponentType::Type p_type) {
        const VehicleServer *server = VehicleServer::get_instance();
        return server != nullptr ? Ref<T>(server->vehicle_component_get(p_vehicle, p_type)) : Ref<T>();
    }

    MaszynaE3DRailVehicleRenderingServer *MaszynaE3DRailVehicleRenderingServer::get_instance() {
        return singleton;
    }

    MaszynaE3DRailVehicleRenderingServer::MaszynaE3DRailVehicleRenderingServer() {
        singleton = this;
        if (VehicleServer *vehicle_server = VehicleServer::get_instance(); vehicle_server != nullptr) {
            vehicle_server->connect(
                    VehicleServer::vehicle_config_changed_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_config_changed));
        }
        if (RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance(); rail_vehicles != nullptr) {
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_driver_cabin_changed_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_driver_cabin_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_trainset_changed_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_trainset_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_attached_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_coupler_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_detached_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_coupler_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_adapter_attached_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_coupler_adapter_changed));
            rail_vehicles->connect(
                    RailVehicleServer::vehicle_coupler_adapter_removed_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_vehicle_coupler_adapter_changed));
        }
        if (GameDataServer *game_data = GameDataServer::get_instance(); game_data != nullptr) {
            game_data->connect(
                    GameDataServer::data_reload_requested_signal,
                    callable_mp(this, &MaszynaE3DRailVehicleRenderingServer::_on_data_reload_requested));
        }
    }

    /* The vehicles go while this renderer's hooks still answer - the base's own destructor would
     * find none of them */
    MaszynaE3DRailVehicleRenderingServer::~MaszynaE3DRailVehicleRenderingServer() {
        _detach_all();
        singleton = nullptr;
    }

    void MaszynaE3DRailVehicleRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("vehicle_set_load_model", "vehicle", "data_path", "model_filename"),
                &MaszynaE3DRailVehicleRenderingServer::vehicle_set_load_model);
        ClassDB::bind_method(
                D_METHOD("cabin_set_light_level", "cabin", "level"),
                &MaszynaE3DRailVehicleRenderingServer::cabin_set_light_level);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_attached(const RID &p_vehicle) {
        maszyna_vehicles[p_vehicle] = MaszynaVisual();
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_detaching(const RID &p_vehicle) {
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(maszyna);
        if (ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance(); lazy_loader != nullptr) {
            for (const RID &resource:
                 {maszyna->load_resource, maszyna->coupler_adapter_resources[0],
                  maszyna->coupler_adapter_resources[1]}) {
                if (resource.is_valid()) {
                    lazy_loader->resource_free(resource);
                }
            }
        }
        if (maszyna->load.is_valid()) {
            ModelRenderingServer::get_instance()->instance_free(maszyna->load);
        }
        maszyna_vehicles.erase(p_vehicle);
        fading.erase(p_vehicle);
    }

    /* Models of its own: the adapters' files are registered with the appearance's; a model handed
     * over is its owner's, built already - its pantographs are read off it at once */
    void MaszynaE3DRailVehicleRenderingServer::_vehicle_appearance_changed(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        if (visual->appearance.is_valid() && !visual->appearance->get_model_filename().is_empty()) {
            _register_coupler_adapters(p_vehicle, *maszyna);
            return;
        }
        _publish_pantographs(p_vehicle, *visual);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_add_preload_requests(
            const RID &p_vehicle, Array &p_r_requests) const {
        const Visual *visual = _get_visual(p_vehicle);
        const MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        if (visual == nullptr || maszyna == nullptr) {
            return;
        }
        // the cargo carries no skin of its own (_build_load())
        if (maszyna->load_resource.is_valid()) {
            p_r_requests.push_back(Array::make(maszyna->load_resource, maszyna->load_data_path, PackedStringArray()));
        }
        const PackedStringArray skins =
                visual->appearance.is_valid() ? visual->appearance->get_skins() : PackedStringArray();
        for (const RID &resource: maszyna->coupler_adapter_resources) {
            if (resource.is_valid()) {
                p_r_requests.push_back(Array::make(resource, COUPLER_ADAPTER_MODELS, skins));
            }
        }
    }

    /* What depends on the model file alone - the pantographs' geometry, and the cargo and the
     * adapters of a vehicle built already */
    void MaszynaE3DRailVehicleRenderingServer::_vehicle_preloaded(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        _publish_pantographs(p_vehicle, *visual);
        if (visual->own_models && visual->model.is_valid()) {
            _update_coupler_adapters(p_vehicle, *visual, *maszyna);
            _build_load(p_vehicle, *visual, *maszyna);
        }
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_models_created(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        _update_coupler_adapters(p_vehicle, *visual, *maszyna);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_models_freed(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        if (ModelRenderingServer *models = ModelRenderingServer::get_instance();
            models != nullptr && visual->own_models) {
            for (const RID &instance: maszyna->coupler_adapters) {
                if (instance.is_valid()) {
                    models->instance_free(instance);
                }
            }
        }
        maszyna->coupler_adapters[0] = RID();
        maszyna->coupler_adapters[1] = RID();
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_models_changed(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        _build_load(p_vehicle, *visual, *maszyna);
    }

    /* The submodels its MMD names, the couplers, hoses and lamps the model has, the low-poly cabs
     * hidden and dark, and all of them shown as the vehicle stands now */
    void MaszynaE3DRailVehicleRenderingServer::_vehicle_parts_bound(const RID &p_vehicle) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        maszyna->coupler_submodels.clear();
        maszyna->coupler_state = -1;
        maszyna->lights.clear();
        maszyna->wiper_positions.clear();
        maszyna->mirror_left = -1.0;
        if (visual->appearance.is_null() || models == nullptr || !visual->model.is_valid()) {
            return;
        }
        if (const Ref<MaszynaRailVehicleAppearance> appearance = visual->appearance; appearance.is_valid()) {
            maszyna->pantograph_arms[RailVehicleEnginePowerSource::PANTOGRAPH_FIRST] =
                    _parts(*visual, appearance->get_pantograph_front_arms());
            maszyna->pantograph_arms[RailVehicleEnginePowerSource::PANTOGRAPH_SECOND] =
                    _parts(*visual, appearance->get_pantograph_rear_arms());
            maszyna->wiper_arms = _parts(*visual, appearance->get_wiper_arms());
            maszyna->mirrors = _parts(*visual, appearance->get_mirrors());
            maszyna->doors = _parts(*visual, appearance->get_doors());
            maszyna->door_steps = _parts(*visual, appearance->get_door_steps());
            maszyna->pendulums = _parts(*visual, appearance->get_pendulums());
            maszyna->pendulum_amplitude = appearance->get_pendulum_amplitude();
        }
        maszyna->door_positions[0] = maszyna->door_positions[1] = -1.0;
        maszyna->door_step_positions[0] = maszyna->door_step_positions[1] = -1.0;
        for (const char *coupler: COUPLER_SUBMODELS) {
            for (const char *suffix: COUPLER_SUFFIXES) {
                const String name = String(coupler) + suffix;
                if (models->instance_has_part(visual->model, name)) {
                    maszyna->coupler_submodels.insert(name);
                }
            }
        }
        // only the lamps the model has
        for (const LightStateBinding &binding: LIGHT_STATE_BINDINGS) {
            if (models->instance_has_light(visual->model, binding.light_name)) {
                maszyna->lights[binding.light_name] = false;
            }
        }
        maszyna->headlights_dimmed = false;
        models->instance_set_lights_state(visual->model, maszyna->lights);
        _update_low_poly_cabs(p_vehicle, *visual);
        // the interior is lit only by the lights of its cabs (cabin_set_light_level())
        if (visual->low_poly.is_valid()) {
            models->instance_set_emission_energy(visual->low_poly, 0.0);
            if (!fading.has(p_vehicle)) {
                fading.push_back(p_vehicle);
            }
        }
        _update_couplers(p_vehicle, *visual, *maszyna);
        _update_lights(p_vehicle, *visual, *maszyna);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_moved(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        if (ModelRenderingServer *models = ModelRenderingServer::get_instance();
            models != nullptr && visual->own_models) {
            for (int end = 0; end < 2; ++end) {
                if (maszyna->coupler_adapters[end].is_valid()) {
                    models->instance_set_transform(
                            maszyna->coupler_adapters[end], _coupler_adapter_transform(p_vehicle, *visual, end));
                }
            }
        }
        _update_load(p_vehicle, *visual, *maszyna);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_scenario_changed(const RID &p_vehicle, const RID &p_scenario) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const Visual *visual = _get_visual(p_vehicle);
        const MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(models);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        if (visual->own_models) {
            for (const RID &instance: maszyna->coupler_adapters) {
                if (instance.is_valid()) {
                    models->instance_set_scenario(instance, p_scenario);
                }
            }
        }
        if (maszyna->load.is_valid()) {
            models->instance_set_scenario(maszyna->load, p_scenario);
        }
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_update(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        _update_lights(p_vehicle, *visual, *maszyna);
        _update_smoke(p_vehicle, *visual);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_pose(const RID &p_vehicle) {
        Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(visual);
        ERR_FAIL_NULL(maszyna);
        _pose_pantographs(p_vehicle, *visual, *maszyna);
        _pose_wipers(p_vehicle, *visual, *maszyna);
        _pose_mirrors(p_vehicle, *visual, *maszyna);
        _pose_doors(p_vehicle, *visual, *maszyna);
        _pose_pendulums(p_vehicle, *visual, *maszyna);
        _update_lights(p_vehicle, *visual, *maszyna);
    }

    void MaszynaE3DRailVehicleRenderingServer::_vehicle_low_poly_cabins_changed(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        ERR_FAIL_NULL(visual);
        _update_low_poly_cabs(p_vehicle, *visual);
    }

    /* The low-poly interiors still following their roof light */
    void MaszynaE3DRailVehicleRenderingServer::_frame_processed(const double p_delta) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        for (int index = static_cast<int>(fading.size()) - 1; index >= 0 && models != nullptr; --index) {
            const Visual *visual = _get_visual(fading[index]);
            MaszynaVisual *maszyna = maszyna_vehicles.getptr(fading[index]);
            const Ref<MaszynaRailVehicleAppearance> appearance =
                    visual != nullptr ? Ref<MaszynaRailVehicleAppearance>(visual->appearance)
                                      : Ref<MaszynaRailVehicleAppearance>();
            if (maszyna == nullptr || appearance.is_null() || !visual->low_poly.is_valid()) {
                fading.remove_at(index);
                continue;
            }
            const double full_energy = appearance->get_low_poly_emission_energy();
            const double fade_time = appearance->get_low_poly_emission_fade_time();
            // jointcabs: one light for every cab (Train.cpp:5249-5256)
            const bool joint_cabs = appearance->get_joint_cabs();
            double joint_level = 0.0;
            for (const double level: maszyna->cab_light_levels) {
                joint_level = MAX(joint_level, level);
            }
            bool faded = true;
            for (int cab = 0; cab < static_cast<int>(LOW_POLY_CABS.size()); ++cab) {
                const double target = (joint_cabs ? joint_level : maszyna->cab_light_levels[cab]) * full_energy;
                double &energy = maszyna->cab_light_energies[cab];
                const double step = fade_time > 0.0 ? full_energy * p_delta / fade_time : Math::abs(target - energy);
                energy += CLAMP(target - energy, -step, step);
                models->instance_set_part_emission_energy(
                        visual->low_poly, LOW_POLY_CABS[cab], static_cast<float>(energy));
                faded = faded && Math::is_equal_approx(energy, target);
            }
            if (faded) {
                fading.remove_at(index);
            }
        }
    }

    /* The cargo is configuration of the vehicle, and so are the pantographs' type and factors */
    void MaszynaE3DRailVehicleRenderingServer::_on_vehicle_config_changed(const RID &p_vehicle) {
        const Visual *visual = _get_visual(p_vehicle);
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        if (visual != nullptr && maszyna != nullptr) {
            _update_load(p_vehicle, *visual, *maszyna);
            _publish_pantographs(p_vehicle, *visual);
        }
    }

    /* The cargo is built again from its file, as the vehicle's own models are */
    void MaszynaE3DRailVehicleRenderingServer::_on_data_reload_requested() {
        Vector<RID> reloaded;
        for (const KeyValue<RID, MaszynaVisual> &item: maszyna_vehicles) {
            if (item.value.load.is_valid()) {
                reloaded.push_back(item.key);
            }
        }
        for (const RID &vehicle: reloaded) {
            if (const MaszynaVisual *maszyna = maszyna_vehicles.getptr(vehicle); maszyna != nullptr) {
                const String data_path = maszyna->load_data_path;
                const String model_filename = maszyna->load_model_filename;
                vehicle_set_load_model(vehicle, data_path, model_filename);
            }
        }
    }

    /* Each end's adapter model file registered, as the appearance's are; the one fitted before is
     * let go */
    void
    MaszynaE3DRailVehicleRenderingServer::_register_coupler_adapters(const RID &p_vehicle, MaszynaVisual &p_maszyna) {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ERR_FAIL_NULL(models);
        ERR_FAIL_NULL(lazy_loader);
        for (int end = 0; end < 2; ++end) {
            const String model_name = server != nullptr
                                              ? server->vehicle_get_coupler_adapter_model(
                                                        p_vehicle, static_cast<RailVehicleController::CouplerEnd>(end))
                                              : String();
            if (model_name == p_maszyna.coupler_adapter_models[end]) {
                continue;
            }
            if (p_maszyna.coupler_adapter_resources[end].is_valid()) {
                lazy_loader->resource_free(p_maszyna.coupler_adapter_resources[end]);
                p_maszyna.coupler_adapter_resources[end] = RID();
            }
            p_maszyna.coupler_adapter_models[end] = model_name;
            // the original's model manager takes it under models/ (TModelsManager::GetModel())
            if (!model_name.is_empty()) {
                p_maszyna.coupler_adapter_resources[end] = lazy_loader->resource_register(
                        String(COUPLER_ADAPTER_MODELS).path_join(model_name),
                        callable_mp(models, &ModelRenderingServer::model_load)
                                .bind(MaszynaE3DRenderingServer::BACKEND_NAME,
                                      String(COUPLER_ADAPTER_MODELS).path_join(model_name)));
            }
        }
    }

    void MaszynaE3DRailVehicleRenderingServer::vehicle_set_load_model(
            const RID &p_vehicle, const String &p_data_path, const String &p_model_filename) {
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(maszyna);
        maszyna->load_data_path = p_data_path;
        maszyna->load_model_filename = p_model_filename;
        ResourceLazyLoader *lazy_loader = ResourceLazyLoader::get_instance();
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ERR_FAIL_NULL(lazy_loader);
        ERR_FAIL_NULL(models);
        if (maszyna->load_resource.is_valid()) {
            lazy_loader->resource_free(maszyna->load_resource);
            maszyna->load_resource = RID();
        }
        if (!p_model_filename.is_empty()) {
            maszyna->load_resource = lazy_loader->resource_register(
                    p_data_path.path_join(p_model_filename),
                    callable_mp(models, &ModelRenderingServer::model_load)
                            .bind(MaszynaE3DRenderingServer::BACKEND_NAME, p_data_path.path_join(p_model_filename)));
        }
        // the cargo is built from the preload, with the vehicle's models or at once when they are
        _vehicle_preload(p_vehicle);
    }

    /* The cargo is built while the vehicle's models are, and goes with them */
    void MaszynaE3DRailVehicleRenderingServer::_build_load(
            const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        ERR_FAIL_NULL(models);
        if (p_maszyna.load.is_valid()) {
            models->instance_free(p_maszyna.load);
            p_maszyna.load = RID();
        }
        const Ref<Resource> model =
                p_visual.model.is_valid() ? _get_model(p_visual, p_maszyna.load_resource) : Ref<Resource>();
        if (model.is_null()) {
            return;
        }
        // nobody looks into the cargo, so it needs no node tree
        p_maszyna.load = models->instance_create(
                MaszynaE3DRenderingServer::BACKEND_NAME, model, ModelRenderingServer::INSTANCER_OPTIMIZED,
                ModelRenderingServer::INSTANCE_KIND_DYNAMIC);
        models->instance_set_options(
                p_maszyna.load, p_maszyna.load_data_path, PackedStringArray(), Array(), false, {}, 0);
        models->instance_set_scenario(p_maszyna.load, p_visual.placed ? p_visual.scenario : RID());
        models->instance_build(p_maszyna.load);
        _update_load(p_vehicle, p_visual, p_maszyna);
    }

    /* Another cab hidden while the low-poly cabs are not all visible */
    void MaszynaE3DRailVehicleRenderingServer::_on_vehicle_driver_cabin_changed(
            const RID &p_vehicle, const RID & /* p_cabin */) {
        if (const Visual *visual = _get_visual(p_vehicle); visual != nullptr) {
            _update_low_poly_cabs(p_vehicle, *visual);
        }
    }

    void MaszynaE3DRailVehicleRenderingServer::cabin_set_light_level(const RID &p_cabin, const double p_level) {
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_NULL(rail_vehicles);
        const RID vehicle = vehicle_server->cabin_get_vehicle(p_cabin);
        // a vehicle not drawn (never attached) has no low-poly cab to light
        MaszynaVisual *maszyna = maszyna_vehicles.getptr(vehicle);
        if (maszyna == nullptr) {
            return;
        }
        maszyna->cab_light_levels[low_poly_cab(rail_vehicles->cabin_get_kind(p_cabin))] = p_level;
        if (!fading.has(vehicle)) {
            fading.push_back(vehicle);
        }
    }

    Transform3D MaszynaE3DRailVehicleRenderingServer::_coupler_adapter_transform(
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

    void MaszynaE3DRailVehicleRenderingServer::_update_coupler_adapters(
            const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        if (models == nullptr || !p_visual.own_models) {
            return;
        }
        for (int end = 0; end < 2; ++end) {
            if (p_maszyna.coupler_adapters[end].is_valid()) {
                models->instance_free(p_maszyna.coupler_adapters[end]);
                p_maszyna.coupler_adapters[end] = RID();
            }
            const Ref<Resource> model = _get_model(p_visual, p_maszyna.coupler_adapter_resources[end]);
            if (model.is_null()) {
                continue;
            }
            const RID instance = models->instance_create(
                    MaszynaE3DRenderingServer::BACKEND_NAME, model, ModelRenderingServer::INSTANCER_OPTIMIZED,
                    ModelRenderingServer::INSTANCE_KIND_DYNAMIC);
            models->instance_set_options(
                    instance, COUPLER_ADAPTER_MODELS, p_visual.appearance->get_skins(), Array(), true, {}, 0);
            models->instance_set_scenario(instance, p_visual.placed ? p_visual.scenario : RID());
            models->instance_set_transform(instance, _coupler_adapter_transform(p_vehicle, p_visual, end));
            models->instance_build(instance);
            p_maszyna.coupler_adapters[end] = instance;
        }
    }

    /* The pantograph as the model builds it, handed to RailVehicleServer, which raises it -
     * TAnimPant's lengths and angles (DynObj.cpp:5508-5549). Where it sits on the vehicle is read
     * the same way the original reads it off the submodel's matrix (TAnimPant::vPos): without it
     * both pantographs of a vehicle sampled the wire at the vehicle's origin. Read off the model
     * file, not off its drawing - a vehicle far from the camera has none - and only for a vehicle
     * with a power source, whose pantographs RailVehicleServer raises (vehicle_collect_current()):
     * an EMU's car without an engine carries its unit's (FizTrainPowerParser). */
    void
    MaszynaE3DRailVehicleRenderingServer::_publish_pantographs(const RID &p_vehicle, const Visual &p_visual) const {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
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
        Ref<MaszynaE3DModel> model;
        if (const RID *resource = p_visual.model_resources.getptr(appearance->get_model_filename());
            resource != nullptr) {
            model = _get_model(p_visual, *resource);
        }
        if (model.is_null() && p_visual.model.is_valid()) {
            model = MaszynaE3DRenderingServer::get_instance()->instance_get_model(p_visual.model);
        }
        if (model.is_null()) {
            return;
        }
        _publish_pantograph_geometry(p_vehicle, p_visual, model, RailVehicleEnginePowerSource::PANTOGRAPH_FIRST);
        _publish_pantograph_geometry(p_vehicle, p_visual, model, RailVehicleEnginePowerSource::PANTOGRAPH_SECOND);
    }

    /* The submodel of the model by its lowered name, and where it rests in the model's frame - the
     * chain of its parents' transforms; null when the model has none of that name */
    static Ref<MaszynaE3DSubModel> find_submodel(
            const TypedArray<MaszynaE3DSubModel> &p_submodels, const String &p_name, const Transform3D &p_parent,
            Transform3D &p_r_transform) {
        for (int index = 0; index < p_submodels.size(); ++index) {
            Ref<MaszynaE3DSubModel> submodel = p_submodels[index];
            if (submodel.is_null()) {
                continue;
            }
            const Transform3D transform = p_parent * submodel->get_transform();
            if (submodel->get_name().to_lower() == p_name) {
                p_r_transform = transform;
                return submodel;
            }
            if (Ref<MaszynaE3DSubModel> found =
                        find_submodel(submodel->get_submodels(), p_name, transform, p_r_transform);
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
    void MaszynaE3DRailVehicleRenderingServer::_publish_pantograph_geometry(
            const RID &p_vehicle, const Visual &p_visual, const Ref<MaszynaE3DModel> &p_model,
            const RailVehicleEnginePowerSource::PantographSelector p_pantograph) const {
        RailVehicleServer *server = RailVehicleServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        const Ref<MaszynaRailVehicleAppearance> appearance = p_visual.appearance;
        if (server == nullptr || vehicle_server == nullptr || appearance.is_null()) {
            return;
        }
        const PackedStringArray names = p_pantograph == RailVehicleEnginePowerSource::PANTOGRAPH_FIRST
                                                ? appearance->get_pantograph_front_arms()
                                                : appearance->get_pantograph_rear_arms();
        if (names.size() != PANTOGRAPH_ELEMENTS) {
            return;
        }
        // each arm where it rests in the vehicle's own frame, and the slider's own submodel
        const Transform3D model_transform = appearance->get_model_transform();
        Part arms[PANTOGRAPH_ELEMENTS];
        Ref<MaszynaE3DSubModel> slider_model;
        for (int element = 0; element < PANTOGRAPH_ELEMENTS; ++element) {
            Transform3D rest;
            const String name = names[element].to_lower();
            const Ref<MaszynaE3DSubModel> submodel =
                    name.is_empty() ? Ref<MaszynaE3DSubModel>()
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
        const PackedFloat64Array factors = appearance->get_pantograph_factors();
        if (factors.size() == PANTOGRAPH_FACTOR_COUNT) {
            const int index = p_pantograph == RailVehicleEnginePowerSource::PANTOGRAPH_FIRST ? 0 : 1;
            if (slider_height == 0.0) {
                // the MMD's place along is the model's own, the vehicle frame turns it (DynObj.cpp:5605-5614)
                position.z = model_transform.basis.xform(Vector3(0.0, 0.0, static_cast<real_t>(factors[index]))).z;
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

    /* The arms drawn as far as RailVehicleServer has raised them: lower arm, its pair, upper arm,
     * its pair, slider */
    void MaszynaE3DRailVehicleRenderingServer::_pose_pantographs(
            const RID &p_vehicle, Visual &p_visual, const MaszynaVisual &p_maszyna) {
        const RailVehicleServer *server = RailVehicleServer::get_instance();
        if (server == nullptr) {
            return;
        }
        for (const RailVehicleEnginePowerSource::PantographSelector pantograph:
             {RailVehicleEnginePowerSource::PANTOGRAPH_FIRST, RailVehicleEnginePowerSource::PANTOGRAPH_SECOND}) {
            const Vector<Part> &arms = p_maszyna.pantograph_arms[pantograph];
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
    void MaszynaE3DRailVehicleRenderingServer::_pose_wipers(
            const RID &p_vehicle, Visual &p_visual, MaszynaVisual &p_maszyna) {
        const Ref<RailVehicleWipers> wipers =
                component<RailVehicleWipers>(p_vehicle, VehicleComponentType::COMPONENT_WIPERS);
        if (p_maszyna.wiper_arms.is_empty() || wipers.is_null()) {
            return;
        }
        const PackedFloat64Array positions = wipers->get_sweep_positions();
        if (positions == p_maszyna.wiper_positions) {
            return;
        }
        p_maszyna.wiper_positions = positions;
        const double wiper_angle = Math::deg_to_rad(wipers->get_angle());
        for (int wiper = 0; wiper < positions.size() &&
                            (static_cast<int64_t>(wiper) + 1) * WIPER_ELEMENTS <= p_maszyna.wiper_arms.size();
             ++wiper) {
            // the state tells the way out (0..1) from the way back (1..2)
            double sweep = positions[wiper] > 1.0 ? positions[wiper] - 1.0 : positions[wiper];
            // smoothInterpolate() (utilities.h:324)
            sweep = sweep * sweep * (3.0 - (2.0 * sweep));
            const double angle = (wiper % 2 == 1 ? -wiper_angle : wiper_angle) * sweep;
            for (int element = 0; element < WIPER_ELEMENTS; ++element) {
                _pose(p_visual, p_maszyna.wiper_arms[(wiper * WIPER_ELEMENTS) + element],
                      Basis(Vector3(0.0, 1.0, 0.0), static_cast<real_t>(element == WIPER_BLADE ? -angle : angle)));
            }
        }
    }

    // TDynamicObject::UpdateMirror() (DynObj.cpp:748-766): the mirrors at the end of the occupied
    // cab turn out about their vertical axis by MirrorMaxShift, as far as their side is unfolded.
    // Odd mirrors are on the left, even on the right, and one at the front end of the model is
    // the original's offset().z > 0 (DynObj.cpp:5903) - in the model's own frame.
    void MaszynaE3DRailVehicleRenderingServer::_pose_mirrors(
            const RID &p_vehicle, Visual &p_visual, MaszynaVisual &p_maszyna) {
        const Ref<RailVehicleDoors> doors =
                component<RailVehicleDoors>(p_vehicle, VehicleComponentType::COMPONENT_DOORS);
        if (p_maszyna.mirrors.is_empty() || doors.is_null()) {
            return;
        }
        const double left = doors->get_mirror_left_position();
        const double right = doors->get_mirror_right_position();
        const RailVehicleCabinKind::Kind driver_cabin = driver_cabin_kind(p_vehicle);
        if (left == p_maszyna.mirror_left && right == p_maszyna.mirror_right &&
            driver_cabin == p_maszyna.mirror_cabin) {
            return;
        }
        p_maszyna.mirror_left = left;
        p_maszyna.mirror_right = right;
        p_maszyna.mirror_cabin = driver_cabin;
        const double max_shift = Math::deg_to_rad(doors->get_mirror_max_shift());
        const Transform3D model_frame = p_visual.model_transform.affine_inverse();
        for (int index = 0; index < p_maszyna.mirrors.size(); ++index) {
            const Part &mirror = p_maszyna.mirrors[index];
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
    void MaszynaE3DRailVehicleRenderingServer::_pose_doors(
            const RID &p_vehicle, Visual &p_visual, MaszynaVisual &p_maszyna) {
        const Ref<RailVehicleDoors> doors =
                component<RailVehicleDoors>(p_vehicle, VehicleComponentType::COMPONENT_DOORS);
        if ((p_maszyna.doors.is_empty() && p_maszyna.door_steps.is_empty()) || doors.is_null()) {
            return;
        }
        const double positions[2] = {doors->get_left_position(), doors->get_right_position()};
        const double step_positions[2] = {doors->get_left_step_position(), doors->get_right_step_position()};
        if (positions[0] == p_maszyna.door_positions[0] && positions[1] == p_maszyna.door_positions[1] &&
            step_positions[0] == p_maszyna.door_step_positions[0] &&
            step_positions[1] == p_maszyna.door_step_positions[1]) {
            return;
        }
        for (int side = 0; side < 2; ++side) {
            p_maszyna.door_positions[side] = positions[side];
            p_maszyna.door_step_positions[side] = step_positions[side];
        }
        const double range_out = doors->get_max_shift_plug();
        for (int64_t first = 0; first < p_maszyna.doors.size(); first += DOOR_ELEMENTS) {
            // door n+1 odd: the left side, positions[0]
            const auto position = static_cast<real_t>(positions[(first / DOOR_ELEMENTS) % 2]);
            const Part &leaf = p_maszyna.doors[first];
            switch (doors->get_type()) {
                case RailVehicleDoors::TYPE_SHIFT:
                    _pose(p_visual, leaf, Transform3D(Basis(), Vector3(0.0, 0.0, position)));
                    break;
                case RailVehicleDoors::TYPE_ROTATE:
                    _pose(p_visual, leaf, Basis(Vector3(1.0, 0.0, 0.0), Math::deg_to_rad(position)));
                    break;
                case RailVehicleDoors::TYPE_FOLD:
                    _pose(p_visual, leaf, Basis(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(position)));
                    _pose(p_visual, p_maszyna.doors[first + 1],
                          Basis(Vector3(0.0, 0.0, 1.0), Math::deg_to_rad(-2 * position)));
                    _pose(p_visual, p_maszyna.doors[first + 2],
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
        for (int step = 0; step < p_maszyna.door_steps.size(); ++step) {
            const auto shift = static_cast<real_t>(step_range * step_positions[step % 2]);
            const Part &platform = p_maszyna.door_steps[step];
            if (doors->get_platform_type() == RailVehicleDoors::PLATFORM_TYPE_ROTATE) {
                _pose(p_visual, platform, Basis(Vector3(0.0, 1.0, 0.0), Math::deg_to_rad(shift)));
            } else {
                _pose(p_visual, platform, Transform3D(Basis(), Vector3(shift, 0.0, 0.0)));
            }
        }
    }

    // The pendulums swing about their x by the amplitude [deg] times the cosine of the engine's turn
    // (DynObj.cpp:1121-1125)
    void MaszynaE3DRailVehicleRenderingServer::_pose_pendulums(
            const RID &p_vehicle, Visual &p_visual, const MaszynaVisual &p_maszyna) {
        const Ref<RailVehicleEngine> engine =
                component<RailVehicleEngine>(p_vehicle, VehicleComponentType::COMPONENT_ENGINE);
        if (p_maszyna.pendulums.is_empty() || engine.is_null()) {
            return;
        }
        const Basis swing(
                Vector3(1.0, 0.0, 0.0),
                static_cast<real_t>(Math::deg_to_rad(p_maszyna.pendulum_amplitude * Math::cos(engine->get_angle()))));
        for (const Part &pendulum: p_maszyna.pendulums) {
            _pose(p_visual, pendulum, swing);
        }
    }

    // Original engine: TDynamicObject::GetPneumatic() (DynObj.cpp:395) - which hoses the model has
    // at that end: 1 left, 2 right (the "r" variant), 3 both; AirCoupler::GetStatus()
    // (AirCoupler.cpp:30) tells a slanted (_xon) from a straight (_on) connected submodel
    MaszynaE3DRailVehicleRenderingServer::PneumaticLayout MaszynaE3DRailVehicleRenderingServer::_pneumatic_layout(
            const MaszynaVisual &p_maszyna, const RailVehicleController::CouplerEnd p_end,
            const MaszynaE3DRailVehicleRenderingServer::PneumaticLine p_line) const {
        const auto has_connected = [&](const String &p_name) {
            return p_maszyna.coupler_submodels.has(p_name + String("_xon")) ||
                   p_maszyna.coupler_submodels.has(p_name + String("_on"));
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
    MaszynaE3DRailVehicleRenderingServer::CouplerVariant MaszynaE3DRailVehicleRenderingServer::_pneumatic_variant(
            const RID &p_vehicle, const MaszynaVisual &p_maszyna, const RailVehicleController::CouplerEnd p_end,
            const MaszynaE3DRailVehicleRenderingServer::PneumaticLine p_line) const {
        const Ref<RailVehicleBuffCoupl> coupler = couplers(p_vehicle);
        RailVehicleServer *server = RailVehicleServer::get_instance();
        if (coupler.is_null() || server == nullptr) {
            return COUPLER_VARIANT_OFF;
        }
        const PneumaticLayout own = _pneumatic_layout(p_maszyna, p_end, p_line);
        PneumaticLayout other = PNEUMATIC_LAYOUT_NONE;
        // the vehicles coupled beyond p_end, from the farthest one back through this one: the
        // neighbour is the one just before it
        const TypedArray<RID> coupled =
                server->vehicle_get_coupled(p_vehicle, p_end, RailVehicleController::COUPLING_FLAG_COUPLER);
        if (const int64_t own_index = coupled.find(p_vehicle); own_index > 0) {
            if (const MaszynaVisual *neighbour = maszyna_vehicles.getptr(coupled[own_index - 1]);
                neighbour != nullptr) {
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
    void MaszynaE3DRailVehicleRenderingServer::_show_air_coupler(
            const Visual &p_visual, const MaszynaVisual &p_maszyna, const String &p_name, const bool p_on,
            const bool p_xon) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const bool states[] = {p_on, !(p_on || p_xon), p_xon};
        for (int index = 0; index < static_cast<int>(COUPLER_SUFFIXES.size()); ++index) {
            const String name = p_name + String(COUPLER_SUFFIXES[index]);
            if (p_maszyna.coupler_submodels.has(name)) {
                models->instance_set_part_visible(p_visual.model, name, states[index]);
            }
        }
    }

    // Original engine: coupler and hose submodel visibility (DynObj.cpp:758-925, bnewAirCouplers branch)
    void MaszynaE3DRailVehicleRenderingServer::_update_couplers(
            const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna) {
        const Ref<RailVehicleBuffCoupl> coupler = couplers(p_vehicle);
        if (p_maszyna.coupler_submodels.is_empty() || coupler.is_null()) {
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
                                       ? _pneumatic_variant(p_vehicle, p_maszyna, end, PNEUMATIC_LINE_BRAKE)
                                       : COUPLER_VARIANT_OFF;
            variants[end][2] = coupler->is_main_hose_connected(end)
                                       ? _pneumatic_variant(p_vehicle, p_maszyna, end, PNEUMATIC_LINE_MAIN)
                                       : COUPLER_VARIANT_OFF;
            for (const CouplerVariant variant: variants[end]) {
                state = (state * COUPLER_VARIANT_COUNT) + variant;
            }
        }
        if (state == p_maszyna.coupler_state) {
            return;
        }
        p_maszyna.coupler_state = state;
        for (const RailVehicleController::CouplerEnd end:
             {RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLER_END_REAR}) {
            // the original numbers the couplers from 1 (coupler1, coupler2)
            const String number = itos(end + 1);
            _show_air_coupler(
                    p_visual, p_maszyna, "coupler" + number, variants[end][0] == COUPLER_VARIANT_ON,
                    variants[end][0] == COUPLER_VARIANT_XON &&
                            p_maszyna.coupler_submodels.has("coupler" + number + "_xon"));
            for (const PneumaticLine line: {PNEUMATIC_LINE_BRAKE, PNEUMATIC_LINE_MAIN}) {
                const String name = String(PNEUMATIC_SUBMODELS[line]) + number;
                const CouplerVariant variant = variants[end][line + 1];
                _show_air_coupler(
                        p_visual, p_maszyna, name, variant == COUPLER_VARIANT_ON, variant == COUPLER_VARIANT_XON);
                _show_air_coupler(
                        p_visual, p_maszyna, name + String("r"), variant == COUPLER_VARIANT_RIGHT_ON,
                        variant == COUPLER_VARIANT_RIGHT_XON);
            }
        }
    }

    void MaszynaE3DRailVehicleRenderingServer::_update_lights(
            const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const Ref<RailVehicleLighting> lighting =
                component<RailVehicleLighting>(p_vehicle, VehicleComponentType::COMPONENT_LIGHTING);
        if (models == nullptr || lighting.is_null() || !p_visual.model.is_valid()) {
            return;
        }
        bool changed = false;
        for (const LightStateBinding &binding: LIGHT_STATE_BINDINGS) {
            if (!p_maszyna.lights.has(binding.light_name)) {
                continue;
            }
            const bool enabled = (lighting.ptr()->*binding.is_enabled)();
            if (static_cast<bool>(p_maszyna.lights[binding.light_name]) != enabled) {
                p_maszyna.lights[binding.light_name] = enabled;
                changed = true;
            }
        }
        if (changed) {
            models->instance_set_lights_state(p_visual.model, p_maszyna.lights);
        }
        // Headlights dimmed (DynamicObject->DimHeadlights)
        if (const bool dimmed = lighting->get_headlights_dimmed(); dimmed != p_maszyna.headlights_dimmed) {
            p_maszyna.headlights_dimmed = dimmed;
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
    void MaszynaE3DRailVehicleRenderingServer::_update_smoke(const RID &p_vehicle, const Visual &p_visual) const {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
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
            MaszynaE3DRenderingServer::get_instance()->instance_set_smoke_intensity(p_visual.model, 1.0);
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
            const double revolutions_deficit = (max_rpm - revolutions) / Units::SECONDS_PER_MINUTE;
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
        MaszynaE3DRenderingServer::get_instance()->instance_set_smoke_intensity(
                p_visual.model, static_cast<float>(CLAMP(intensity, 0.0, 1.0) * fill));
    }

    // Original engine: every low-poly cab is drawn, but in the view from inside the player's
    // vehicle, where the "cabN" of the occupied cab is hidden - or all of them with jointcabs: -
    // so the hi-fi cab doesn't overlap it; a cab without a hi-fi model hides none
    // (DynObj.cpp:1389-1397). Which view that is belongs to whoever shows the cab's interior
    // (vehicle_set_visible_low_poly_cabins()).
    void
    MaszynaE3DRailVehicleRenderingServer::_update_low_poly_cabs(const RID &p_vehicle, const Visual &p_visual) const {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        if (models == nullptr || !p_visual.low_poly.is_valid() || p_visual.appearance.is_null()) {
            return;
        }
        const Ref<MaszynaRailVehicleAppearance> appearance = p_visual.appearance;
        const int hidden = low_poly_cab(driver_cabin_kind(p_vehicle));
        const bool joint_cabs = appearance.is_valid() && appearance->get_joint_cabs();
        for (int cab = 0; cab < static_cast<int>(LOW_POLY_CABS.size()); ++cab) {
            const String name = LOW_POLY_CABS[cab];
            if (models->instance_has_part(p_visual.low_poly, name)) {
                models->instance_set_part_visible(
                        p_visual.low_poly, name, p_visual.low_poly_cabs_visible || (!joint_cabs && cab != hidden));
            }
        }
    }

    /* Where the cargo sits: the original sinks it into the body as the vehicle empties, lerping
     * from the cargo's own offset_min to zero with how full it is (DynObj.cpp:3070-3080), and
     * leaves it alone when that cargo declares no offset. Both numbers are the vehicle's
     * configuration, so they are taken again when it changes. */
    void MaszynaE3DRailVehicleRenderingServer::_update_load(
            const RID &p_vehicle, const Visual &p_visual, MaszynaVisual &p_maszyna) {
        ModelRenderingServer *models = ModelRenderingServer::get_instance();
        const VehicleServer *vehicle_server = VehicleServer::get_instance();
        if (models == nullptr || vehicle_server == nullptr || !p_maszyna.load.is_valid()) {
            return;
        }
        const Ref<RailVehicleController> vehicle = vehicle_server->vehicle_get_controller(p_vehicle);
        const Ref<RailVehicleLoad> load = component<RailVehicleLoad>(p_vehicle, VehicleComponentType::COMPONENT_LOAD);
        p_maszyna.load_height = 0.0;
        if (vehicle.is_valid() && load.is_valid()) {
            const TypedArray<String> accepted = load->get_accepted_loads();
            const TypedArray<float> offsets = load->get_minimum_load_offsets();
            const String cargo = vehicle->get_load_name().to_lower();
            for (int index = 0; index < accepted.size() && index < offsets.size(); ++index) {
                if (String(accepted[index]).to_lower() == cargo) {
                    const double max_load = load->get_max_load();
                    const double fill = max_load > 0.0 ? CLAMP(vehicle->get_load_amount() / max_load, 0.0, 1.0) : 0.0;
                    p_maszyna.load_height = Math::lerp(static_cast<double>(offsets[index]), 0.0, fill);
                    break;
                }
            }
        }
        models->instance_set_transform(
                p_maszyna.load,
                p_visual.transform * p_visual.model_transform *
                        Transform3D(Basis(), Vector3(0.0, static_cast<real_t>(p_maszyna.load_height), 0.0)));
    }

    /* A coupling changed at either end: the couplers are drawn again, and the neighbours', whose
     * hoses match this vehicle's (SetPneumatic(), DynObj.cpp:430) */
    void MaszynaE3DRailVehicleRenderingServer::_on_vehicle_trainset_changed(const RID &p_vehicle) {
        if (MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle); maszyna != nullptr) {
            _update_couplers(p_vehicle, *_get_visual(p_vehicle), *maszyna);
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
            if (MaszynaVisual *maszyna = maszyna_vehicles.getptr(neighbour); maszyna != nullptr) {
                _update_couplers(neighbour, *_get_visual(neighbour), *maszyna);
            }
        }
    }

    void MaszynaE3DRailVehicleRenderingServer::_on_vehicle_coupler_changed(const RID &p_vehicle, const int64_t p_flag) {
        _on_vehicle_trainset_changed(p_vehicle);
    }

    /* The new adapter's model is loaded on a worker, and fitted once it is (_apply_preloaded()) */
    void MaszynaE3DRailVehicleRenderingServer::_on_vehicle_coupler_adapter_changed(
            const RID &p_vehicle, const int64_t p_end) {
        if (MaszynaVisual *maszyna = maszyna_vehicles.getptr(p_vehicle); maszyna != nullptr) {
            _register_coupler_adapters(p_vehicle, *maszyna);
            _vehicle_preload(p_vehicle);
        }
    }
} // namespace godot
