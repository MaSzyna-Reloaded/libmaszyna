#include "RailVehicleMasterController.hpp"

namespace godot {
    void RailVehicleMasterController::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_main_position_count", "value"), &RailVehicleMasterController::set_main_position_count);
        ClassDB::bind_method(
                D_METHOD("get_main_position_count"), &RailVehicleMasterController::get_main_position_count);
        ClassDB::bind_method(
                D_METHOD("set_second_position_count", "value"),
                &RailVehicleMasterController::set_second_position_count);
        ClassDB::bind_method(
                D_METHOD("get_second_position_count"), &RailVehicleMasterController::get_second_position_count);
        ClassDB::bind_method(
                D_METHOD("set_direction_change_max_position", "value"),
                &RailVehicleMasterController::set_direction_change_max_position);
        ClassDB::bind_method(
                D_METHOD("get_direction_change_max_position"),
                &RailVehicleMasterController::get_direction_change_max_position);
        ClassDB::bind_method(
                D_METHOD("set_coupled_controllers", "value"), &RailVehicleMasterController::set_coupled_controllers);
        ClassDB::bind_method(
                D_METHOD("get_coupled_controllers"), &RailVehicleMasterController::get_coupled_controllers);
        ClassDB::bind_method(D_METHOD("set_initial_delay", "value"), &RailVehicleMasterController::set_initial_delay);
        ClassDB::bind_method(D_METHOD("get_initial_delay"), &RailVehicleMasterController::get_initial_delay);
        ClassDB::bind_method(D_METHOD("set_step_delay", "value"), &RailVehicleMasterController::set_step_delay);
        ClassDB::bind_method(D_METHOD("get_step_delay"), &RailVehicleMasterController::get_step_delay);
        ClassDB::bind_method(
                D_METHOD("set_step_down_delay", "value"), &RailVehicleMasterController::set_step_down_delay);
        ClassDB::bind_method(D_METHOD("get_step_down_delay"), &RailVehicleMasterController::get_step_down_delay);
        ClassDB::bind_method(
                D_METHOD("set_tachometer_max_speed", "value"), &RailVehicleMasterController::set_tachometer_max_speed);
        ClassDB::bind_method(
                D_METHOD("get_tachometer_max_speed"), &RailVehicleMasterController::get_tachometer_max_speed);

        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "main_position_count"), "set_main_position_count",
                "get_main_position_count");
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "second_position_count"), "set_second_position_count",
                "get_second_position_count");
        ADD_PROPERTY(
                PropertyInfo(Variant::INT, "direction_change_max_position"), "set_direction_change_max_position",
                "get_direction_change_max_position");
        ADD_PROPERTY(
                PropertyInfo(Variant::BOOL, "coupled_controllers"), "set_coupled_controllers",
                "get_coupled_controllers");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "initial_delay"), "set_initial_delay", "get_initial_delay");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "step_delay"), "set_step_delay", "get_step_delay");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "step_down_delay"), "set_step_down_delay", "get_step_down_delay");
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "tachometer_max_speed"), "set_tachometer_max_speed",
                "get_tachometer_max_speed");

        ClassDB::bind_method(D_METHOD("get_main_position"), &RailVehicleMasterController::get_main_position);
        ClassDB::bind_method(D_METHOD("get_second_position"), &RailVehicleMasterController::get_second_position);
        ClassDB::bind_method(D_METHOD("get_joint_position"), &RailVehicleMasterController::get_joint_position);
        ClassDB::bind_method(
                D_METHOD("get_main_actual_position"), &RailVehicleMasterController::get_main_actual_position);
        ClassDB::bind_method(
                D_METHOD("get_second_actual_position"), &RailVehicleMasterController::get_second_actual_position);
        ClassDB::bind_method(D_METHOD("get_main_delayed"), &RailVehicleMasterController::get_main_delayed);
        ClassDB::bind_method(
                D_METHOD("get_main_no_power_position"), &RailVehicleMasterController::get_main_no_power_position);
        ClassDB::bind_method(D_METHOD("get_cabin"), &RailVehicleMasterController::get_cabin);
        ClassDB::bind_method(D_METHOD("get_cabin_controleable"), &RailVehicleMasterController::get_cabin_controleable);
        ClassDB::bind_method(D_METHOD("get_tachometer_speed"), &RailVehicleMasterController::get_tachometer_speed);
        ClassDB::bind_method(
                D_METHOD("get_tachometer_speed_jump"), &RailVehicleMasterController::get_tachometer_speed_jump);
        ClassDB::bind_method(
                D_METHOD("get_tachometer_clock_speed"), &RailVehicleMasterController::get_tachometer_clock_speed);
        ClassDB::bind_method(D_METHOD("get_distance_counter"), &RailVehicleMasterController::get_distance_counter);
        ClassDB::bind_method(
                D_METHOD("distance_counter_activate", "pressed"),
                &RailVehicleMasterController::distance_counter_activate);
    }

    void RailVehicleMasterController::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("distance_counter_activate", Callable(this, "distance_counter_activate"));
    }

    void RailVehicleMasterController::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("distance_counter_activate");
    }

    void RailVehicleMasterController::_fill_state_dictionary(Dictionary &p_state) const {
        if (!is_simulation_ready()) {
            return;
        }
        p_state["controller_main_position"] = get_main_position();
        p_state["controller_second_position"] = get_second_position();
        p_state["controller_joint_position"] = get_joint_position();
        p_state["controller_main_actual_position"] = get_main_actual_position();
        p_state["controller_second_actual_position"] = get_second_actual_position();
        p_state["controller_main_delayed"] = get_main_delayed();
        p_state["controller_main_no_power_position"] = get_main_no_power_position();
        p_state["cabin"] = get_cabin();
        p_state["cabin_controleable"] = get_cabin_controleable();
        p_state["tachometer_speed"] = get_tachometer_speed();
        p_state["tachometer_speed_jump"] = get_tachometer_speed_jump();
        p_state["tachometer_clock_speed"] = get_tachometer_clock_speed();
        p_state["distance_counter"] = get_distance_counter();
    }

    void RailVehicleMasterController::set_main_position_count(const int p_value) {
        main_position_count = p_value;
    }

    int RailVehicleMasterController::get_main_position_count() const {
        return main_position_count;
    }

    void RailVehicleMasterController::set_second_position_count(const int p_value) {
        second_position_count = p_value;
    }

    int RailVehicleMasterController::get_second_position_count() const {
        return second_position_count;
    }

    void RailVehicleMasterController::set_direction_change_max_position(const int p_value) {
        direction_change_max_position = p_value;
    }

    int RailVehicleMasterController::get_direction_change_max_position() const {
        return direction_change_max_position;
    }

    void RailVehicleMasterController::set_coupled_controllers(const bool p_value) {
        coupled_controllers = p_value;
    }

    bool RailVehicleMasterController::get_coupled_controllers() const {
        return coupled_controllers;
    }

    void RailVehicleMasterController::set_initial_delay(const double p_value) {
        initial_delay = p_value;
    }

    double RailVehicleMasterController::get_initial_delay() const {
        return initial_delay;
    }

    void RailVehicleMasterController::set_step_delay(const double p_value) {
        step_delay = p_value;
    }

    double RailVehicleMasterController::get_step_delay() const {
        return step_delay;
    }

    void RailVehicleMasterController::set_step_down_delay(const double p_value) {
        step_down_delay = p_value;
    }

    double RailVehicleMasterController::get_step_down_delay() const {
        return step_down_delay;
    }

    void RailVehicleMasterController::set_tachometer_max_speed(const double p_value) {
        tachometer_max_speed = p_value;
    }

    double RailVehicleMasterController::get_tachometer_max_speed() const {
        return tachometer_max_speed;
    }
} // namespace godot
