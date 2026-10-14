#include "RailVehicleWheels.hpp"
#include <godot_cpp/core/math.hpp>

namespace godot {
    void RailVehicleWheels::_bind_methods() {
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, powered_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, front_rolling_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, rear_rolling_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, axle_inertial_moment);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, track_width);
        BIND_PROPERTY(RailVehicleWheels, Variant::STRING, axle_arrangement);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_axle_spacing);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_pivot_spacing);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, minimum_curve_radius);
        BIND_PROPERTY_W_HINT(RailVehicleWheels, Variant::INT, bearing_type, PROPERTY_HINT_ENUM, "Slide,Roll");

        BIND_ENUM_CONSTANT(BOGIE_FRONT);
        BIND_ENUM_CONSTANT(BOGIE_REAR);
        BIND_ENUM_CONSTANT(BEARING_TYPE_SLIDE);
        BIND_ENUM_CONSTANT(BEARING_TYPE_ROLL);

        ClassDB::bind_method(D_METHOD("get_angle_front_deg"), &RailVehicleWheels::get_angle_front_deg);
        ClassDB::bind_method(D_METHOD("get_angle_powered_deg"), &RailVehicleWheels::get_angle_powered_deg);
        ClassDB::bind_method(D_METHOD("get_angle_rear_deg"), &RailVehicleWheels::get_angle_rear_deg);
        ClassDB::bind_method(D_METHOD("get_rotation_speed_rps"), &RailVehicleWheels::get_rotation_speed_rps);
        ClassDB::bind_method(
                D_METHOD("get_rotation_acceleration_rps2"), &RailVehicleWheels::get_rotation_acceleration_rps2);
        ClassDB::bind_method(D_METHOD("get_slipping"), &RailVehicleWheels::get_slipping);
        ClassDB::bind_method(D_METHOD("get_flat"), &RailVehicleWheels::get_flat);
    }

    double RailVehicleWheels::_get_resolved_powered_wheel_diameter() const {
        return get_powered_wheel_diameter() > 0.0 ? get_powered_wheel_diameter() : DEFAULT_WHEEL_DIAMETER;
    }

    double RailVehicleWheels::_get_resolved_front_rolling_wheel_diameter() const {
        return get_front_rolling_wheel_diameter() > 0.0 ? get_front_rolling_wheel_diameter()
                                                        : _get_resolved_powered_wheel_diameter();
    }

    double RailVehicleWheels::_get_resolved_rear_rolling_wheel_diameter() const {
        return get_rear_rolling_wheel_diameter() > 0.0 ? get_rear_rolling_wheel_diameter()
                                                       : _get_resolved_powered_wheel_diameter();
    }

    // DynObj.cpp:3776-3792 (TDynamicObject::Update, "krecenie kolami"): the rolling wheels turn
    // with the velocity, the powered ones with the motors' revolutions
    void RailVehicleWheels::_do_process_component(const double p_delta) {
        const VehicleController *controller = get_controller().ptr();
        ERR_FAIL_NULL(controller);
        if (controller->get_speed() == 0.0) {
            return;
        }
        const double velocity = controller->get_velocity();
        wheel_angle_front_deg += Math::rad_to_deg(velocity * p_delta / _get_resolved_front_rolling_wheel_diameter());
        wheel_angle_front_deg = Math::wrapf(wheel_angle_front_deg, 0.0, 360.0);
        wheel_angle_powered_deg += get_rotation_speed_rps() * p_delta * 360.0;
        wheel_angle_powered_deg = Math::wrapf(wheel_angle_powered_deg, 0.0, 360.0);
        wheel_angle_rear_deg += Math::rad_to_deg(velocity * p_delta / _get_resolved_rear_rolling_wheel_diameter());
        wheel_angle_rear_deg = Math::wrapf(wheel_angle_rear_deg, 0.0, 360.0);
    }

    double RailVehicleWheels::get_angle_front_deg() const {
        return wheel_angle_front_deg;
    }

    double RailVehicleWheels::get_angle_powered_deg() const {
        return wheel_angle_powered_deg;
    }

    double RailVehicleWheels::get_angle_rear_deg() const {
        return wheel_angle_rear_deg;
    }
} // namespace godot
