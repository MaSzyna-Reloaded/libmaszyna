#include "Cabin3D.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "vehicles/base/VehicleComponentType.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    namespace {
        /* The soundproofing columns (sound.cpp:981-1008): CabOccupied + 1, and an open window */
        constexpr int SOUND_LISTENER_REAR_CAB = 0;
        constexpr int SOUND_LISTENER_MACHINE_ROOM = 1;
        constexpr int SOUND_LISTENER_FRONT_CAB = 2;
        constexpr int SOUND_LISTENER_OPEN_WINDOW = 3;
    } // namespace

    const char *Cabin3D::cabin_ready_signal = "cabin_ready";
    const char *Cabin3D::camera_configuration_changed_signal = "camera_configuration_changed";
    const char *Cabin3D::vehicle_rid_changed_signal = "vehicle_rid_changed";
    const char *Cabin3D::cabin_changed_signal = "cabin_changed";

    void Cabin3D::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_vehicle_rid"), &Cabin3D::get_vehicle_rid);
        ClassDB::bind_method(D_METHOD("get_camera_transform"), &Cabin3D::get_camera_transform);
        ClassDB::bind_method(D_METHOD("get_camera_shake_offset"), &Cabin3D::get_camera_shake_offset);
        ClassDB::bind_method(D_METHOD("get_camera_shake_roll"), &Cabin3D::get_camera_shake_roll);
        ClassDB::bind_method(D_METHOD("is_cabin_ready"), &Cabin3D::is_cabin_ready);
        ClassDB::bind_method(D_METHOD("get_sound_listener_context"), &Cabin3D::get_sound_listener_context);

        ClassDB::bind_method(D_METHOD("set_cabin", "cabin"), &Cabin3D::set_cabin);
        ClassDB::bind_method(D_METHOD("get_cabin"), &Cabin3D::get_cabin);

        ClassDB::bind_method(D_METHOD("set_has_cab_model", "has_cab_model"), &Cabin3D::set_has_cab_model);
        ClassDB::bind_method(D_METHOD("get_has_cab_model"), &Cabin3D::get_has_cab_model);
        /* False when the cab has no hi-fi model (MMD "cabNmodel: none" or missing) - the
         * vehicle's low-poly interior then stays fully visible instead (DynObj.cpp:1214). */
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "has_cab_model"), "set_has_cab_model", "get_has_cab_model");

        ClassDB::bind_method(D_METHOD("set_cab_window_open", "open"), &Cabin3D::set_cab_window_open);
        ClassDB::bind_method(D_METHOD("get_cab_window_open"), &Cabin3D::get_cab_window_open);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "cab_window_open"), "set_cab_window_open", "get_cab_window_open");

        ClassDB::bind_method(D_METHOD("set_camera_bound_min", "min"), &Cabin3D::set_camera_bound_min);
        ClassDB::bind_method(D_METHOD("get_camera_bound_min"), &Cabin3D::get_camera_bound_min);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR3, "camera_bound_min"), "set_camera_bound_min", "get_camera_bound_min");
        ClassDB::bind_method(D_METHOD("set_camera_bound_max", "max"), &Cabin3D::set_camera_bound_max);
        ClassDB::bind_method(D_METHOD("get_camera_bound_max"), &Cabin3D::get_camera_bound_max);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR3, "camera_bound_max"), "set_camera_bound_max", "get_camera_bound_max");
        ClassDB::bind_method(D_METHOD("set_camera_bound_enabled", "enabled"), &Cabin3D::set_camera_bound_enabled);
        ClassDB::bind_method(D_METHOD("get_camera_bound_enabled"), &Cabin3D::get_camera_bound_enabled);
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "camera_bound_enabled"), "set_camera_bound_enabled",
                "get_camera_bound_enabled");
        ClassDB::bind_method(D_METHOD("set_driver_position", "position"), &Cabin3D::set_driver_position);
        ClassDB::bind_method(D_METHOD("get_driver_position"), &Cabin3D::get_driver_position);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "driver_position"), "set_driver_position", "get_driver_position");
        ClassDB::bind_method(D_METHOD("set_driver_view_angle", "angle"), &Cabin3D::set_driver_view_angle);
        ClassDB::bind_method(D_METHOD("get_driver_view_angle"), &Cabin3D::get_driver_view_angle);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR2, "driver_view_angle"), "set_driver_view_angle", "get_driver_view_angle");

        ADD_GROUP("Camera Shake", "");
        ClassDB::bind_method(D_METHOD("set_shake_spring_stiffness", "value"), &Cabin3D::set_shake_spring_stiffness);
        ClassDB::bind_method(D_METHOD("get_shake_spring_stiffness"), &Cabin3D::get_shake_spring_stiffness);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "shake_spring_stiffness"), "set_shake_spring_stiffness",
                "get_shake_spring_stiffness");
        ClassDB::bind_method(D_METHOD("set_shake_spring_damping", "value"), &Cabin3D::set_shake_spring_damping);
        ClassDB::bind_method(D_METHOD("get_shake_spring_damping"), &Cabin3D::get_shake_spring_damping);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "shake_spring_damping"), "set_shake_spring_damping",
                "get_shake_spring_damping");
        ClassDB::bind_method(D_METHOD("set_shake_jolt_scale", "value"), &Cabin3D::set_shake_jolt_scale);
        ClassDB::bind_method(D_METHOD("get_shake_jolt_scale"), &Cabin3D::get_shake_jolt_scale);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR3, "shake_jolt_scale"), "set_shake_jolt_scale", "get_shake_jolt_scale");
        ClassDB::bind_method(D_METHOD("set_shake_jolt_limit", "value"), &Cabin3D::set_shake_jolt_limit);
        ClassDB::bind_method(D_METHOD("get_shake_jolt_limit"), &Cabin3D::get_shake_jolt_limit);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "shake_jolt_limit"), "set_shake_jolt_limit", "get_shake_jolt_limit");
        ClassDB::bind_method(D_METHOD("set_shake_angle_scale", "value"), &Cabin3D::set_shake_angle_scale);
        ClassDB::bind_method(D_METHOD("get_shake_angle_scale"), &Cabin3D::get_shake_angle_scale);
        ADD_PROPERTY(
                PropertyInfo(Variant::VECTOR2, "shake_angle_scale"), "set_shake_angle_scale", "get_shake_angle_scale");
        ClassDB::bind_method(D_METHOD("set_engine_shake_scale", "value"), &Cabin3D::set_engine_shake_scale);
        ClassDB::bind_method(D_METHOD("get_engine_shake_scale"), &Cabin3D::get_engine_shake_scale);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "engine_shake_scale"), "set_engine_shake_scale", "get_engine_shake_scale");
        ClassDB::bind_method(D_METHOD("set_engine_shake_fade_in_rpm", "value"), &Cabin3D::set_engine_shake_fade_in_rpm);
        ClassDB::bind_method(D_METHOD("get_engine_shake_fade_in_rpm"), &Cabin3D::get_engine_shake_fade_in_rpm);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "engine_shake_fade_in_rpm"), "set_engine_shake_fade_in_rpm",
                "get_engine_shake_fade_in_rpm");
        ClassDB::bind_method(
                D_METHOD("set_engine_shake_fade_in_factor", "value"), &Cabin3D::set_engine_shake_fade_in_factor);
        ClassDB::bind_method(D_METHOD("get_engine_shake_fade_in_factor"), &Cabin3D::get_engine_shake_fade_in_factor);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "engine_shake_fade_in_factor"), "set_engine_shake_fade_in_factor",
                "get_engine_shake_fade_in_factor");
        ClassDB::bind_method(
                D_METHOD("set_engine_shake_fade_out_rpm", "value"), &Cabin3D::set_engine_shake_fade_out_rpm);
        ClassDB::bind_method(D_METHOD("get_engine_shake_fade_out_rpm"), &Cabin3D::get_engine_shake_fade_out_rpm);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "engine_shake_fade_out_rpm"), "set_engine_shake_fade_out_rpm",
                "get_engine_shake_fade_out_rpm");
        ClassDB::bind_method(
                D_METHOD("set_engine_shake_fade_out_factor", "value"), &Cabin3D::set_engine_shake_fade_out_factor);
        ClassDB::bind_method(D_METHOD("get_engine_shake_fade_out_factor"), &Cabin3D::get_engine_shake_fade_out_factor);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "engine_shake_fade_out_factor"), "set_engine_shake_fade_out_factor",
                "get_engine_shake_fade_out_factor");

        ADD_SIGNAL(MethodInfo(cabin_ready_signal));
        /* The cab now sits in a different vehicle - the vehicle of the cabin set_cabin() was
         * given. A subclass reacts to this rather than overriding set_cabin(): a typed call from
         * C++ reaches the native method, and a script's method of the same name would simply be
         * skipped. */
        ADD_SIGNAL(MethodInfo(vehicle_rid_changed_signal, PropertyInfo(Variant::RID, "vehicle_rid")));
        ADD_SIGNAL(MethodInfo(cabin_changed_signal, PropertyInfo(Variant::RID, "cabin")));
        /// Emitted after rebuilding the cabin's driver position and camera bounds.
        ADD_SIGNAL(MethodInfo(camera_configuration_changed_signal));
    }

    void Cabin3D::_notification(const int p_what) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE: {
                VehicleServer *server = VehicleServer::get_instance();
                ERR_FAIL_NULL(server);
                server->connect(
                        VehicleServer::vehicle_controller_changed_signal,
                        callable_mp(this, &Cabin3D::_on_vehicle_changed));
                server->connect(
                        VehicleServer::vehicle_configured_signal, callable_mp(this, &Cabin3D::_on_vehicle_changed));
                // the vehicle may have got its controller while the cab was out of the tree
                _refresh_shake();
            } break;
            case NOTIFICATION_EXIT_TREE: {
                VehicleServer *server = VehicleServer::get_instance();
                ERR_FAIL_NULL(server);
                server->disconnect(
                        VehicleServer::vehicle_controller_changed_signal,
                        callable_mp(this, &Cabin3D::_on_vehicle_changed));
                server->disconnect(
                        VehicleServer::vehicle_configured_signal, callable_mp(this, &Cabin3D::_on_vehicle_changed));
            } break;
            case NOTIFICATION_READY:
                cabin_ready = true;
                emit_signal(cabin_ready_signal);
                break;
            case NOTIFICATION_PROCESS: {
                shake_accumulator += get_process_delta_time();
                while (shake_accumulator >= SHAKE_STEP) {
                    _process_engine_shake(SHAKE_STEP);
                    shake_accumulator -= SHAKE_STEP;
                }
            } break;
            default:;
        }
    }

    /* In the original only a diesel shakes the cab, so the engine's own kind answers the question -
     * no configuration is looked up for it. Asked of the server by the vehicle's handle each time:
     * the cab holds neither the engine nor the controller. */
    static Ref<RailVehicleDieselEngine> shaking_engine(const VehicleServer *p_server, const RID &p_vehicle) {
        return p_server != nullptr && p_vehicle.is_valid()
                       ? p_server->vehicle_component_get(p_vehicle, VehicleComponentType::COMPONENT_ENGINE)
                       : Ref<VehicleComponent>();
    }

    /* A cab whose vehicle has no diesel stands at rest and does not process */
    void Cabin3D::_refresh_shake() {
        const bool shaking = shaking_engine(VehicleServer::get_instance(), vehicle_rid).is_valid();
        if (!shaking) {
            shake_velocity = Vector3();
            shake_offset = Vector3();
            shake_accumulator = 0.0;
        }
        set_process(shaking);
    }

    void Cabin3D::_on_vehicle_changed(const RID &p_vehicle) {
        if (p_vehicle == vehicle_rid) {
            _refresh_shake();
        }
    }

    /* TSpring::ComputateForces (world/Spring.cpp) - the force pulling the cab from its shake offset
     * towards p_position */
    Vector3 Cabin3D::_compute_spring_force(const Vector3 &p_position) const {
        const Vector3 spring_delta = p_position - shake_offset;
        const double distance = spring_delta.length();
        if (distance <= SPRING_REST_LENGTH) {
            return {};
        }
        double force = (distance - SPRING_REST_LENGTH) * shake_spring_stiffness;
        force += distance * shake_spring_damping;
        return spring_delta / static_cast<real_t>(distance) * static_cast<real_t>(-force);
    }

    void Cabin3D::_process_engine_shake(const double p_delta) {
        const VehicleServer *server = VehicleServer::get_instance();
        const Ref<RailVehicleDieselEngine> engine = shaking_engine(server, vehicle_rid);
        if (engine.is_null()) {
            _refresh_shake();
            return;
        }
        Vector3 shake_vector;
        const double engine_revolutions = Math::abs(engine->get_rpm_count());
        if (engine_revolutions > 0.0) {
            engine_angle = Math::fmod(engine_angle + (engine_revolutions * p_delta), Math::TAU);
            const double fade_in =
                    CLAMP((engine_revolutions - (engine_shake_fade_in_rpm / LibMaszynaUnits::SECONDS_PER_MINUTE)) *
                                  engine_shake_fade_in_factor,
                          0.0, 1.0);
            const double fade_out =
                    1.0 -
                    CLAMP((engine_revolutions - (engine_shake_fade_out_rpm / LibMaszynaUnits::SECONDS_PER_MINUTE)) *
                                  engine_shake_fade_out_factor,
                          0.0, 1.0);
            shake_vector.x = static_cast<real_t>(
                    Math::sin(engine_angle * ENGINE_SHAKE_ANGLE_MULTIPLIER) * p_delta * engine_shake_scale * fade_in *
                    fade_out);
        }

        Vector3 shake = _compute_spring_force(shake_vector) * SHAKE_FORCE_GAIN;
        // the extra shake at increased velocity (DynObj.cpp:8102, 8113-8123)
        const double velocity = MIN(Math::abs(server->vehicle_get_speed(vehicle_rid)), SHAKE_MAX_VELOCITY);
        if (UtilityFunctions::randf_range(0.0, velocity) > SHAKE_JOLT_MIN_VELOCITY) {
            const double jolt_range = velocity * 2.0;
            const auto jolt = [velocity, jolt_range]() {
                return static_cast<real_t>(
                        (UtilityFunctions::randf_range(0.0, jolt_range) - velocity) /
                        (jolt_range * SHAKE_JOLT_DIVISOR));
            };
            shake += _compute_spring_force(
                    Vector3(jolt(), jolt(), jolt()) * shake_jolt_scale * static_cast<real_t>(SHAKE_FORCE_GAIN));
        }
        shake *= static_cast<real_t>(SHAKE_FORCE_ATTENUATION);
        const double damping =
                (shake_jolt_scale.x + shake_jolt_scale.y + shake_jolt_scale.z) / SHAKE_JOLT_SCALE_DIVISOR;
        shake_velocity -= (shake + shake_velocity * SHAKE_VELOCITY_DAMPING) * static_cast<real_t>(damping);
        shake_offset += shake_velocity * static_cast<real_t>(p_delta);
        if (Math::abs(shake_offset.y) > Math::abs(shake_jolt_limit)) {
            shake_velocity.y = -shake_velocity.y;
        }
    }

    /* The cab's elements are GDScript nodes (cabin_button.gd, legacy cabin behaviours, ...), whose
     * classes this C++ node cannot know at build time - hence the call by name. */
    void Cabin3D::_propagate_vehicle_rid(Node *p_node) const {
        for (int index = 0; index < p_node->get_child_count(); ++index) {
            Node *child = p_node->get_child(index);
            _propagate_vehicle_rid(child);
            if (child->has_method("set_vehicle_rid")) {
                child->call("set_vehicle_rid", vehicle_rid);
            }
        }
    }

    RID Cabin3D::get_vehicle_rid() const {
        return vehicle_rid;
    }

    Transform3D Cabin3D::get_camera_transform() const {
        return get_global_transform().translated_local(driver_position);
    }

    Vector3 Cabin3D::get_camera_shake_offset() const {
        return shake_offset;
    }

    double Cabin3D::get_camera_shake_roll() const {
        return Math::atan(shake_velocity.x * shake_angle_scale.x);
    }

    bool Cabin3D::is_cabin_ready() const {
        return cabin_ready;
    }

    int Cabin3D::get_sound_listener_context() const {
        if (cab_window_open) {
            return SOUND_LISTENER_OPEN_WINDOW;
        }
        const RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        switch (rail_vehicles != nullptr ? rail_vehicles->cabin_get_kind(cabin)
                                         : RailVehicleCabinKind::RAIL_VEHICLE_CABIN_NONE) {
            case RailVehicleCabinKind::RAIL_VEHICLE_CABIN_REAR:
                return SOUND_LISTENER_REAR_CAB;
            case RailVehicleCabinKind::RAIL_VEHICLE_CABIN_FRONT:
                return SOUND_LISTENER_FRONT_CAB;
            default:
                return SOUND_LISTENER_MACHINE_ROOM;
        }
    }

    void Cabin3D::set_cabin(const RID &p_cabin) {
        if (cabin == p_cabin) {
            return;
        }
        cabin = p_cabin;
        const VehicleServer *server = VehicleServer::get_instance();
        const RID vehicle = server != nullptr && cabin.is_valid() ? server->cabin_get_vehicle(cabin) : RID();
        if (vehicle != vehicle_rid) {
            vehicle_rid = vehicle;
            _refresh_shake();
            _propagate_vehicle_rid(this);
            emit_signal(vehicle_rid_changed_signal, vehicle_rid);
        }
        emit_signal(cabin_changed_signal, cabin);
    }

    RID Cabin3D::get_cabin() const {
        return cabin;
    }
    void Cabin3D::set_has_cab_model(const bool p_has_cab_model) {
        has_cab_model = p_has_cab_model;
    }
    bool Cabin3D::get_has_cab_model() const {
        return has_cab_model;
    }
    void Cabin3D::set_cab_window_open(const bool p_open) {
        cab_window_open = p_open;
    }
    bool Cabin3D::get_cab_window_open() const {
        return cab_window_open;
    }
    void Cabin3D::set_camera_bound_min(const Vector3 &p_min) {
        camera_bound_min = p_min;
    }
    Vector3 Cabin3D::get_camera_bound_min() const {
        return camera_bound_min;
    }
    void Cabin3D::set_camera_bound_max(const Vector3 &p_max) {
        camera_bound_max = p_max;
    }
    Vector3 Cabin3D::get_camera_bound_max() const {
        return camera_bound_max;
    }
    void Cabin3D::set_camera_bound_enabled(const bool p_enabled) {
        camera_bound_enabled = p_enabled;
    }
    bool Cabin3D::get_camera_bound_enabled() const {
        return camera_bound_enabled;
    }
    void Cabin3D::set_driver_position(const Vector3 &p_position) {
        driver_position = p_position;
    }
    Vector3 Cabin3D::get_driver_position() const {
        return driver_position;
    }
    void Cabin3D::set_driver_view_angle(const Vector2 &p_angle) {
        driver_view_angle = p_angle;
    }
    Vector2 Cabin3D::get_driver_view_angle() const {
        return driver_view_angle;
    }
    void Cabin3D::set_shake_spring_stiffness(const double p_value) {
        shake_spring_stiffness = p_value;
    }
    double Cabin3D::get_shake_spring_stiffness() const {
        return shake_spring_stiffness;
    }
    void Cabin3D::set_shake_spring_damping(const double p_value) {
        shake_spring_damping = p_value;
    }
    double Cabin3D::get_shake_spring_damping() const {
        return shake_spring_damping;
    }
    void Cabin3D::set_shake_jolt_scale(const Vector3 &p_value) {
        shake_jolt_scale = p_value;
    }
    Vector3 Cabin3D::get_shake_jolt_scale() const {
        return shake_jolt_scale;
    }
    void Cabin3D::set_shake_jolt_limit(const double p_value) {
        shake_jolt_limit = p_value;
    }
    double Cabin3D::get_shake_jolt_limit() const {
        return shake_jolt_limit;
    }
    void Cabin3D::set_shake_angle_scale(const Vector2 &p_value) {
        shake_angle_scale = p_value;
    }
    Vector2 Cabin3D::get_shake_angle_scale() const {
        return shake_angle_scale;
    }
    void Cabin3D::set_engine_shake_scale(const double p_value) {
        engine_shake_scale = p_value;
    }
    double Cabin3D::get_engine_shake_scale() const {
        return engine_shake_scale;
    }
    void Cabin3D::set_engine_shake_fade_in_rpm(const double p_value) {
        engine_shake_fade_in_rpm = p_value;
    }
    double Cabin3D::get_engine_shake_fade_in_rpm() const {
        return engine_shake_fade_in_rpm;
    }
    void Cabin3D::set_engine_shake_fade_in_factor(const double p_value) {
        engine_shake_fade_in_factor = p_value;
    }
    double Cabin3D::get_engine_shake_fade_in_factor() const {
        return engine_shake_fade_in_factor;
    }
    void Cabin3D::set_engine_shake_fade_out_rpm(const double p_value) {
        engine_shake_fade_out_rpm = p_value;
    }
    double Cabin3D::get_engine_shake_fade_out_rpm() const {
        return engine_shake_fade_out_rpm;
    }
    void Cabin3D::set_engine_shake_fade_out_factor(const double p_value) {
        engine_shake_fade_out_factor = p_value;
    }
    double Cabin3D::get_engine_shake_fade_out_factor() const {
        return engine_shake_fade_out_factor;
    }
} // namespace godot
