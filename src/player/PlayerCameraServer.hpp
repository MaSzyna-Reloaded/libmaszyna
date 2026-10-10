#pragma once
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /// The player's view - the one owner of where the player looks from: the cab of the vehicle the
    /// player drives (PlayerServer), the free camera (walking) or the camera following a vehicle,
    /// which vehicle and in which view. Everything that changes the view - the keys (F4, Shift+F4,
    /// drivermode.cpp:1244-1277), the HUD, scripts - calls these operations by the vehicles'
    /// RailVehicleServer handles. The server holds the view and announces it; the player's cameras
    /// are the player's, which follows these signals.
    class PlayerCameraServer : public Object {
            GDCLASS(PlayerCameraServer, Object)

        public:
            enum CameraMode {
                /// From the cab of the vehicle the player drives
                CAMERA_MODE_CABIN,
                /// The free camera - the player walking
                CAMERA_MODE_FREE,
                /// The following camera, following the target
                CAMERA_MODE_FOLLOW,
            };
            /// The views of the following camera (ExternalCamera3D.View), in the order Shift+F4
            /// steps through them
            enum CameraFollowView {
                CAMERA_FOLLOW_VIEW_TRAINSET_FRONT,
                CAMERA_FOLLOW_VIEW_TRAINSET_REAR,
                CAMERA_FOLLOW_VIEW_BOGIE,
                CAMERA_FOLLOW_VIEW_DRIVEBY,
                CAMERA_FOLLOW_VIEW_MAX,
            };

            /// The mode, the target or the follow view changed
            static const char *camera_changed_signal;
            /// The free camera is to stand at the transform (transform: Transform3D)
            static const char *camera_placed_signal;

            static PlayerCameraServer *get_instance() {
                return Object::cast_to<PlayerCameraServer>(
                        Engine::get_singleton()->get_singleton("PlayerCameraServer"));
            }

        private:
            CameraMode mode = CAMERA_MODE_FREE;
            RID target;
            CameraFollowView follow_view = CAMERA_FOLLOW_VIEW_TRAINSET_FRONT;

            void _on_vehicle_freed(const RID &p_vehicle);
            /// Let go while looked from its cab, the free camera
            void _on_player_vehicle_changed(const RID &p_vehicle, const RID &p_previous);
            /// Taken over - the vehicle driven too - the view is from its cab
            void _on_player_vehicle_entered(const RID &p_vehicle);

        protected:
            static void _bind_methods();

        public:
            PlayerCameraServer();

            /// The view from the cab (a vehicle driven), the free camera, or the following camera (a
            /// target set)
            void camera_set_mode(CameraMode p_mode);
            CameraMode camera_get_mode() const;
            /// The vehicle the following camera follows; followed now, it follows this one
            void camera_set_target(const RID &p_vehicle);
            RID camera_get_target() const;
            void camera_set_follow_view(CameraFollowView p_view);
            CameraFollowView camera_get_follow_view() const;
            /// Shift+F4: following, the next view; else the player's vehicle followed
            void camera_cycle_follow_view();
            /// F4: outside - following or walking - back into the cab of the vehicle driven (following
            /// with none, the free camera); in the cab, out of it
            void camera_toggle_cabin();
            /// The free camera beside the vehicle, looking at it
            void camera_show_vehicle(const RID &p_vehicle);
            /// Where camera_show_vehicle() puts the camera: beside the vehicle, looking at it
            Transform3D camera_get_show_transform(const RID &p_vehicle) const;
    };
} // namespace godot

VARIANT_ENUM_CAST(PlayerCameraServer::CameraMode)
VARIANT_ENUM_CAST(PlayerCameraServer::CameraFollowView)
