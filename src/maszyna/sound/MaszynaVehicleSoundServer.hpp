#pragma once
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/tween.hpp>
#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>

namespace godot {
    class AudioEffectLowPassFilter;
    class Node;
    class MaszynaMoverRailVehicleBrake;
    class RailVehicleDieselEngine;
    class RailVehicleDoors;
    class RailVehicleHorns;
    class RailVehiclePowerSupply;
    class RailVehicleSecuritySystem;
    class RailVehicleElectricEngine;
    class RailVehicleElectricSeriesEngine;
    class RailVehicleEngine;
    class RailVehicleEnginePowerSource;
    class RailVehicleMasterController;
    class RailVehicleSpringBrake;
    class RailVehicleWheels;
    class RoadVehicleController;
    class TrainSoundListener3D;
    class VehicleController;

    /* The sound of MaSzyna's vehicles. A vehicle's sound is built once it is within earshot of the
     * listener (vehicle_set_bank_builder()); every bank built for it (one gnd-sfx player) is
     * registered here (bank_register()). The bank carries the original's formulas - its derived
     * parameters, triggers and automations (MmdSoundBankInstancer); this server only feeds it the
     * vehicle's raw quantities by the names the bank declares, per simulation step, the nearer the
     * listener the more often, and the listener-dependent soundproofing whenever the listener
     * moves between cabs. What the bank cannot integrate - the traction motor's momentum, the rail
     * joints an axle passes, the EP brake's clicks - is counted here.
     *
     * gnd-sfx is a GDExtension of its own: its players and resources are no class this library is
     * built against, so they are reached through call()/get()/set() by name, each place saying so. */
    class MaszynaVehicleSoundServer : public Object {
            GDCLASS(MaszynaVehicleSoundServer, Object)

        public:
            static MaszynaVehicleSoundServer *get_instance() {
                return singleton;
            }

            /* Where a sound sits, the rows of a soundproofing profile (sound_placement, sound.h) */
            enum Placement {
                PLACEMENT_INTERNAL,
                PLACEMENT_ENGINE,
                PLACEMENT_EXTERNAL,
                PLACEMENT_GENERAL,
                PLACEMENT_CUSTOM,
            };

            /* The raw quantities a bank may declare as parameters, by the names quantity_get_name() gives */
            enum Quantity {
                QUANTITY_SPEED,
                QUANTITY_VELOCITY,
                QUANTITY_BRAKE_CYLINDER_PRESSURE,
                QUANTITY_BRAKE_LOCAL_PRESSURE,
                QUANTITY_BRAKE_PIPE_PRESSURE,
                QUANTITY_BRAKE_UNIT_FORCE,
                QUANTITY_BRAKE_FORCE_RATIO,
                QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW,
                QUANTITY_BRAKE_MAIN_VALVE_FLOW,
                QUANTITY_BRAKE_RELEASER_ACTIVE,
                QUANTITY_BRAKE_HANDLE_BRAKING_FLOW,
                QUANTITY_BRAKE_HANDLE_RELEASE_FLOW,
                QUANTITY_BRAKE_HANDLE_EMERGENCY_FLOW,
                QUANTITY_BRAKE_HANDLE_CONTROL_CHAMBER_FLOW,
                QUANTITY_BRAKE_HANDLE_TIMING_RESERVOIR_FLOW,
                QUANTITY_BRAKE_ACCELERATOR_COUNT,
                QUANTITY_EP_BRAKE_INCREASE_COUNT,
                QUANTITY_EP_BRAKE_INCREASE_PITCH,
                QUANTITY_EP_BRAKE_DECREASE_COUNT,
                QUANTITY_EP_BRAKE_DECREASE_PITCH,
                QUANTITY_SPRING_BRAKE_ACTIVE,
                QUANTITY_WHEELS_SLIPPING,
                QUANTITY_WHEELS_ROTATION,
                QUANTITY_ENGINE_POWER,
                QUANTITY_ENGINE_REVOLUTIONS,
                QUANTITY_ENGINE_RPM,
                QUANTITY_ENGINE_RPM_RATIO,
                QUANTITY_ENGINE_FILL,
                QUANTITY_ENGINE_DYNAMIC_BRAKE_ACTIVE,
                QUANTITY_ENGINE_RESISTOR_FAN_ROTATION,
                QUANTITY_MOTOR_MOMENTUM,
                QUANTITY_MASTER_CONTROLLER_POSITION,
                QUANTITY_CAR_ENGINE_RUNNING,
                QUANTITY_CAR_ENGINE_REVOLUTIONS,
                QUANTITY_CAR_THROTTLE,
                QUANTITY_TRACK_QUALITY,
                QUANTITY_TRACK_ENVIRONMENT,
                QUANTITY_TRACK_SWITCH,
                QUANTITY_CURVE_RADIUS,
                QUANTITY_CURVE_CANT,
                QUANTITY_OUTER_NOISE_AUDIBLE,
                /* What the MMD's regular sounds follow (MmdSoundCatalog's state names) */
                QUANTITY_OIL_PUMP_ACTIVE,
                QUANTITY_FUEL_PUMP_ACTIVE,
                QUANTITY_BATTERY_ENABLED,
                QUANTITY_COMPRESSOR_ENABLED,
                QUANTITY_CONVERTER_ENABLED,
                QUANTITY_PANTOGRAPH_COMPRESSOR_ENABLED,
                QUANTITY_HORN_LOW_ACTIVE,
                QUANTITY_HORN_HIGH_ACTIVE,
                QUANTITY_WHISTLE_ACTIVE,
                QUANTITY_BEEPING,
                QUANTITY_CABSIGNAL_BEEPING,
                QUANTITY_DEPARTURE_SIGNAL_SOUNDING,
                QUANTITY_TACHOMETER_CLOCK_SPEED,
                /* The vehicle's events, each a count that only goes up */
                QUANTITY_COUPLER_ATTACH_COUPLER,
                QUANTITY_COUPLER_ATTACH_BRAKEHOSE,
                QUANTITY_COUPLER_ATTACH_MAINHOSE,
                QUANTITY_COUPLER_ATTACH_CONTROL,
                QUANTITY_COUPLER_ATTACH_GANGWAY,
                QUANTITY_COUPLER_ATTACH_HEATING,
                QUANTITY_COUPLER_DETACH_COUPLER,
                QUANTITY_COUPLER_DETACH_BRAKEHOSE,
                QUANTITY_COUPLER_DETACH_MAINHOSE,
                QUANTITY_COUPLER_DETACH_CONTROL,
                QUANTITY_COUPLER_DETACH_GANGWAY,
                QUANTITY_COUPLER_DETACH_HEATING,
                QUANTITY_COUPLER_ADAPTER_ATTACH,
                QUANTITY_COUPLER_ADAPTER_REMOVE,
                QUANTITY_PANTOGRAPH_UP,
                QUANTITY_PANTOGRAPH_DOWN,
                QUANTITY_DERAILED,
                QUANTITY_MAX,
            };

        private:
            static inline MaszynaVehicleSoundServer *singleton = nullptr;

            /* How often the distance to the listener is looked at [s of simulated time] */
            static constexpr double SWEEP_INTERVAL = 0.25;
            /* A vehicle at the culling distance is fed this seldom [s], one at the listener every step */
            static constexpr double FAR_UPDATE_INTERVAL = 0.5;
            /* Time the banks of vehicles coming within earshot may take to build per frame [ms]; a
             * bank that started is finished, so a frame builds at least one */
            static constexpr int BANK_BUILD_BUDGET_MSEC = 8;
            static constexpr double CULLING_DISTANCE_DEFAULT = 1000.0;
            static constexpr const char *CULLING_DISTANCE_SETTING = "maszyna/sound/culling_distance";
            /* The sound runs at the simulation's speed, like a tape, up to this multiple */
            static constexpr double TAPE_MAX_SPEED = 4.0;
            /* Set faster than this, the vehicles go silent */
            static constexpr double SILENT_ABOVE_SPEED = 8.0;
            /* The buses the vehicles are heard on: outside and the cab's own */
            static constexpr const char *EXTERIOR_BUS = "Exterior";
            static constexpr const char *CABIN_BUS = "Cabin";
            /* The cab wall, as heard from inside - the Exterior bus's low-pass corner and trim per
             * listener context: outside, closed cab, cab with an open window; moved between in a fade */
            static constexpr double WALL_OPEN_HZ = 20500.0;
            static constexpr double WALL_CABIN_HZ = 1600.0;
            static constexpr double WALL_WINDOW_HZ = 4000.0;
            static constexpr double WALL_OPEN_DB = 0.0;
            static constexpr double WALL_CABIN_DB = -2.0;
            static constexpr double WALL_WINDOW_DB = -2.0;
            static constexpr double WALL_FADE_SECONDS = 0.15;
            /* Cabin3D.get_sound_listener_context() while the cab window is open */
            static constexpr int OPEN_WINDOW_CONTEXT = 3;
            /* A soundproofing profile: a row per placement, a column per listener context */
            static constexpr int PROFILE_PLACEMENTS = 5;
            static constexpr int PROFILE_CONTEXTS = 6;
            /* An entry of a profile the MMD leaves to the default */
            static constexpr double PROFILE_UNSET = -1.0;

            /* Original engine: the traction motor's momentum, which its gain follows - it decays
             * by 1 a second and grows by the torque over 60, up to 1.25 (DynObj.cpp:7998-8001) */
            static constexpr double MOTOR_MOMENTUM_MAX = 1.25;
            static constexpr double MOTOR_MOMENTUM_TORQUE_DIVISOR = 60.0;
            /* ... while the wheels turn faster than this [1/s] (DynObj.cpp:8441) */
            static constexpr double MOTOR_MIN_ROTATION = 0.01;
            /* An axle clicks at a rail joint only above this speed [km/h] (DynObj.cpp:3518) */
            static constexpr double CLATTER_MIN_SPEED = 0.1;
            /* A track without rail joints (TrackServer.track_get_joint_spacing()) */
            static constexpr double NO_JOINTS = -1.0;
            /* The EP brake clicks once per 1/50 of its pressure, an increase at most every 0.05 s and
             * a decrease every 0.3 s; its chunk is the click's step over 100 (DynObj.cpp:4569-4613) */
            static constexpr double EP_STEPS = 50.0;
            static constexpr double EP_INCREASE_INTERVAL = 0.05;
            static constexpr double EP_DECREASE_INTERVAL = 0.3;
            static constexpr double EP_TIMER_DISABLED = -1.0;
            static constexpr double UNSET_PRESSURE = -1.0;
            /* A combined sound's click picks its chunk by its step over 100 (DynObj.cpp:4593) */
            static constexpr double CHUNK_POINT_SCALE = 0.01;
            /* Mover.cpp, LoadFIZ_Brake: an auxiliary pressure below this is not given, and
             * MaxBrakePress[0] is the cylinders' */
            static constexpr double AUX_PRESSURE_MIN = 0.01;

            /* One of a bank's parameters this server writes: its name and its place in
             * SfxBank.parameters, the order of Bank.values */
            struct Feed {
                    StringName parameter;
                    int64_t index = 0;
            };
            struct QuantityFeed : Feed {
                    Quantity quantity = QUANTITY_SPEED;
            };
            /* An event's soundproofing, by where it sits and its own profile, if it has one */
            struct ProofingFeed : Feed {
                    Placement placement = PLACEMENT_GENERAL;
                    PackedFloat32Array profile;
            };
            /* An axle of the wheel clatter: how far it is past its last rail joint and its clicks */
            struct AxleFeed : Feed {
                    double offset = 0.0;
                    double distance = 0.0;
                    int64_t clicks = 0;
            };

            struct Bank {
                    ObjectID player;
                    RID vehicle;
                    /* Its raw parameters as last written, in SfxBank.parameters' order - handed to the
                     * player in one call when one has changed */
                    PackedFloat64Array values;
                    bool values_changed = false;
                    bool cabin_only = false;
                    bool cut = false;
                    LocalVector<QuantityFeed> quantities;
                    LocalVector<ProofingFeed> proofing;
                    LocalVector<AxleFeed> axles;
                    /* The rail joints' spacing the axles were phased for [m] */
                    double rail_length = 0.0;
                    /* The events heard from the cab's controls, placed at them when the cab is shown */
                    PackedStringArray brake_valve_events;
                    PackedStringArray local_brake_events;
                    PackedStringArray tachometer_events;
                    uint64_t anchored_cabin = 0;
            };

            /* What a vehicle's banks are fed from, taken once per controller */
            struct Vehicle {
                    LocalVector<ObjectID> banks;
                    Ref<VehicleController> controller;
                    Ref<RoadVehicleController> road;
                    Ref<MaszynaMoverRailVehicleBrake> brake;
                    Ref<RailVehicleWheels> wheels;
                    Ref<RailVehicleSpringBrake> spring_brake;
                    Ref<RailVehicleEngine> engine;
                    Ref<RailVehicleEnginePowerSource> power_source;
                    Ref<RailVehicleMasterController> master_controller;
                    Ref<RailVehiclePowerSupply> power_supply;
                    Ref<RailVehicleHorns> horns;
                    Ref<RailVehicleSecuritySystem> security_system;
                    Ref<RailVehicleDoors> doors;
                    /* The counts of its events, by Quantity from QUANTITY_COUPLER_ATTACH_COUPLER */
                    LocalVector<int64_t> events;
                    int64_t accelerator_count = 0;
                    double motor_momentum = 0.0;
                    /* m_lastepbrakepressure and its timers (DynObj.h:542-545) */
                    double last_ep_pressure = UNSET_PRESSURE;
                    double ep_increase_timer = 0.0;
                    double ep_decrease_timer = 0.0;
                    int64_t ep_increase_count = 0;
                    double ep_increase_pitch = 0.0;
                    int64_t ep_decrease_count = 0;
                    double ep_decrease_pitch = 0.0;
                    /* Its soundproofing profile (the MMD's), five rows of six */
                    LocalVector<PackedFloat32Array> soundproofing;
                    /* Within the culling distance: fed every update_interval of simulated time */
                    bool active = false;
                    double update_interval = 0.0;
                    double update_elapsed = 0.0;
                    bool outer_noise_audible = true;
                    /* Its quantities as read in the feed of feed_serial, for all its banks */
                    uint64_t feed_serial = 0;
                    double quantity_values[QUANTITY_MAX] = {};
                    uint64_t quantity_serials[QUANTITY_MAX] = {};
            };

            HashMap<ObjectID, Bank> banks;
            HashMap<RID, Vehicle> vehicles;
            LocalVector<RID> active;
            HashMap<RID, Callable> bank_builders;
            LocalVector<RID> bank_build_queue;
            ObjectID listener;
            double culling_distance = CULLING_DISTANCE_DEFAULT;
            double sweep_elapsed = 0.0;
            bool wired = false;
            bool advancing = false;
            bool building = false;
            Ref<Tween> wall_tween;
            StringName quantity_names[QUANTITY_MAX];
            /* gnd-sfx's player methods this server calls, by their names (see the class) */
            StringName set_parameter_values_method;
            StringName set_cut_method;

            void _wire();
            void _set_advancing(bool p_advancing);
            void _on_simulation_advanced(double p_seconds);
            void _on_bank_build_frame();
            void _on_node_added(Node *p_node);
            void _on_node_removed(Node *p_node);
            void _on_listener_context_changed();
            void _on_vehicle_controller_changed(const RID &p_vehicle);
            void _on_vehicle_freed(const RID &p_vehicle);
            void _on_coupler_attached(const RID &p_vehicle, int64_t p_flag);
            void _on_coupler_detached(const RID &p_vehicle, int64_t p_flag);
            void _on_coupler_adapter_attached(const RID &p_vehicle, int64_t p_end);
            void _on_coupler_adapter_removed(const RID &p_vehicle, int64_t p_end);
            void _on_derailed(const RID &p_vehicle, int64_t p_cause);
            void _on_pantograph_up(int64_t p_selector, const RID &p_vehicle);
            void _on_pantograph_down(int64_t p_selector, const RID &p_vehicle);
            void _on_accelerator_activated(const RID &p_vehicle);
            void _on_bank_exiting(uint64_t p_player);
            void _on_runtime_paused_changed();
            void _on_simulation_current_speed_changed();
            void _count_event(const RID &p_vehicle, Quantity p_event);
            void _resolve_vehicle(const RID &p_vehicle, Vehicle &p_entry);
            void _release_vehicle(Vehicle &p_entry, const RID &p_vehicle);
            void _sweep();
            void _feed(const RID &p_vehicle, Vehicle &p_entry, double p_elapsed);
            void _feed_bank(const RID &p_vehicle, Vehicle &p_entry, Bank &p_bank, double p_elapsed);
            double _quantity(const RID &p_vehicle, Vehicle &p_entry, Quantity p_quantity) const;
            void _advance_integrals(const RID &p_vehicle, Vehicle &p_entry, double p_elapsed);
            void _feed_axles(const RID &p_vehicle, Bank &p_bank, double p_elapsed);
            void _refresh_context();
            void _refresh_bank_context(Bank &p_bank);
            void _refresh_exterior_wall();
            void _set_exterior_bus_volume(double p_volume_db);
            void _set_bank_cut(Bank &p_bank, bool p_cut);
            void _write(Bank &p_bank, const Feed &p_feed, double p_value);
            void _flush(Bank &p_bank);
            double _soundproofing(const Bank &p_bank, const ProofingFeed &p_feed) const;
            void _anchor(Bank &p_bank);
            TrainSoundListener3D *_get_listener() const;
            bool _is_heard_inside(const RID &p_vehicle) const;

        protected:
            static void _bind_methods();

        public:
            MaszynaVehicleSoundServer();
            ~MaszynaVehicleSoundServer() override;

            StringName quantity_get_name(Quantity p_quantity) const;
            /* The vehicle's sound is built by p_builder once the vehicle is within the culling
             * distance of the listener - its banks register as they are built. An empty Callable takes
             * it back: the vehicle is built anew or gone. */
            void vehicle_set_bank_builder(const RID &p_vehicle, const Callable &p_builder);
            /* A gnd-sfx player of p_vehicle, its bank built. p_registration: "cabin_only" (heard only
             * from the vehicle's own cab), "soundproofing" (the vehicle's profile, five rows of six),
             * "proofing" ({parameter, placement, profile} per soundproofed sound), "axles" (the wheel clatter's axle
             * offsets, parameters axle_clicks_<index>), "brake_valve_events", "local_brake_events",
             * "tachometer_events". Every other parameter of the bank is a quantity by its name. */
            void bank_register(uint64_t p_player, const RID &p_vehicle, const Dictionary &p_registration);
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaVehicleSoundServer::Placement);
VARIANT_ENUM_CAST(MaszynaVehicleSoundServer::Quantity);
