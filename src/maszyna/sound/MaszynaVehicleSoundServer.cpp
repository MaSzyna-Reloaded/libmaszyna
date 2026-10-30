#include "MaszynaVehicleSoundServer.hpp"
#include "cabin/Cabin3D.hpp"
#include "maszyna/scenery/MaszynaTrackData.hpp"
#include "maszyna/vehicles/MaszynaMoverRailVehicleBrake.hpp"
#include "player/TrainSoundListener3D.hpp"
#include "roads/RoadServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "tracks/TrackServer.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleComponentType.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleComponentType.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleDoors.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"
#include "vehicles/rail/RailVehicleElectricSeriesEngine.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"
#include "vehicles/rail/RailVehicleHorns.hpp"
#include "vehicles/rail/RailVehicleMasterController.hpp"
#include "vehicles/rail/RailVehiclePowerSupply.hpp"
#include "vehicles/rail/RailVehicleRenderingServer.hpp"
#include "vehicles/rail/RailVehicleSecuritySystem.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include "vehicles/rail/RailVehicleSpringBrake.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"
#include "vehicles/road/RoadVehicleController.hpp"
#include "vehicles/road/RoadVehicleServer.hpp"

#include <algorithm>
#include <godot_cpp/classes/audio_effect_low_pass_filter.hpp>
#include <godot_cpp/classes/audio_server.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/property_tweener.hpp>
#include <godot_cpp/classes/scene_tree.hpp>
#include <godot_cpp/classes/time.hpp>
#include <godot_cpp/classes/window.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /* The original's soundproofing when the MMD gives none, squared: a row per placement, a column
         * per listener context (sound.cpp:981-1008, EU07_SOUND_PROOFINGPLACEMENT defaults) */
        constexpr double DEFAULT_PROOFING[5][6] = {
                {1.0, 0.2, 1.0, 0.65, 0.2, 0.2},   {0.2, 1.0, 0.2, 0.65, 0.65, 0.65}, {0.2, 0.2, 0.2, 0.65, 0.65, 1.0},
                {0.01, 0.01, 0.01, 0.2, 0.2, 1.0}, {1.0, 1.0, 1.0, 1.0, 1.0, 1.0},
        };
        constexpr const char *QUANTITY_NAMES[MaszynaVehicleSoundServer::QUANTITY_MAX] = {
                "speed",
                "velocity",
                "brake_cylinder_pressure",
                "brake_local_pressure",
                "pipe_pressure",
                "brake_unit_force",
                "brake_force_ratio",
                "brake_emergency_valve_flow",
                "brake_main_valve_flow",
                "brake_releaser_active",
                "brake_handle_braking_flow",
                "brake_handle_release_flow",
                "brake_handle_emergency_flow",
                "brake_handle_control_chamber_flow",
                "brake_handle_timing_reservoir_flow",
                "brake_accelerator_count",
                "ep_brake_increase_count",
                "ep_brake_increase_pitch",
                "ep_brake_decrease_count",
                "ep_brake_decrease_pitch",
                "spring_brake_active",
                "wheels_slipping",
                "wheels_rotation",
                "engine_power",
                "engine_revolutions",
                "engine_rpm",
                "engine_rpm_ratio",
                "engine_fill",
                "engine_dynamic_brake_active",
                "engine_resistor_fan_rotation",
                "motor_momentum",
                "master_controller_position",
                "car_engine_running",
                "car_engine_revolutions",
                "car_throttle",
                "track_quality",
                "track_environment",
                "track_switch",
                "curve_radius",
                "curve_cant",
                "outer_noise_audible",
                "oil_pump_active",
                "fuel_pump_active",
                "battery_enabled",
                "compressor_enabled",
                "converter_enabled",
                "current_collector_pantograph_compressor_enabled",
                "horn_low_active",
                "horn_high_active",
                "whistle_active",
                "beeping",
                "cabsignal_beeping",
                "doors_departure_signal_sounding",
                "tachometer_clock_speed",
                "coupler_attach_coupler",
                "coupler_attach_brakehose",
                "coupler_attach_mainhose",
                "coupler_attach_control",
                "coupler_attach_gangway",
                "coupler_attach_heating",
                "coupler_detach_coupler",
                "coupler_detach_brakehose",
                "coupler_detach_mainhose",
                "coupler_detach_control",
                "coupler_detach_gangway",
                "coupler_detach_heating",
                "coupler_adapter_attach",
                "coupler_adapter_remove",
                "pantograph_up",
                "pantograph_down",
                "derailed",
        };
        /* The parameter of an axle of the wheel clatter is this followed by its index */
        constexpr const char *AXLE_CLICKS_PREFIX = "axle_clicks_";
        /* The track quantities are read only of a moving vehicle, the curve only above 5 km/h
         * (DynObj.cpp:4735) */
        constexpr double CURVE_MIN_SPEED = 5.0;
        constexpr int VEHICLE_EVENT_COUNT =
                MaszynaVehicleSoundServer::QUANTITY_MAX - MaszynaVehicleSoundServer::QUANTITY_COUPLER_ATTACH_COUPLER;
        constexpr int COUPLER_DETACH_OFFSET = MaszynaVehicleSoundServer::QUANTITY_COUPLER_DETACH_COUPLER -
                                              MaszynaVehicleSoundServer::QUANTITY_COUPLER_ATTACH_COUPLER;
    } // namespace

    /* The vehicles whose sound is built first: the nearest */
    struct NearerFirst {
            bool operator()(const Pair<double, RID> &p_first, const Pair<double, RID> &p_second) const {
                return p_first.first < p_second.first;
            }
    };

    /* A coupling's place among the attach (and, offset, the detach) counts */
    static int coupling_event(const int64_t p_flag) {
        switch (p_flag) {
            case RailVehicleController::COUPLING_FLAG_COUPLER:
                return 0;
            case RailVehicleController::COUPLING_FLAG_BRAKEHOSE:
                return 1;
            case RailVehicleController::COUPLING_FLAG_MAINHOSE:
                return 2;
            case RailVehicleController::COUPLING_FLAG_CONTROL:
                return 3;
            case RailVehicleController::COUPLING_FLAG_GANGWAY:
                return 4;
            case RailVehicleController::COUPLING_FLAG_HEATING:
                return 5;
            default:
                return -1;
        }
    }

    void MaszynaVehicleSoundServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("quantity_get_name", "quantity"), &MaszynaVehicleSoundServer::quantity_get_name);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_bank_builder", "vehicle", "builder"),
                &MaszynaVehicleSoundServer::vehicle_set_bank_builder);
        ClassDB::bind_method(
                D_METHOD("bank_register", "player", "vehicle", "registration"),
                &MaszynaVehicleSoundServer::bank_register);

        BIND_ENUM_CONSTANT(PLACEMENT_INTERNAL);
        BIND_ENUM_CONSTANT(PLACEMENT_ENGINE);
        BIND_ENUM_CONSTANT(PLACEMENT_EXTERNAL);
        BIND_ENUM_CONSTANT(PLACEMENT_GENERAL);
        BIND_ENUM_CONSTANT(PLACEMENT_CUSTOM);

        BIND_ENUM_CONSTANT(QUANTITY_SPEED);
        BIND_ENUM_CONSTANT(QUANTITY_VELOCITY);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_CYLINDER_PRESSURE);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_LOCAL_PRESSURE);
        BIND_ENUM_CONSTANT(QUANTITY_PIPE_PRESSURE);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_UNIT_FORCE);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_FORCE_RATIO);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_MAIN_VALVE_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_RELEASER_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_HANDLE_BRAKING_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_HANDLE_RELEASE_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_HANDLE_EMERGENCY_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_HANDLE_CONTROL_CHAMBER_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_HANDLE_TIMING_RESERVOIR_FLOW);
        BIND_ENUM_CONSTANT(QUANTITY_BRAKE_ACCELERATOR_COUNT);
        BIND_ENUM_CONSTANT(QUANTITY_EP_BRAKE_INCREASE_COUNT);
        BIND_ENUM_CONSTANT(QUANTITY_EP_BRAKE_INCREASE_PITCH);
        BIND_ENUM_CONSTANT(QUANTITY_EP_BRAKE_DECREASE_COUNT);
        BIND_ENUM_CONSTANT(QUANTITY_EP_BRAKE_DECREASE_PITCH);
        BIND_ENUM_CONSTANT(QUANTITY_SPRING_BRAKE_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_WHEELS_SLIPPING);
        BIND_ENUM_CONSTANT(QUANTITY_WHEELS_ROTATION);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_POWER);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_REVOLUTIONS);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_RPM);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_RPM_RATIO);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_FILL);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_DYNAMIC_BRAKE_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_ENGINE_RESISTOR_FAN_ROTATION);
        BIND_ENUM_CONSTANT(QUANTITY_MOTOR_MOMENTUM);
        BIND_ENUM_CONSTANT(QUANTITY_MASTER_CONTROLLER_POSITION);
        BIND_ENUM_CONSTANT(QUANTITY_CAR_ENGINE_RUNNING);
        BIND_ENUM_CONSTANT(QUANTITY_CAR_ENGINE_REVOLUTIONS);
        BIND_ENUM_CONSTANT(QUANTITY_CAR_THROTTLE);
        BIND_ENUM_CONSTANT(QUANTITY_TRACK_QUALITY);
        BIND_ENUM_CONSTANT(QUANTITY_TRACK_ENVIRONMENT);
        BIND_ENUM_CONSTANT(QUANTITY_TRACK_SWITCH);
        BIND_ENUM_CONSTANT(QUANTITY_CURVE_RADIUS);
        BIND_ENUM_CONSTANT(QUANTITY_CURVE_CANT);
        BIND_ENUM_CONSTANT(QUANTITY_OUTER_NOISE_AUDIBLE);
        BIND_ENUM_CONSTANT(QUANTITY_OIL_PUMP_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_FUEL_PUMP_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_BATTERY_ENABLED);
        BIND_ENUM_CONSTANT(QUANTITY_COMPRESSOR_ENABLED);
        BIND_ENUM_CONSTANT(QUANTITY_CONVERTER_ENABLED);
        BIND_ENUM_CONSTANT(QUANTITY_PANTOGRAPH_COMPRESSOR_ENABLED);
        BIND_ENUM_CONSTANT(QUANTITY_HORN_LOW_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_HORN_HIGH_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_WHISTLE_ACTIVE);
        BIND_ENUM_CONSTANT(QUANTITY_BEEPING);
        BIND_ENUM_CONSTANT(QUANTITY_CABSIGNAL_BEEPING);
        BIND_ENUM_CONSTANT(QUANTITY_DEPARTURE_SIGNAL_SOUNDING);
        BIND_ENUM_CONSTANT(QUANTITY_TACHOMETER_CLOCK_SPEED);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ATTACH_COUPLER);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ATTACH_BRAKEHOSE);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ATTACH_MAINHOSE);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ATTACH_CONTROL);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ATTACH_GANGWAY);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ATTACH_HEATING);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_DETACH_COUPLER);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_DETACH_BRAKEHOSE);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_DETACH_MAINHOSE);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_DETACH_CONTROL);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_DETACH_GANGWAY);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_DETACH_HEATING);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ADAPTER_ATTACH);
        BIND_ENUM_CONSTANT(QUANTITY_COUPLER_ADAPTER_REMOVE);
        BIND_ENUM_CONSTANT(QUANTITY_PANTOGRAPH_UP);
        BIND_ENUM_CONSTANT(QUANTITY_PANTOGRAPH_DOWN);
        BIND_ENUM_CONSTANT(QUANTITY_DERAILED);
        BIND_ENUM_CONSTANT(QUANTITY_MAX);
    }

    MaszynaVehicleSoundServer::MaszynaVehicleSoundServer() {
        singleton = this;
        for (int quantity = 0; quantity < QUANTITY_MAX; quantity++) {
            quantity_names[quantity] = QUANTITY_NAMES[quantity];
        }
        set_parameter_values_method = "set_parameter_values";
        set_cut_method = "set_cut";
    }

    MaszynaVehicleSoundServer::~MaszynaVehicleSoundServer() {
        _set_advancing(false);
        singleton = nullptr;
    }

    StringName MaszynaVehicleSoundServer::quantity_get_name(const Quantity p_quantity) const {
        ERR_FAIL_INDEX_V(p_quantity, QUANTITY_MAX, StringName());
        return quantity_names[p_quantity];
    }

    /* The servers this one follows, once there is a tree - this server is made before the main
     * loop exists, and the game's first vehicle or bank is the first moment one is needed */
    void MaszynaVehicleSoundServer::_wire() {
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (wired || tree == nullptr) {
            return;
        }
        wired = true;
        tree->connect("node_added", callable_mp(this, &MaszynaVehicleSoundServer::_on_node_added));
        tree->connect("node_removed", callable_mp(this, &MaszynaVehicleSoundServer::_on_node_removed));
        // the player's listener may be in the tree before the first vehicle is
        TypedArray<Node> listeners = tree->get_root()->find_children("*", "TrainSoundListener3D", true, false);
        if (!listeners.is_empty()) {
            _on_node_added(Object::cast_to<Node>(listeners[0]));
        }
        SimulationServer *simulation = SimulationServer::get_instance();
        ERR_FAIL_NULL(simulation);
        simulation->connect(
                SimulationServer::simulation_paused_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_runtime_paused_changed));
        simulation->connect(
                SimulationServer::simulation_unpaused_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_runtime_paused_changed));
        simulation->connect(
                SimulationServer::simulation_speed_changed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_runtime_paused_changed));
        simulation->connect(
                SimulationServer::simulation_current_speed_changed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_simulation_current_speed_changed));
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        vehicle_server->connect(
                VehicleServer::vehicle_controller_changed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_vehicle_controller_changed));
        vehicle_server->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &MaszynaVehicleSoundServer::_on_vehicle_freed));
        RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(rail);
        rail->connect(
                RailVehicleServer::vehicle_coupler_attached_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_coupler_attached));
        rail->connect(
                RailVehicleServer::vehicle_coupler_detached_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_coupler_detached));
        rail->connect(
                RailVehicleServer::vehicle_coupler_adapter_attached_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_coupler_adapter_attached));
        rail->connect(
                RailVehicleServer::vehicle_coupler_adapter_removed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_coupler_adapter_removed));
        rail->connect(
                RailVehicleServer::vehicle_derailed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_derailed));
        _on_simulation_current_speed_changed();
        _on_runtime_paused_changed();
    }

    /* The step runs while there is a vehicle with sound, built or to build */
    void MaszynaVehicleSoundServer::_set_advancing(const bool p_advancing) {
        SimulationServer *simulation = SimulationServer::get_instance();
        if (advancing == p_advancing || simulation == nullptr) {
            return;
        }
        advancing = p_advancing;
        const Callable advanced = callable_mp(this, &MaszynaVehicleSoundServer::_on_simulation_advanced);
        if (advancing) {
            simulation->connect(SimulationServer::simulation_advanced_signal, advanced);
            return;
        }
        simulation->disconnect(SimulationServer::simulation_advanced_signal, advanced);
    }

    void MaszynaVehicleSoundServer::vehicle_set_bank_builder(const RID &p_vehicle, const Callable &p_builder) {
        _wire();
        if (p_builder.is_valid()) {
            bank_builders[p_vehicle] = p_builder;
            _set_advancing(true);
            return;
        }
        bank_builders.erase(p_vehicle);
        const int64_t queued = bank_build_queue.find(p_vehicle);
        if (queued >= 0) {
            bank_build_queue.remove_at(static_cast<uint32_t>(queued));
        }
    }

    void MaszynaVehicleSoundServer::bank_register(
            const uint64_t p_player, const RID &p_vehicle, const Dictionary &p_registration) {
        _wire();
        Object *player = ObjectDB::get_instance(ObjectID(p_player));
        ERR_FAIL_NULL(player);
        const ObjectID player_id(p_player);
        ERR_FAIL_COND_MSG(banks.has(player_id), "The sound player is registered already");
        Bank &bank = banks.insert(player_id, Bank())->value;
        bank.player = player_id;
        bank.vehicle = p_vehicle;
        bank.cabin_only = p_registration.get("cabin_only", false);
        bank.brake_valve_events = p_registration.get("brake_valve_events", PackedStringArray());
        bank.local_brake_events = p_registration.get("local_brake_events", PackedStringArray());
        bank.tachometer_events = p_registration.get("tachometer_events", PackedStringArray());
        // built unheard: the first sweep that hears it lets it in, and what it finds on then resumes
        // past its opening - as the original's sounds activated out of range (sound.cpp:364-371)
        _set_bank_cut(bank, true);

        HashMap<StringName, ProofingFeed> proofing;
        const Array proofing_entries = p_registration.get("proofing", Array());
        for (const Variant &entry_variant: proofing_entries) {
            const Dictionary entry = entry_variant;
            ProofingFeed feed;
            feed.parameter = entry["parameter"];
            feed.placement = static_cast<Placement>(static_cast<int>(entry["placement"]));
            feed.profile = entry.get("profile", PackedFloat32Array());
            proofing[feed.parameter] = feed;
        }
        const PackedFloat32Array axle_offsets = p_registration.get("axles", PackedFloat32Array());

        // the parameters the bank declares (SfxBank.parameters, SfxParameter.name - gnd-sfx's, see the class)
        Object *sfx_bank = Object::cast_to<Object>(player->get("bank"));
        ERR_FAIL_NULL_MSG(sfx_bank, "The sound player has no bank");
        const Array parameters = sfx_bank->get("parameters");
        bank.values.resize(parameters.size());
        for (int64_t index = 0; index < parameters.size(); index++) {
            const Object *parameter = Object::cast_to<Object>(parameters[index]);
            const StringName name = parameter->get("name");
            bank.values.set(index, parameter->get("default_value"));
            if (proofing.has(name)) {
                ProofingFeed feed = proofing[name];
                feed.index = index;
                bank.proofing.push_back(feed);
                continue;
            }
            const String text = name;
            if (text.begins_with(AXLE_CLICKS_PREFIX)) {
                const int64_t axle = text.trim_prefix(AXLE_CLICKS_PREFIX).to_int();
                ERR_CONTINUE_MSG(axle >= axle_offsets.size(), vformat("No offset of the axle of %s", text));
                AxleFeed feed;
                feed.parameter = name;
                feed.index = index;
                feed.offset = axle_offsets[axle];
                bank.axles.push_back(feed);
                continue;
            }
            int quantity = 0;
            while (quantity < QUANTITY_MAX && quantity_names[quantity] != name) {
                quantity++;
            }
            ERR_CONTINUE_MSG(quantity == QUANTITY_MAX, vformat("No quantity is named %s", text));
            QuantityFeed feed;
            feed.parameter = name;
            feed.index = index;
            feed.quantity = static_cast<Quantity>(quantity);
            bank.quantities.push_back(feed);
        }

        player->connect(
                "tree_exiting", callable_mp(this, &MaszynaVehicleSoundServer::_on_bank_exiting).bind(p_player),
                CONNECT_ONE_SHOT);
        if (!vehicles.has(p_vehicle)) {
            Vehicle &entry = vehicles.insert(p_vehicle, Vehicle())->value;
            entry.events.resize(VEHICLE_EVENT_COUNT);
            for (int64_t &count: entry.events) {
                count = 0;
            }
        }
        Vehicle &entry = vehicles[p_vehicle];
        entry.banks.push_back(player_id);
        const Array profile = p_registration.get("soundproofing", Array());
        if (!profile.is_empty()) {
            entry.soundproofing.clear();
            for (const Variant &row: profile) {
                entry.soundproofing.push_back(row);
            }
        }
        if (entry.controller.is_null()) {
            _resolve_vehicle(p_vehicle, entry);
        }
        _refresh_bank_context(bank);
        _set_advancing(true);
        // heard before the next sweep looks at it
        sweep_elapsed = SWEEP_INTERVAL;
    }

    void MaszynaVehicleSoundServer::_on_bank_exiting(const uint64_t p_player) {
        const ObjectID player_id(p_player);
        if (!banks.has(player_id)) {
            return;
        }
        const RID vehicle = banks[player_id].vehicle;
        banks.erase(player_id);
        if (!vehicles.has(vehicle)) {
            return;
        }
        Vehicle &entry = vehicles[vehicle];
        entry.banks.erase(player_id);
        if (entry.banks.is_empty()) {
            _release_vehicle(entry, vehicle);
        }
    }

    /* The components of the vehicle's controller - another controller brings other objects */
    void MaszynaVehicleSoundServer::_resolve_vehicle(const RID &p_vehicle, Vehicle &p_entry) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_NULL(rail);
        if (p_entry.power_source.is_valid()) {
            p_entry.power_source->disconnect(
                    RailVehicleEnginePowerSource::pantograph_up_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_pantograph_up));
            p_entry.power_source->disconnect(
                    RailVehicleEnginePowerSource::pantograph_down_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_pantograph_down));
        }
        if (p_entry.brake.is_valid()) {
            p_entry.brake->disconnect(
                    RailVehicleBrake::accelerator_activated_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_accelerator_activated));
        }
        p_entry.controller.unref();
        p_entry.road.unref();
        p_entry.brake.unref();
        p_entry.wheels.unref();
        p_entry.spring_brake.unref();
        p_entry.engine.unref();
        p_entry.power_source.unref();
        p_entry.master_controller.unref();
        p_entry.power_supply.unref();
        p_entry.horns.unref();
        p_entry.security_system.unref();
        p_entry.doors.unref();
        if (!vehicle_server->vehicle_is_simulation_ready(p_vehicle)) {
            return;
        }
        p_entry.controller = vehicle_server->vehicle_get_controller(p_vehicle);
        p_entry.road = p_entry.controller;
        p_entry.wheels = vehicle_server->vehicle_component_get(p_vehicle, VehicleComponentType::COMPONENT_WHEELS);
        p_entry.engine = vehicle_server->vehicle_component_get(p_vehicle, VehicleComponentType::COMPONENT_ENGINE);
        if (p_entry.road.is_valid()) {
            return;
        }
        p_entry.horns = vehicle_server->vehicle_component_get(p_vehicle, VehicleComponentType::COMPONENT_HORNS);
        p_entry.doors = vehicle_server->vehicle_component_get(p_vehicle, VehicleComponentType::COMPONENT_DOORS);
        p_entry.power_supply = rail->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_POWER_SUPPLY);
        p_entry.security_system = rail->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_SECURITY);
        p_entry.brake = rail->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_BRAKES);
        p_entry.spring_brake = rail->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_SPRING_BRAKE);
        p_entry.power_source =
                rail->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_ENGINE_POWER_SOURCE);
        p_entry.master_controller =
                rail->vehicle_component_get(p_vehicle, RailVehicleComponentType::COMPONENT_MASTER_CONTROLLER);
        if (p_entry.power_source.is_valid()) {
            p_entry.power_source->connect(
                    RailVehicleEnginePowerSource::pantograph_up_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_pantograph_up).bind(p_vehicle));
            p_entry.power_source->connect(
                    RailVehicleEnginePowerSource::pantograph_down_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_pantograph_down).bind(p_vehicle));
        }
        if (p_entry.brake.is_valid()) {
            p_entry.brake->connect(
                    RailVehicleBrake::accelerator_activated_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_accelerator_activated).bind(p_vehicle));
        }
    }

    /* The vehicle's last bank is gone: it is no longer counted or fed */
    void MaszynaVehicleSoundServer::_release_vehicle(Vehicle &p_entry, const RID &p_vehicle) {
        p_entry.banks.clear();
        _resolve_vehicle(RID(), p_entry);
        active.erase(p_vehicle);
        vehicles.erase(p_vehicle);
    }

    void MaszynaVehicleSoundServer::_on_vehicle_controller_changed(const RID &p_vehicle) {
        if (!vehicles.has(p_vehicle)) {
            return;
        }
        _resolve_vehicle(p_vehicle, vehicles[p_vehicle]);
        for (const ObjectID &player: vehicles[p_vehicle].banks) {
            _refresh_bank_context(banks[player]);
        }
    }

    void MaszynaVehicleSoundServer::_on_vehicle_freed(const RID &p_vehicle) {
        bank_builders.erase(p_vehicle);
        bank_build_queue.erase(p_vehicle);
        if (vehicles.has(p_vehicle)) {
            _release_vehicle(vehicles[p_vehicle], p_vehicle);
        }
    }

    void MaszynaVehicleSoundServer::_count_event(const RID &p_vehicle, const Quantity p_event) {
        if (vehicles.has(p_vehicle)) {
            vehicles[p_vehicle].events[p_event - QUANTITY_COUPLER_ATTACH_COUPLER]++;
        }
    }

    void MaszynaVehicleSoundServer::_on_coupler_attached(const RID &p_vehicle, const int64_t p_flag) {
        const int event = coupling_event(p_flag);
        if (event >= 0) {
            _count_event(p_vehicle, static_cast<Quantity>(QUANTITY_COUPLER_ATTACH_COUPLER + event));
        }
    }

    void MaszynaVehicleSoundServer::_on_coupler_detached(const RID &p_vehicle, const int64_t p_flag) {
        const int event = coupling_event(p_flag);
        if (event >= 0) {
            _count_event(
                    p_vehicle, static_cast<Quantity>(QUANTITY_COUPLER_ATTACH_COUPLER + COUPLER_DETACH_OFFSET + event));
        }
    }

    void MaszynaVehicleSoundServer::_on_coupler_adapter_attached(const RID &p_vehicle, const int64_t /*p_end*/) {
        _count_event(p_vehicle, QUANTITY_COUPLER_ADAPTER_ATTACH);
    }

    void MaszynaVehicleSoundServer::_on_coupler_adapter_removed(const RID &p_vehicle, const int64_t /*p_end*/) {
        _count_event(p_vehicle, QUANTITY_COUPLER_ADAPTER_REMOVE);
    }

    void MaszynaVehicleSoundServer::_on_derailed(const RID &p_vehicle, const int64_t /*p_cause*/) {
        _count_event(p_vehicle, QUANTITY_DERAILED);
    }

    /* Both pantographs count as one event: the vehicle has one sound for them (sPantUp, DynObj.cpp:3881) */
    void MaszynaVehicleSoundServer::_on_pantograph_up(const int64_t /*p_selector*/, const RID &p_vehicle) {
        _count_event(p_vehicle, QUANTITY_PANTOGRAPH_UP);
    }

    void MaszynaVehicleSoundServer::_on_pantograph_down(const int64_t /*p_selector*/, const RID &p_vehicle) {
        _count_event(p_vehicle, QUANTITY_PANTOGRAPH_DOWN);
    }

    void MaszynaVehicleSoundServer::_on_accelerator_activated(const RID &p_vehicle) {
        if (vehicles.has(p_vehicle)) {
            vehicles[p_vehicle].accelerator_count++;
        }
    }

    void MaszynaVehicleSoundServer::_on_node_added(Node *p_node) {
        TrainSoundListener3D *added = Object::cast_to<TrainSoundListener3D>(p_node);
        if (added == nullptr) {
            return;
        }
        if (TrainSoundListener3D *previous = _get_listener()) {
            previous->disconnect(
                    TrainSoundListener3D::context_changed_signal,
                    callable_mp(this, &MaszynaVehicleSoundServer::_on_listener_context_changed));
        }
        listener = ObjectID(added->get_instance_id());
        added->connect(
                TrainSoundListener3D::context_changed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_listener_context_changed));
        _refresh_context();
    }

    void MaszynaVehicleSoundServer::_on_node_removed(Node *p_node) {
        if (p_node == nullptr || ObjectID(p_node->get_instance_id()) != listener) {
            return;
        }
        p_node->disconnect(
                TrainSoundListener3D::context_changed_signal,
                callable_mp(this, &MaszynaVehicleSoundServer::_on_listener_context_changed));
        listener = ObjectID();
        _refresh_context();
    }

    void MaszynaVehicleSoundServer::_on_listener_context_changed() {
        _refresh_context();
    }

    /* The listener is in a cab of the vehicle */
    bool MaszynaVehicleSoundServer::_is_heard_inside(const RID &p_vehicle) const {
        const TrainSoundListener3D *heard_by = _get_listener();
        return heard_by != nullptr && heard_by->is_inside_vehicle(p_vehicle);
    }

    TrainSoundListener3D *MaszynaVehicleSoundServer::_get_listener() const {
        return Object::cast_to<TrainSoundListener3D>(ObjectDB::get_instance(listener));
    }

    /* The sounds' state moves with the simulation - an axle's run to the next rail joint: its
     * seconds, not the frame's */
    void MaszynaVehicleSoundServer::_on_simulation_advanced(const double p_seconds) {
        sweep_elapsed += p_seconds;
        if (sweep_elapsed >= SWEEP_INTERVAL) {
            sweep_elapsed = 0.0;
            _sweep();
        }
        for (const RID &vehicle: active) {
            Vehicle &entry = vehicles[vehicle];
            entry.update_elapsed += p_seconds;
            if (entry.update_elapsed < entry.update_interval) {
                continue;
            }
            const double elapsed = entry.update_elapsed;
            entry.update_elapsed = 0.0;
            _feed(vehicle, entry, elapsed);
        }
    }

    /* Distance, culling and the vehicles a step feeds - what this server owns about where the
     * listener is, swept every SWEEP_INTERVAL of the simulation's time */
    void MaszynaVehicleSoundServer::_sweep() {
        ProjectSettings *settings = ProjectSettings::get_singleton();
        // a player beyond gnd-sfx's hard cut is silent anyway, so nothing farther is fed
        culling_distance =
                MIN(static_cast<double>(settings->get_setting(CULLING_DISTANCE_SETTING, CULLING_DISTANCE_DEFAULT)),
                    static_cast<double>(settings->get_setting("gnd_sfx/hard_cut_distance", CULLING_DISTANCE_DEFAULT)));
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        RailVehicleServer *rail = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_NULL(rail);
        const TrainSoundListener3D *heard_by = _get_listener();
        const Vector3 listener_position = heard_by != nullptr ? heard_by->get_global_position() : Vector3();
        // the trainset driven from the listener's cab hears the cab's running noise instead of its
        // outer noise (DynObj.cpp:4632-4640)
        TypedArray<RID> listener_trainset;
        if (heard_by != nullptr && heard_by->get_listener_cabin() != nullptr) {
            listener_trainset = rail->vehicle_get_coupled(
                    heard_by->get_listener_vehicle(), RailVehicleController::COUPLER_END_FRONT,
                    RailVehicleController::COUPLING_FLAG_COUPLER);
        }
        active.clear();
        for (KeyValue<RID, Vehicle> &pair: vehicles) {
            Vehicle &entry = pair.value;
            entry.outer_noise_audible = !listener_trainset.has(pair.key);
            const double distance =
                    heard_by != nullptr ? vehicle_server->vehicle_get_position(pair.key).distance_to(listener_position)
                                        : 0.0;
            const bool culled = distance > culling_distance || entry.controller.is_null();
            for (const ObjectID &player: entry.banks) {
                Bank &bank = banks[player];
                _set_bank_cut(bank, culled || (bank.cabin_only && !_is_heard_inside(pair.key)));
            }
            if (culled) {
                continue;
            }
            // the farther away, the less often - its sound is quiet there already
            entry.update_interval = Math::lerp(0.0, FAR_UPDATE_INTERVAL, CLAMP(distance / culling_distance, 0.0, 1.0));
            active.push_back(pair.key);
        }

        // the sound of a vehicle within earshot is built, the nearest first - and not before there
        // is a listener to be near; a queue still draining is refilled once it is empty
        if (heard_by != nullptr && bank_build_queue.is_empty()) {
            LocalVector<Pair<double, RID>> queued;
            for (const KeyValue<RID, Callable> &pair: bank_builders) {
                const double distance = vehicle_server->vehicle_get_position(pair.key).distance_to(listener_position);
                if (distance <= culling_distance) {
                    queued.push_back(Pair<double, RID>(distance, pair.key));
                }
            }
            queued.sort_custom<NearerFirst>();
            for (const Pair<double, RID> &item: queued) {
                bank_build_queue.push_back(item.second);
            }
            SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
            if (!bank_build_queue.is_empty() && !building && tree != nullptr) {
                building = true;
                tree->connect("process_frame", callable_mp(this, &MaszynaVehicleSoundServer::_on_bank_build_frame));
            }
        }
        _set_advancing(!vehicles.is_empty() || !bank_builders.is_empty());
    }

    /* The queued vehicles' sound, a budget a frame: a station's worth of vehicles coming within
     * earshot at once stalled the frame */
    void MaszynaVehicleSoundServer::_on_bank_build_frame() {
        const uint64_t deadline = Time::get_singleton()->get_ticks_msec() + BANK_BUILD_BUDGET_MSEC;
        while (!bank_build_queue.is_empty() && Time::get_singleton()->get_ticks_msec() < deadline) {
            const RID vehicle = bank_build_queue[0];
            bank_build_queue.remove_at(0);
            const Callable builder = bank_builders[vehicle];
            bank_builders.erase(vehicle);
            builder.call();
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (bank_build_queue.is_empty() && tree != nullptr) {
            building = false;
            tree->disconnect("process_frame", callable_mp(this, &MaszynaVehicleSoundServer::_on_bank_build_frame));
        }
    }

    void MaszynaVehicleSoundServer::_set_bank_cut(Bank &p_bank, const bool p_cut) {
        if (p_bank.cut == p_cut) {
            return;
        }
        p_bank.cut = p_cut;
        // what changed while it was cut is the bank's before it hears again: a one-shot due unheard
        // is dropped (sound.cpp:360-367), as its triggers take the values found as they come back
        Vehicle *entry = vehicles.getptr(p_bank.vehicle);
        if (!p_cut && entry != nullptr && entry->controller.is_valid()) {
            entry->feed_serial++;
            _feed_bank(p_bank.vehicle, *entry, p_bank, 0.0);
        }
        // SfxPlayer3D.cut - gnd-sfx's, see the class; a cut bank resumes past its loops' openings
        if (Object *player = ObjectDB::get_instance(p_bank.player)) {
            player->call(set_cut_method, p_cut);
        }
    }

    void MaszynaVehicleSoundServer::_write(Bank &p_bank, const Feed &p_feed, const double p_value) {
        if (p_bank.values[p_feed.index] == p_value) {
            return;
        }
        p_bank.values.set(p_feed.index, p_value);
        p_bank.values_changed = true;
    }

    /* The bank's parameters written since the last flush, in one call */
    void MaszynaVehicleSoundServer::_flush(Bank &p_bank) {
        if (!p_bank.values_changed) {
            return;
        }
        p_bank.values_changed = false;
        // SfxPlayer3D.set_parameter_values() - gnd-sfx's, see the class
        if (Object *player = ObjectDB::get_instance(p_bank.player)) {
            player->call(set_parameter_values_method, p_bank.values);
        }
    }

    void MaszynaVehicleSoundServer::_feed(const RID &p_vehicle, Vehicle &p_entry, const double p_elapsed) {
        if (p_entry.controller.is_null()) {
            return;
        }
        _advance_integrals(p_vehicle, p_entry, p_elapsed);
        p_entry.feed_serial++;
        for (const ObjectID &player: p_entry.banks) {
            Bank &bank = banks[player];
            if (!bank.cut) {
                _feed_bank(p_vehicle, p_entry, bank, p_elapsed);
            }
        }
    }

    void MaszynaVehicleSoundServer::_feed_bank(
            const RID &p_vehicle, Vehicle &p_entry, Bank &p_bank, const double p_elapsed) {
        const bool moving = VehicleServer::get_instance()->vehicle_get_speed(p_vehicle) > 0.0;
        for (QuantityFeed &feed: p_bank.quantities) {
            // the track is read only under a moving vehicle; standing, it keeps what it had
            if (feed.quantity >= QUANTITY_TRACK_QUALITY && feed.quantity <= QUANTITY_CURVE_CANT && !moving) {
                continue;
            }
            // read once a feed for all the vehicle's banks
            if (p_entry.quantity_serials[feed.quantity] != p_entry.feed_serial) {
                p_entry.quantity_values[feed.quantity] = _quantity(p_vehicle, p_entry, feed.quantity);
                p_entry.quantity_serials[feed.quantity] = p_entry.feed_serial;
            }
            _write(p_bank, feed, p_entry.quantity_values[feed.quantity]);
        }
        if (!p_bank.axles.is_empty()) {
            _feed_axles(p_vehicle, p_bank, p_elapsed);
        }
        _flush(p_bank);
    }

    double
    MaszynaVehicleSoundServer::_quantity(const RID &p_vehicle, Vehicle &p_entry, const Quantity p_quantity) const {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        const MaszynaMoverRailVehicleBrake *brake = p_entry.brake.ptr();
        const RailVehicleEngine *engine = p_entry.engine.ptr();
        switch (p_quantity) {
            case QUANTITY_SPEED:
                return vehicle_server->vehicle_get_speed(p_vehicle);
            case QUANTITY_VELOCITY:
                return vehicle_server->vehicle_get_velocity(p_vehicle);
            case QUANTITY_BRAKE_CYLINDER_PRESSURE:
                return brake != nullptr ? brake->get_air_pressure() : 0.0;
            case QUANTITY_BRAKE_LOCAL_PRESSURE:
                return brake != nullptr ? brake->get_loco_pressure() : 0.0;
            case QUANTITY_PIPE_PRESSURE:
                return brake != nullptr ? brake->get_pipe_pressure() : 0.0;
            case QUANTITY_BRAKE_UNIT_FORCE:
                return brake != nullptr ? brake->get_unit_force() : 0.0;
            case QUANTITY_BRAKE_FORCE_RATIO:
                if (p_entry.road.is_valid()) {
                    return p_entry.road->get_brake_force_ratio();
                }
                return brake != nullptr ? brake->get_force_ratio() : 0.0;
            case QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW:
                return brake != nullptr ? brake->get_emergency_valve_flow() : 0.0;
            case QUANTITY_BRAKE_MAIN_VALVE_FLOW:
                return brake != nullptr ? brake->get_main_valve_flow() : 0.0;
            case QUANTITY_BRAKE_RELEASER_ACTIVE:
                return brake != nullptr && brake->get_releaser_active() ? 1.0 : 0.0;
            case QUANTITY_BRAKE_HANDLE_BRAKING_FLOW:
                return brake != nullptr ? brake->get_handle_braking_flow() : 0.0;
            case QUANTITY_BRAKE_HANDLE_RELEASE_FLOW:
                return brake != nullptr ? brake->get_handle_release_flow() : 0.0;
            case QUANTITY_BRAKE_HANDLE_EMERGENCY_FLOW:
                return brake != nullptr ? brake->get_handle_emergency_flow() : 0.0;
            case QUANTITY_BRAKE_HANDLE_CONTROL_CHAMBER_FLOW:
                return brake != nullptr ? brake->get_handle_control_chamber_flow() : 0.0;
            case QUANTITY_BRAKE_HANDLE_TIMING_RESERVOIR_FLOW:
                return brake != nullptr ? brake->get_handle_timing_reservoir_flow() : 0.0;
            case QUANTITY_BRAKE_ACCELERATOR_COUNT:
                return static_cast<double>(p_entry.accelerator_count);
            case QUANTITY_EP_BRAKE_INCREASE_COUNT:
                return static_cast<double>(p_entry.ep_increase_count);
            case QUANTITY_EP_BRAKE_INCREASE_PITCH:
                return p_entry.ep_increase_pitch;
            case QUANTITY_EP_BRAKE_DECREASE_COUNT:
                return static_cast<double>(p_entry.ep_decrease_count);
            case QUANTITY_EP_BRAKE_DECREASE_PITCH:
                return p_entry.ep_decrease_pitch;
            case QUANTITY_SPRING_BRAKE_ACTIVE:
                return p_entry.spring_brake.is_valid() && p_entry.spring_brake->get_active() ? 1.0 : 0.0;
            case QUANTITY_WHEELS_SLIPPING:
                return p_entry.wheels.is_valid() && p_entry.wheels->get_slipping() ? 1.0 : 0.0;
            case QUANTITY_WHEELS_ROTATION:
                return p_entry.wheels.is_valid() ? Math::abs(p_entry.wheels->get_rotation_speed_rps()) : 0.0;
            case QUANTITY_ENGINE_POWER:
                return engine != nullptr ? engine->get_power() : 0.0;
            case QUANTITY_ENGINE_REVOLUTIONS:
                return engine != nullptr ? engine->get_rpm_count() : 0.0;
            case QUANTITY_ENGINE_RPM: {
                if (p_entry.road.is_valid()) {
                    return p_entry.road->is_engine_running()
                                   ? p_entry.road->get_engine_revolutions() * Units::SECONDS_PER_MINUTE
                                   : 0.0;
                }
                const RailVehicleDieselEngine *diesel = Object::cast_to<RailVehicleDieselEngine>(engine);
                return diesel != nullptr ? diesel->get_rpm() : 0.0;
            }
            case QUANTITY_ENGINE_RPM_RATIO:
                return engine != nullptr ? engine->get_rpm_ratio() : 0.0;
            case QUANTITY_ENGINE_FILL: {
                const RailVehicleDieselEngine *diesel = Object::cast_to<RailVehicleDieselEngine>(engine);
                return diesel != nullptr ? diesel->get_fill() : 0.0;
            }
            case QUANTITY_ENGINE_DYNAMIC_BRAKE_ACTIVE: {
                const RailVehicleElectricEngine *electric = Object::cast_to<RailVehicleElectricEngine>(engine);
                return electric != nullptr && electric->get_dynamic_brake_active() ? 1.0 : 0.0;
            }
            case QUANTITY_ENGINE_RESISTOR_FAN_ROTATION: {
                const RailVehicleElectricSeriesEngine *series =
                        Object::cast_to<RailVehicleElectricSeriesEngine>(engine);
                return series != nullptr ? series->get_resistor_fan_rotation() : 0.0;
            }
            case QUANTITY_MOTOR_MOMENTUM:
                return p_entry.motor_momentum;
            case QUANTITY_MASTER_CONTROLLER_POSITION:
                return p_entry.master_controller.is_valid() ? p_entry.master_controller->get_main_position() : 0.0;
            case QUANTITY_CAR_ENGINE_RUNNING:
                return p_entry.road.is_valid() && p_entry.road->is_engine_running() ? 1.0 : 0.0;
            case QUANTITY_CAR_ENGINE_REVOLUTIONS:
                return p_entry.road.is_valid() ? p_entry.road->get_engine_revolutions() : 0.0;
            case QUANTITY_CAR_THROTTLE:
                return p_entry.road.is_valid() ? p_entry.road->get_throttle() : 0.0;
            case QUANTITY_TRACK_QUALITY:
            case QUANTITY_TRACK_ENVIRONMENT:
            case QUANTITY_TRACK_SWITCH: {
                RID track;
                if (p_entry.road.is_valid()) {
                    // a road vehicle hears the track its road is laid along
                    track = RoadServer::get_instance()->road_get_track(
                            RoadVehicleServer::get_instance()->vehicle_get_road(p_vehicle));
                } else {
                    track = RailVehicleServer::get_instance()->vehicle_get_track_position(p_vehicle).get(
                            "track_rid", RID());
                }
                if (p_quantity == QUANTITY_TRACK_SWITCH) {
                    return TrackServer::get_instance()->track_is_switch(track) ? 1.0 : 0.0;
                }
                // the quality and the surroundings are the .scn's, kept in the track's meta (MaszynaTrackData)
                const Dictionary meta = TrackServer::get_instance()->track_get_meta(track);
                if (p_quantity == QUANTITY_TRACK_QUALITY) {
                    return meta.get(MaszynaTrackData::META_QUALITY, 0);
                }
                return meta.get(MaszynaTrackData::META_ENVIRONMENT, MaszynaTrackData::FLAT);
            }
            case QUANTITY_CURVE_RADIUS:
            case QUANTITY_CURVE_CANT: {
                if (p_entry.road.is_valid() || vehicle_server->vehicle_get_speed(p_vehicle) <= CURVE_MIN_SPEED) {
                    return 0.0;
                }
                const Dictionary curve = RailVehicleServer::get_instance()->vehicle_get_curve(p_vehicle);
                return curve.get(p_quantity == QUANTITY_CURVE_RADIUS ? "radius" : "cant", 0.0);
            }
            case QUANTITY_OUTER_NOISE_AUDIBLE:
                return p_entry.outer_noise_audible ? 1.0 : 0.0;
            case QUANTITY_OIL_PUMP_ACTIVE: {
                const RailVehicleDieselEngine *diesel = Object::cast_to<RailVehicleDieselEngine>(engine);
                return diesel != nullptr && diesel->get_oil_pump_active() ? 1.0 : 0.0;
            }
            case QUANTITY_FUEL_PUMP_ACTIVE: {
                const RailVehicleDieselEngine *diesel = Object::cast_to<RailVehicleDieselEngine>(engine);
                return diesel != nullptr && diesel->get_fuel_pump_active() ? 1.0 : 0.0;
            }
            case QUANTITY_BATTERY_ENABLED:
                return p_entry.power_supply.is_valid() && p_entry.power_supply->get_battery_enabled() ? 1.0 : 0.0;
            case QUANTITY_COMPRESSOR_ENABLED:
                return brake != nullptr && brake->get_compressor_enabled() ? 1.0 : 0.0;
            case QUANTITY_CONVERTER_ENABLED:
                return p_entry.power_supply.is_valid() && p_entry.power_supply->get_converter_enabled() ? 1.0 : 0.0;
            case QUANTITY_PANTOGRAPH_COMPRESSOR_ENABLED:
                return p_entry.power_source.is_valid() &&
                                       p_entry.power_source->get_collector_pantograph_compressor_enabled()
                               ? 1.0
                               : 0.0;
            case QUANTITY_HORN_LOW_ACTIVE:
                if (p_entry.road.is_valid()) {
                    return p_entry.road->get_horns().has_flag(RoadVehicleController::HORNS_LOW) ? 1.0 : 0.0;
                }
                return p_entry.horns.is_valid() && p_entry.horns->get_low_active() ? 1.0 : 0.0;
            case QUANTITY_HORN_HIGH_ACTIVE:
                if (p_entry.road.is_valid()) {
                    return p_entry.road->get_horns().has_flag(RoadVehicleController::HORNS_HIGH) ? 1.0 : 0.0;
                }
                return p_entry.horns.is_valid() && p_entry.horns->get_high_active() ? 1.0 : 0.0;
            case QUANTITY_WHISTLE_ACTIVE:
                return p_entry.horns.is_valid() && p_entry.horns->get_whistle_active() ? 1.0 : 0.0;
            case QUANTITY_BEEPING:
                return p_entry.security_system.is_valid() && p_entry.security_system->get_beeping() ? 1.0 : 0.0;
            case QUANTITY_CABSIGNAL_BEEPING:
                return p_entry.security_system.is_valid() && p_entry.security_system->get_cabsignal_beeping() ? 1.0
                                                                                                              : 0.0;
            case QUANTITY_DEPARTURE_SIGNAL_SOUNDING:
                return p_entry.doors.is_valid() && p_entry.doors->get_departure_signal_sounding() ? 1.0 : 0.0;
            case QUANTITY_TACHOMETER_CLOCK_SPEED:
                return p_entry.master_controller.is_valid() ? p_entry.master_controller->get_tachometer_clock_speed()
                                                            : 0.0;
            default:
                return static_cast<double>(p_entry.events[p_quantity - QUANTITY_COUPLER_ATTACH_COUPLER]);
        }
    }

    /* What a bank cannot integrate: the motors' momentum and the EP brake's clicks */
    void MaszynaVehicleSoundServer::_advance_integrals(const RID &p_vehicle, Vehicle &p_entry, const double p_elapsed) {
        // DynObj.cpp:8441, 8497-8502 - only while the wheels turn
        if (p_entry.engine.is_valid() && p_entry.wheels.is_valid() &&
            Math::abs(p_entry.wheels->get_rotation_speed_rps()) > MOTOR_MIN_ROTATION) {
            p_entry.motor_momentum = CLAMP(
                    p_entry.motor_momentum - p_elapsed +
                            (Math::abs(p_entry.engine->get_motor_torque()) / MOTOR_MOMENTUM_TORQUE_DIVISOR * p_elapsed),
                    0.0, MOTOR_MOMENTUM_MAX);
        }
        const MaszynaMoverRailVehicleBrake *brake = p_entry.brake.ptr();
        if (brake == nullptr || brake->get_cntrl_brake_system() != RailVehicleBrake::BRAKE_SYSTEM_ELECTRO_PNEUMATIC) {
            return;
        }
        // DynObj.cpp:4569-4613; MaxBrakePress[0] and [3]
        const double max_control_pressure = brake->get_max_aux_pressure() < AUX_PRESSURE_MIN
                                                    ? brake->get_max_cylinder_pressure()
                                                    : brake->get_max_aux_pressure();
        const double control_divisor = std::max(1.0, max_control_pressure);
        const double ratio = std::min(
                std::max(0.0, brake->get_control_pressure()) / control_divisor,
                p_entry.ep_decrease_timer > EP_TIMER_DISABLED
                        ? std::max(
                                  brake->get_local_aeim_position(),
                                  brake->get_edb_cylinder_pressure() / brake->get_max_cylinder_pressure())
                        : 1.0);
        if (p_entry.last_ep_pressure != UNSET_PRESSURE) {
            p_entry.ep_increase_timer += p_elapsed;
            p_entry.ep_decrease_timer += p_elapsed;
            const int step = static_cast<int>(EP_STEPS * ratio);
            const int change =
                    step - static_cast<int>(EP_STEPS * std::max(0.0, p_entry.last_ep_pressure) / control_divisor);
            if (change > 0 && p_entry.ep_increase_timer > EP_INCREASE_INTERVAL) {
                p_entry.ep_increase_pitch = step * CHUNK_POINT_SCALE;
                p_entry.ep_increase_count++;
                p_entry.ep_increase_timer = 0.0;
            } else if (change < 0 && p_entry.ep_decrease_timer > EP_DECREASE_INTERVAL) {
                p_entry.ep_decrease_pitch = -change * CHUNK_POINT_SCALE;
                p_entry.ep_decrease_count++;
                p_entry.ep_decrease_timer = 0.0;
            }
        }
        if (p_entry.ep_increase_timer == 0.0 || p_entry.ep_decrease_timer == 0.0) {
            p_entry.last_ep_pressure =
                    std::min(brake->get_control_pressure(), brake->get_local_aeim_position() * control_divisor);
        }
    }

    /* DynObj.cpp:3477-3545 - each axle counts its way along the rail and clicks when it passes a
     * joint. The axles start phased by their position on the track (DynObj.cpp:2283), a later rail
     * length change re-phases them by their offsets. */
    void MaszynaVehicleSoundServer::_feed_axles(const RID &p_vehicle, Bank &p_bank, const double p_elapsed) {
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        const double speed = vehicle_server->vehicle_get_speed(p_vehicle);
        if (speed <= 0.0) {
            return;
        }
        const Vehicle &entry = vehicles[p_vehicle];
        RID track;
        double along = 0.0;
        if (entry.road.is_valid()) {
            track = RoadServer::get_instance()->road_get_track(
                    RoadVehicleServer::get_instance()->vehicle_get_road(p_vehicle));
        } else {
            const Dictionary position = RailVehicleServer::get_instance()->vehicle_get_track_position(p_vehicle);
            track = position.get("track_rid", RID());
            along = position.get("along", 0.0);
        }
        const double spacing = TrackServer::get_instance()->track_get_joint_spacing(track);
        if (Math::is_equal_approx(spacing, NO_JOINTS)) {
            return;
        }
        if (!Math::is_equal_approx(spacing, p_bank.rail_length)) {
            for (AxleFeed &axle: p_bank.axles) {
                if (p_bank.rail_length > 0.0) {
                    axle.distance = axle.offset;
                } else if (spacing > 0.0) {
                    axle.distance = Math::fposmod(axle.offset + along, spacing);
                }
            }
            p_bank.rail_length = spacing;
        }
        if (p_bank.rail_length <= 0.0) {
            return;
        }
        const double run = vehicle_server->vehicle_get_velocity(p_vehicle) * p_elapsed;
        for (AxleFeed &axle: p_bank.axles) {
            axle.distance += run;
            if (axle.distance >= 0.0 && axle.distance <= p_bank.rail_length) {
                continue;
            }
            axle.distance = Math::fposmod(axle.distance, p_bank.rail_length);
            if (speed > CLATTER_MIN_SPEED) {
                axle.clicks++;
                _write(p_bank, axle, static_cast<double>(axle.clicks));
            }
        }
    }

    void MaszynaVehicleSoundServer::_refresh_context() {
        _refresh_exterior_wall();
        for (KeyValue<ObjectID, Bank> &pair: banks) {
            _refresh_bank_context(pair.value);
        }
        // the listener moved between places: where it is now decides what is heard at once
        _sweep();
    }

    /* What a bank hears of where the listener is: its soundproofing, whether a cab's bank is heard
     * at all, and where the cab's controls it sounds from are */
    void MaszynaVehicleSoundServer::_refresh_bank_context(Bank &p_bank) {
        if (p_bank.cabin_only) {
            _set_bank_cut(p_bank, !_is_heard_inside(p_bank.vehicle));
        }
        for (const ProofingFeed &feed: p_bank.proofing) {
            _write(p_bank, feed, _soundproofing(p_bank, feed));
        }
        _flush(p_bank);
        _anchor(p_bank);
    }

    double MaszynaVehicleSoundServer::_soundproofing(const Bank &p_bank, const ProofingFeed &p_feed) const {
        const TrainSoundListener3D *heard_by = _get_listener();
        const auto profile_value = [this](const RID &p_vehicle, const int p_placement, const int p_context) {
            const Vehicle *entry = vehicles.getptr(p_vehicle);
            if (entry != nullptr && entry->soundproofing.size() == PROFILE_PLACEMENTS) {
                const double value = entry->soundproofing[p_placement][p_context];
                if (!Math::is_equal_approx(value, PROFILE_UNSET)) {
                    return Math::sqrt(CLAMP(value, 0.0, 1.0));
                }
            }
            return Math::sqrt(DEFAULT_PROOFING[p_placement][p_context]);
        };
        // the sound's own profile first, then its vehicle's, then the original's default
        const auto source_value = [&](const int p_context) {
            if (p_feed.profile.size() == PROFILE_CONTEXTS &&
                !Math::is_equal_approx(static_cast<double>(p_feed.profile[p_context]), PROFILE_UNSET)) {
                return Math::sqrt(CLAMP(static_cast<double>(p_feed.profile[p_context]), 0.0, 1.0));
            }
            return profile_value(p_bank.vehicle, p_feed.placement, p_context);
        };
        const bool inside = heard_by != nullptr && heard_by->is_inside_vehicle(p_bank.vehicle);
        if (p_feed.placement == PLACEMENT_INTERNAL && inside) {
            return source_value(heard_by->get_listener_context());
        }
        if (heard_by == nullptr || !heard_by->get_listener_vehicle().is_valid()) {
            return p_feed.placement == PLACEMENT_INTERNAL ? 0.0 : source_value(TrainSoundListener3D::EXTERIOR_CONTEXT);
        }
        double proofing =
                source_value(inside ? heard_by->get_listener_context() : TrainSoundListener3D::EXTERIOR_CONTEXT);
        // heard from another vehicle's cab: through its walls too
        if (heard_by->get_listener_vehicle() != p_bank.vehicle) {
            proofing *= profile_value(
                    heard_by->get_listener_vehicle(), PLACEMENT_EXTERNAL, heard_by->get_listener_context());
        }
        return proofing;
    }

    /* The cab's sounds sit at their controls: the brake hisses at their handle, the local brake's at
     * the driver's brake valve when the cab has no local brake handle (Train.cpp:9406-9421), the
     * Hasler at its needle (Train.cpp:10281). The cab's controls are its GDScript nodes, found by
     * their names; the events are gnd-sfx's (see the class). */
    void MaszynaVehicleSoundServer::_anchor(Bank &p_bank) {
        const TrainSoundListener3D *heard_by = _get_listener();
        Cabin3D *cabin = heard_by != nullptr ? heard_by->get_listener_cabin() : nullptr;
        if (cabin == nullptr || cabin->get_vehicle_rid() != p_bank.vehicle ||
            (p_bank.brake_valve_events.is_empty() && p_bank.local_brake_events.is_empty() &&
             p_bank.tachometer_events.is_empty())) {
            return;
        }
        // Home/End rebuilds the controls inside the same cabin node, so the shown cabin is part of the key
        const uint64_t key = cabin->get_instance_id() ^ cabin->get_cabin().get_id();
        if (key == p_bank.anchored_cabin) {
            return;
        }
        p_bank.anchored_cabin = key;
        const Transform3D to_vehicle =
                RailVehicleRenderingServer::get_instance()->vehicle_get_transform(p_bank.vehicle).affine_inverse();
        const auto control_position = [&](const String &p_prefix) {
            const Node *widget = cabin->find_child(p_prefix + String("*"), true, false);
            if (widget == nullptr) {
                return Vector3();
            }
            Variant mesh_path = widget->get("mesh_path");
            if (mesh_path.get_type() == Variant::NIL) {
                mesh_path = widget->get("target_mesh_path");
            }
            const Node3D *mesh = Object::cast_to<Node3D>(widget->get_node_or_null(mesh_path));
            return mesh != nullptr ? to_vehicle.xform(mesh->get_global_position()) : Vector3();
        };
        const Vector3 fallback =
                (cabin->get_camera_bound_min() + cabin->get_camera_bound_max()) * 0.5 + Vector3(0, 1, 0);
        Vector3 brake_valve = control_position("brakectrl_");
        Vector3 local_brake = control_position("localbrake_");
        if (local_brake == Vector3()) {
            local_brake = brake_valve;
        }
        if (brake_valve == Vector3()) {
            brake_valve = fallback;
        }
        if (local_brake == Vector3()) {
            local_brake = fallback;
        }
        const Vector3 tachometer = control_position("tachometer_");
        Object *player = ObjectDB::get_instance(p_bank.player);
        Object *sfx_bank = player != nullptr ? Object::cast_to<Object>(player->get("bank")) : nullptr;
        if (sfx_bank == nullptr) {
            return;
        }
        const auto place = [sfx_bank](const PackedStringArray &p_events, const Vector3 &p_position) {
            for (const String &name: p_events) {
                Object *event = Object::cast_to<Object>(sfx_bank->call("get_event", StringName(name)));
                Object *spatial = event != nullptr ? Object::cast_to<Object>(event->get("spatial_config")) : nullptr;
                if (spatial != nullptr) {
                    spatial->set("position", p_position);
                }
            }
        };
        place(p_bank.brake_valve_events, brake_valve);
        place(p_bank.local_brake_events, local_brake);
        if (tachometer != Vector3()) {
            place(p_bank.tachometer_events, tachometer);
        }
    }

    /* Follows the listener into and out of the cab: outside is open, a closed cab is muffled, an
     * open window is most of the way back to open. The wall is the first low-pass of the Exterior
     * bus, looked up by type so a reordered chain cannot silently retune something else. */
    void MaszynaVehicleSoundServer::_refresh_exterior_wall() {
        AudioServer *audio = AudioServer::get_singleton();
        const int32_t bus = audio->get_bus_index(EXTERIOR_BUS);
        if (bus < 0) {
            return;
        }
        Ref<AudioEffectLowPassFilter> filter;
        for (int32_t effect = 0; effect < audio->get_bus_effect_count(bus) && filter.is_null(); effect++) {
            filter = audio->get_bus_effect(bus, effect);
        }
        SceneTree *tree = Object::cast_to<SceneTree>(Engine::get_singleton()->get_main_loop());
        if (filter.is_null() || tree == nullptr) {
            return;
        }
        const TrainSoundListener3D *heard_by = _get_listener();
        double cutoff_hz = WALL_OPEN_HZ;
        double volume_db = WALL_OPEN_DB;
        if (heard_by != nullptr && heard_by->get_listener_cabin() != nullptr) {
            const bool open_window = heard_by->get_listener_context() == OPEN_WINDOW_CONTEXT;
            cutoff_hz = open_window ? WALL_WINDOW_HZ : WALL_CABIN_HZ;
            volume_db = open_window ? WALL_WINDOW_DB : WALL_CABIN_DB;
        }
        if (wall_tween.is_valid()) {
            wall_tween->kill();
        }
        wall_tween = tree->create_tween();
        wall_tween->set_parallel(true);
        wall_tween->tween_property(filter.ptr(), "cutoff_hz", cutoff_hz, WALL_FADE_SECONDS);
        wall_tween->tween_method(
                callable_mp(this, &MaszynaVehicleSoundServer::_set_exterior_bus_volume), audio->get_bus_volume_db(bus),
                volume_db, WALL_FADE_SECONDS);
    }

    void MaszynaVehicleSoundServer::_set_exterior_bus_volume(const double p_volume_db) {
        AudioServer *audio = AudioServer::get_singleton();
        audio->set_bus_volume_db(audio->get_bus_index(EXTERIOR_BUS), static_cast<float>(p_volume_db));
    }

    /* The buses the vehicles are heard on are silent while the world is paused or set to run faster
     * than SILENT_ABOVE_SPEED - with every voice on them. The speed set, not the running one: coming
     * down from above, the running speed approaches the limit from above for seconds. */
    void MaszynaVehicleSoundServer::_on_runtime_paused_changed() {
        const SimulationServer *simulation = SimulationServer::get_instance();
        AudioServer *audio = AudioServer::get_singleton();
        const bool silent =
                simulation->simulation_is_paused() || simulation->get_simulation_speed() > SILENT_ABOVE_SPEED;
        audio->set_bus_mute(audio->get_bus_index(CABIN_BUS), silent);
        audio->set_bus_mute(audio->get_bus_index(EXTERIOR_BUS), silent);
    }

    /* The whole audio's speed - the UI's sounds too (TODO.md) */
    void MaszynaVehicleSoundServer::_on_simulation_current_speed_changed() {
        AudioServer::get_singleton()->set_playback_speed_scale(
                static_cast<float>(
                        MIN(SimulationServer::get_instance()->simulation_get_current_speed(), TAPE_MAX_SPEED)));
    }
} // namespace godot
