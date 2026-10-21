#include "PlayerCameraServer.hpp"
#include "PlayerServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    const char *PlayerCameraServer::camera_changed_signal = "camera_changed";
    const char *PlayerCameraServer::camera_placed_signal = "camera_placed";

    namespace {
        /// camera_show_vehicle(): the camera stands this far to the side per metre of the vehicle's
        /// length, never nearer than the minimum [m], at eye height above the vehicle's origin [m]
        constexpr double SHOW_DISTANCE_PER_LENGTH = 1.2;
        constexpr double SHOW_MIN_DISTANCE = 12.0;
        constexpr double SHOW_EYE_HEIGHT = 1.75;
    } // namespace

    void PlayerCameraServer::_bind_methods() {
        ClassDB::bind_method(D_METHOD("camera_set_mode", "mode"), &PlayerCameraServer::camera_set_mode);
        ClassDB::bind_method(D_METHOD("camera_get_mode"), &PlayerCameraServer::camera_get_mode);
        ClassDB::bind_method(D_METHOD("camera_set_target", "vehicle"), &PlayerCameraServer::camera_set_target);
        ClassDB::bind_method(D_METHOD("camera_get_target"), &PlayerCameraServer::camera_get_target);
        ClassDB::bind_method(D_METHOD("camera_set_follow_view", "view"), &PlayerCameraServer::camera_set_follow_view);
        ClassDB::bind_method(D_METHOD("camera_get_follow_view"), &PlayerCameraServer::camera_get_follow_view);
        ClassDB::bind_method(D_METHOD("camera_cycle_follow_view"), &PlayerCameraServer::camera_cycle_follow_view);
        ClassDB::bind_method(D_METHOD("camera_toggle_cabin"), &PlayerCameraServer::camera_toggle_cabin);
        ClassDB::bind_method(D_METHOD("camera_show_vehicle", "vehicle"), &PlayerCameraServer::camera_show_vehicle);
        ClassDB::bind_method(
                D_METHOD("camera_get_show_transform", "vehicle"), &PlayerCameraServer::camera_get_show_transform);

        BIND_ENUM_CONSTANT(CAMERA_MODE_CABIN);
        BIND_ENUM_CONSTANT(CAMERA_MODE_FREE);
        BIND_ENUM_CONSTANT(CAMERA_MODE_FOLLOW);
        BIND_ENUM_CONSTANT(CAMERA_FOLLOW_VIEW_TRAINSET_FRONT);
        BIND_ENUM_CONSTANT(CAMERA_FOLLOW_VIEW_TRAINSET_REAR);
        BIND_ENUM_CONSTANT(CAMERA_FOLLOW_VIEW_BOGIE);
        BIND_ENUM_CONSTANT(CAMERA_FOLLOW_VIEW_DRIVEBY);
        BIND_ENUM_CONSTANT(CAMERA_FOLLOW_VIEW_MAX);

        ADD_SIGNAL(MethodInfo(camera_changed_signal));
        ADD_SIGNAL(MethodInfo(camera_placed_signal, PropertyInfo(Variant::TRANSFORM3D, "transform")));
    }

    /// A freed vehicle is not followed any more, and the view follows what the player drives. No
    /// explicit disconnect: callable_mp reports this instance as the callable's object, so the
    /// engine drops the connection when it dies.
    PlayerCameraServer::PlayerCameraServer() {
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        vehicles->connect(
                VehicleServer::vehicle_freed_signal, callable_mp(this, &PlayerCameraServer::_on_vehicle_freed));
        PlayerServer *player = PlayerServer::get_instance();
        ERR_FAIL_NULL(player);
        player->connect(
                PlayerServer::player_vehicle_changed_signal,
                callable_mp(this, &PlayerCameraServer::_on_player_vehicle_changed));
        player->connect(
                PlayerServer::player_vehicle_entered_signal,
                callable_mp(this, &PlayerCameraServer::_on_player_vehicle_entered));
    }

    void PlayerCameraServer::_on_player_vehicle_changed(const RID &p_vehicle, const RID & /*p_previous*/) {
        if (!p_vehicle.is_valid() && mode == CAMERA_MODE_CABIN) {
            camera_set_mode(CAMERA_MODE_FREE);
        }
    }

    /// Taking a vehicle over puts the player in its cab (Train.cpp:9147), the one already driven
    /// too (drivermode.cpp:264)
    void PlayerCameraServer::_on_player_vehicle_entered(const RID & /*p_vehicle*/) {
        camera_set_mode(CAMERA_MODE_CABIN);
    }

    void PlayerCameraServer::_on_vehicle_freed(const RID &p_vehicle) {
        if (p_vehicle == target) {
            camera_set_target(RID());
        }
    }

    void PlayerCameraServer::camera_set_mode(const CameraMode p_mode) {
        if (p_mode == mode) {
            return;
        }
        const PlayerServer *player = PlayerServer::get_instance();
        ERR_FAIL_COND_MSG(
                p_mode == CAMERA_MODE_CABIN && (player == nullptr || !player->player_get_vehicle().is_valid()),
                "The player drives no vehicle");
        ERR_FAIL_COND_MSG(p_mode == CAMERA_MODE_FOLLOW && !target.is_valid(), "No vehicle to follow");
        mode = p_mode;
        emit_signal(camera_changed_signal);
    }

    PlayerCameraServer::CameraMode PlayerCameraServer::camera_get_mode() const {
        return mode;
    }

    void PlayerCameraServer::camera_set_target(const RID &p_vehicle) {
        if (p_vehicle == target) {
            return;
        }
        target = p_vehicle;
        if (mode == CAMERA_MODE_FOLLOW && !target.is_valid()) {
            camera_set_mode(CAMERA_MODE_FREE);
            return;
        }
        emit_signal(camera_changed_signal);
    }

    RID PlayerCameraServer::camera_get_target() const {
        return target;
    }

    void PlayerCameraServer::camera_set_follow_view(const CameraFollowView p_view) {
        ERR_FAIL_INDEX(p_view, CAMERA_FOLLOW_VIEW_MAX);
        if (p_view == follow_view) {
            return;
        }
        follow_view = p_view;
        emit_signal(camera_changed_signal);
    }

    PlayerCameraServer::CameraFollowView PlayerCameraServer::camera_get_follow_view() const {
        return follow_view;
    }

    void PlayerCameraServer::camera_cycle_follow_view() {
        if (mode == CAMERA_MODE_FOLLOW) {
            camera_set_follow_view(static_cast<CameraFollowView>((follow_view + 1) % CAMERA_FOLLOW_VIEW_MAX));
            return;
        }
        const PlayerServer *player = PlayerServer::get_instance();
        if (player == nullptr || !player->player_get_vehicle().is_valid()) {
            return;
        }
        camera_set_target(player->player_get_vehicle());
        camera_set_mode(CAMERA_MODE_FOLLOW);
    }

    void PlayerCameraServer::camera_toggle_cabin() {
        const PlayerServer *player = PlayerServer::get_instance();
        const bool driving = player != nullptr && player->player_get_vehicle().is_valid();
        switch (mode) {
            case CAMERA_MODE_FOLLOW:
                camera_set_mode(driving ? CAMERA_MODE_CABIN : CAMERA_MODE_FREE);
                break;
            case CAMERA_MODE_FREE:
                if (driving) {
                    camera_set_mode(CAMERA_MODE_CABIN);
                }
                break;
            case CAMERA_MODE_CABIN:
                camera_set_mode(CAMERA_MODE_FREE);
                break;
        }
    }

    Transform3D PlayerCameraServer::camera_get_show_transform(const RID &p_vehicle) const {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, Transform3D());
        ERR_FAIL_NULL_V(rail_vehicles, Transform3D());
        ERR_FAIL_COND_V(!vehicles->vehicle_exists(p_vehicle), Transform3D());
        const Transform3D body = rail_vehicles->vehicle_get_transform(p_vehicle);
        const double length = vehicles->vehicle_dump_config(p_vehicle).get("length", 0.0);
        const Vector3 eye(0.0, SHOW_EYE_HEIGHT, 0.0);
        const Vector3 position =
                body.origin +
                body.basis.get_column(0).normalized() *
                        static_cast<real_t>(MAX(SHOW_MIN_DISTANCE, length * SHOW_DISTANCE_PER_LENGTH)) +
                eye;
        return Transform3D(Basis(), position).looking_at(body.origin + eye);
    }

    void PlayerCameraServer::camera_show_vehicle(const RID &p_vehicle) {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(vehicles);
        ERR_FAIL_COND(!vehicles->vehicle_exists(p_vehicle));
        // placed first: the free camera is where it is to be before the view is announced as free
        emit_signal(camera_placed_signal, camera_get_show_transform(p_vehicle));
        camera_set_mode(CAMERA_MODE_FREE);
    }
} // namespace godot
