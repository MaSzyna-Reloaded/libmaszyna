#pragma once

#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"

namespace godot {
    class RailVehicleWheels : public RailVehicleComponent {
            GDCLASS(RailVehicleWheels, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_WHEELS;
            }

        private:
            static void _bind_methods();
            /* The diameter TMoverParameters keeps when the data gives none (MOVER.h:1595) */
            static constexpr double DEFAULT_WHEEL_DIAMETER = 1.0;
            /* The wheel turns - the vehicle's, not the simulation's (TDynamicObject::Update,
             * DynObj.cpp:3776-3792) [deg] */
            double wheel_angle_front_deg = 0.0;
            double wheel_angle_powered_deg = 0.0;
            double wheel_angle_rear_deg = 0.0;

        protected:
            /* The wheels turned on by a step, from the vehicle's velocity and the powered axles'
             * revolutions */
            void _do_process_component(double p_delta) override;
            /* The diameters the data gives, the powered wheels' for a rolling one it leaves out
             * (LoadFIZ_Wheels) [m] */
            double _get_resolved_powered_wheel_diameter() const;
            double _get_resolved_front_rolling_wheel_diameter() const;
            double _get_resolved_rear_rolling_wheel_diameter() const;

        public:
            double get_angle_front_deg() const;
            double get_angle_powered_deg() const;
            double get_angle_rear_deg() const;
            /* Live state, read straight from the backend - nothing is stored. */
            virtual double get_rotation_speed_rps() const = 0;
            virtual double get_rotation_acceleration_rps2() const = 0;
            virtual bool get_slipping() const = 0;
            virtual double get_flat() const = 0;
            /// Which of the vehicle's two bogies (RailVehicleServer::vehicle_get_bogie_transform()).
            enum Bogie {
                BOGIE_FRONT = 0,
                BOGIE_REAR = 1,
            };

            enum BearingType {
                BEARING_TYPE_SLIDE = 0,
                BEARING_TYPE_ROLL = 1,
            };

        private:
            double powered_wheel_diameter = 0.0;
            double front_rolling_wheel_diameter = 0.0;
            double rear_rolling_wheel_diameter = 0.0;
            double axle_inertial_moment = 0.0;
            double track_width = 1.435;
            String axle_arrangement = "";
            double bogie_axle_spacing = 0.0;
            double bogie_pivot_spacing = 0.0;
            double minimum_curve_radius = 0.0;
            int bearing_type = BEARING_TYPE_ROLL;

        public:
            MAKE_GS(double, powered_wheel_diameter);
            MAKE_GS(double, front_rolling_wheel_diameter);
            MAKE_GS(double, rear_rolling_wheel_diameter);
            MAKE_GS(double, axle_inertial_moment);
            MAKE_GS(double, track_width);
            MAKE_GS(String, axle_arrangement);
            MAKE_GS(double, bogie_axle_spacing);
            MAKE_GS(double, bogie_pivot_spacing);
            MAKE_GS(double, minimum_curve_radius);
            MAKE_GS(int, bearing_type);
    };
} // namespace godot

VARIANT_ENUM_CAST(godot::RailVehicleWheels::BearingType);
VARIANT_ENUM_CAST(godot::RailVehicleWheels::Bogie);
