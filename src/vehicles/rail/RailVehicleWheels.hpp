#pragma once

#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    class RailVehicleWheels : public RailVehicleComponent {
            GDCLASS(RailVehicleWheels, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_WHEELS;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual double get_angle_front_deg() const = 0;
            virtual double get_angle_powered_deg() const = 0;
            virtual double get_angle_rear_deg() const = 0;
            virtual double get_rotation_speed_rps() const = 0;
            virtual double get_rotation_acceleration_rps2() const = 0;
            virtual bool get_slipping() const = 0;
            virtual double get_flat() const = 0;
            /// Which of the vehicle's two bogies a placement is asked for.
            enum Bogie {
                BOGIE_FRONT = 0,
                BOGIE_REAR = 1,
            };
            /// Where a bogie sits on the track: the wheels own the running gear, so they own its
            /// geometry. Sampled from the vehicle's placement at half the pivot spacing to each
            /// side; a vehicle whose pivot spacing is unknown yet has both bogies at the same
            /// point, which is what the caller sees.
            Transform3D get_bogie_transform(Bogie p_bogie) const;

            enum BearingType {
                BEARING_TYPE_SLIDE = 0,
                BEARING_TYPE_ROLL = 1,
            };
            MAKE_MEMBER_GS(double, powered_wheel_diameter, 0.0);
            MAKE_MEMBER_GS(double, front_rolling_wheel_diameter, 0.0);
            MAKE_MEMBER_GS(double, rear_rolling_wheel_diameter, 0.0);
            MAKE_MEMBER_GS(double, axle_inertial_moment, 0.0);
            MAKE_MEMBER_GS(double, track_width, 1.435);
            MAKE_MEMBER_GS(String, axle_arrangement, "");
            MAKE_MEMBER_GS(double, bogie_axle_spacing, 0.0);
            MAKE_MEMBER_GS(double, bogie_pivot_spacing, 0.0);
            MAKE_MEMBER_GS(double, minimum_curve_radius, 0.0);
            MAKE_MEMBER_GS(int, bearing_type, BEARING_TYPE_ROLL);
    };
} // namespace godot

VARIANT_ENUM_CAST(godot::RailVehicleWheels::BearingType);
VARIANT_ENUM_CAST(godot::RailVehicleWheels::Bogie);
