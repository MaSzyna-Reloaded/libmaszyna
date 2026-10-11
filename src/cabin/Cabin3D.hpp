#pragma once
#include <godot_cpp/classes/node3d.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/vector2.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /* The root of a driver's cab.
     *
     * It knows one thing about the vehicle it sits in: its name. Every read and every
     * manipulation below it goes through CabinSystem, and the elements hold no path to a
     * controller at all - the name is handed down to them from here.
     *
     * Native because of the shake: a fixed-step spring integration running every frame in every
     * occupied cab is exactly the work `CODE_STYLE.md` says does not belong in GDScript. */
    class Cabin3D : public Node3D {
            GDCLASS(Cabin3D, Node3D)

        public:
            static const char *cabin_ready_signal;
            static const char *camera_configuration_changed_signal;
            static const char *vehicle_rid_changed_signal;
            /* The interior is of another cabin of its vehicle now (cabin: RID) */
            static const char *cabin_changed_signal;

        private:
            static void _bind_methods();

            /* Original engine: the shake is integrated at a fixed step so it does not depend on
             * the frame rate; the accumulator carries the remainder between frames
             * (drivermode.h:113 m_secondaryupdaterate) */
            static constexpr double SHAKE_STEP = 1.0 / 50.0;
            /* Spring.h:33 restLen */
            static constexpr double SPRING_REST_LENGTH = 0.01;
            /* TDynamicObject::update_shake (DynObj.cpp:8048-8136) */
            static constexpr double ENGINE_SHAKE_ANGLE_MULTIPLIER = 4.0; // DynObj.cpp:8069
            static constexpr double SHAKE_FORCE_GAIN = 1.25;             // DynObj.cpp:8111
            static constexpr double SHAKE_FORCE_ATTENUATION = 0.85;      // DynObj.cpp:8124
            static constexpr double SHAKE_VELOCITY_DAMPING = 100.0;      // DynObj.cpp:8126
            static constexpr double SHAKE_JOLT_SCALE_DIVISOR = 200.0;    // DynObj.cpp:8126
            static constexpr double SHAKE_MAX_VELOCITY = 150.0;          // DynObj.cpp:8102 [km/h]
            static constexpr double SHAKE_JOLT_MIN_VELOCITY = 25.0;      // DynObj.cpp:8113 [km/h]
            static constexpr double SHAKE_JOLT_DIVISOR = 4.0;            // DynObj.cpp:8117

            /// The vehicle of `cabin` (VehicleServer::cabin_get_vehicle()), taken when the cabin is set
            RID vehicle_rid;
            bool cabin_ready = false;
            double engine_angle = Math::PI * 0.5; // MOVER.h:2207 eAngle
            Vector3 shake_velocity;
            Vector3 shake_offset;
            double shake_accumulator = 0.0;

            RID cabin;
            bool has_cab_model = true;
            bool cab_window_open = false;
            Vector3 camera_bound_min;
            Vector3 camera_bound_max;
            bool camera_bound_enabled = false;
            Vector3 driver_position;
            /* The cab camera's start: yaw, then pitch, in degrees, from the way the cab faces
             * (driverNangle:, Train.cpp:10529-10538; drivermode.cpp:1225-1226) */
            Vector2 driver_view_angle;

            double shake_spring_stiffness = 125.0; // DynObj.cpp:2340 ShakeSpring.Init()
            double shake_spring_damping = 0.002;   // Spring.h:29 nKd
            /* BaseShake and EngineShake defaults (DynObj.h:837-846) */
            Vector3 shake_jolt_scale = Vector3(0.2, 0.2, 0.1);
            double shake_jolt_limit = 2.0;
            Vector2 shake_angle_scale = Vector2(0.05, 0.1);
            double engine_shake_scale = 2.0;
            double engine_shake_fade_in_rpm = 90.0;
            double engine_shake_fade_in_factor = 0.3;
            double engine_shake_fade_out_rpm = 600.0;
            double engine_shake_fade_out_factor = 0.5;

            Vector3 _compute_spring_force(const Vector3 &p_position) const;
            void _process_engine_shake(double p_delta);
            void _refresh_shake();
            void _on_vehicle_changed(const RID &p_vehicle);
            /* The cab's elements are GDScript nodes this class only hosts, so the name is handed
             * down by a named call - the one case `CODE_STYLE.md` allows. */
            void _propagate_vehicle_rid(Node *p_node) const;

        public:
            /* Notifications, not the _ready()/_process() virtuals: a GDScript subclass that
             * defines _ready() *replaces* the native virtual, and MaszynaDynamicTrainCabin does - the
             * cab would then never announce itself and the camera would never enter it. A
             * notification reaches the native class and the script alike. */
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);

            /// The vehicle this cab sits in, by name - the vehicle of its cabin (set_cabin()). There
            /// is deliberately no path to a controller here.
            RID get_vehicle_rid() const;

            Transform3D get_camera_transform() const;
            Vector3 get_camera_shake_offset() const;
            double get_camera_shake_roll() const;
            bool is_cabin_ready() const;
            /* Original engine: the soundproofing column is CabOccupied + 1 (sound.cpp:981-1008) -
             * cab2 = 0, machine room = 1, cab1 = 2 - and an open window is 3. */
            int get_sound_listener_context() const;

            /* The VehicleServer cabin this interior is of - set by whoever builds it; its vehicle
             * comes with it */
            void set_cabin(const RID &p_cabin);
            RID get_cabin() const;
            void set_has_cab_model(bool p_has_cab_model);
            bool get_has_cab_model() const;
            void set_cab_window_open(bool p_open);
            bool get_cab_window_open() const;
            void set_camera_bound_min(const Vector3 &p_min);
            Vector3 get_camera_bound_min() const;
            void set_camera_bound_max(const Vector3 &p_max);
            Vector3 get_camera_bound_max() const;
            void set_camera_bound_enabled(bool p_enabled);
            bool get_camera_bound_enabled() const;
            void set_driver_position(const Vector3 &p_position);
            Vector3 get_driver_position() const;
            void set_driver_view_angle(const Vector2 &p_angle);
            Vector2 get_driver_view_angle() const;

            void set_shake_spring_stiffness(double p_value);
            double get_shake_spring_stiffness() const;
            void set_shake_spring_damping(double p_value);
            double get_shake_spring_damping() const;
            void set_shake_jolt_scale(const Vector3 &p_value);
            Vector3 get_shake_jolt_scale() const;
            void set_shake_jolt_limit(double p_value);
            double get_shake_jolt_limit() const;
            void set_shake_angle_scale(const Vector2 &p_value);
            Vector2 get_shake_angle_scale() const;
            void set_engine_shake_scale(double p_value);
            double get_engine_shake_scale() const;
            void set_engine_shake_fade_in_rpm(double p_value);
            double get_engine_shake_fade_in_rpm() const;
            void set_engine_shake_fade_in_factor(double p_value);
            double get_engine_shake_fade_in_factor() const;
            void set_engine_shake_fade_out_rpm(double p_value);
            double get_engine_shake_fade_out_rpm() const;
            void set_engine_shake_fade_out_factor(double p_value);
            double get_engine_shake_fade_out_factor() const;
    };
} // namespace godot
