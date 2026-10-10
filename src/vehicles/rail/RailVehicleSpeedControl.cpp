#include "RailVehicleSpeedControl.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void RailVehicleSpeedControl::_bind_methods() {
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::BOOL, speed_control_enabled);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, delay);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::BOOL, impulse_lever);
        BIND_PROPERTY_W_HINT(
                RailVehicleSpeedControl, Variant::INT, disables_on, PROPERTY_HINT_FLAGS,
                "Main Controller Movement,Braking");
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::PACKED_FLOAT64_ARRAY, preset_speeds);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::BOOL, override_manual_power);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, initial_power);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, full_power_velocity);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, start_velocity);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, velocity_step);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, power_step);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, min_power);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, max_power);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, min_velocity);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, max_velocity);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, offset);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, proportional_gain_positive);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, proportional_gain_negative);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, integral_gain_positive);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, integral_gain_negative);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::BOOL, brake_intervention);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, brake_intervention_max_velocity);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, power_up_speed);
        BIND_PROPERTY(RailVehicleSpeedControl, Variant::FLOAT, power_down_speed);

        ClassDB::bind_method(D_METHOD("get_active"), &RailVehicleSpeedControl::get_active);
        ClassDB::bind_method(D_METHOD("get_desired_velocity"), &RailVehicleSpeedControl::get_desired_velocity);
        ClassDB::bind_method(D_METHOD("get_desired_power"), &RailVehicleSpeedControl::get_desired_power);
        ClassDB::bind_method(D_METHOD("speed_control_increase"), &RailVehicleSpeedControl::speed_control_increase);
        ClassDB::bind_method(D_METHOD("speed_control_decrease"), &RailVehicleSpeedControl::speed_control_decrease);
        ClassDB::bind_method(
                D_METHOD("speed_control_power_increase"), &RailVehicleSpeedControl::speed_control_power_increase);
        ClassDB::bind_method(
                D_METHOD("speed_control_power_decrease"), &RailVehicleSpeedControl::speed_control_power_decrease);
        ClassDB::bind_method(
                D_METHOD("speed_control_button", "button"), &RailVehicleSpeedControl::speed_control_button);
        ClassDB::bind_method(D_METHOD("speed_control_set", "velocity"), &RailVehicleSpeedControl::speed_control_set);
        ClassDB::bind_method(D_METHOD("get_set_velocity"), &RailVehicleSpeedControl::get_set_velocity);
        ClassDB::bind_method(D_METHOD("get_standby"), &RailVehicleSpeedControl::get_standby);
        ClassDB::bind_method(D_METHOD("get_selected_velocity"), &RailVehicleSpeedControl::get_selected_velocity);
    }

    void RailVehicleSpeedControl::_register_commands() {
        register_command("speed_control_increase", Callable(this, "speed_control_increase"));
        register_command("speed_control_decrease", Callable(this, "speed_control_decrease"));
        register_command("speed_control_power_increase", Callable(this, "speed_control_power_increase"));
        register_command("speed_control_power_decrease", Callable(this, "speed_control_power_decrease"));
        register_command("speed_control_button", Callable(this, "speed_control_button"));
        register_command("speed_control_set", Callable(this, "speed_control_set"));
    }

    void RailVehicleSpeedControl::_unregister_commands() {
        unregister_command("speed_control_increase");
        unregister_command("speed_control_decrease");
        unregister_command("speed_control_power_increase");
        unregister_command("speed_control_power_decrease");
        unregister_command("speed_control_button");
        unregister_command("speed_control_set");
    }
} // namespace godot
