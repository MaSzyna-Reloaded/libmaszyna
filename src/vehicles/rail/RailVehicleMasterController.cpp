#include "RailVehicleMasterController.hpp"
#include <cmath>
#include <godot_cpp/variant/utility_functions.hpp>

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

    void RailVehicleMasterController::_do_process_component(const double p_delta) {
        const VehicleController *controller = get_controller().ptr();
        ERR_FAIL_NULL(controller);
        // Original engine: TTrain::Update() Hasler block (Train.cpp:8580-8611) and its tachoclock
        // sound gate (Train.cpp:10091-10103).
        tachometer_velocity = std::min(
                std::abs(
                        TACHOMETER_WHEEL_SPEED_FACTOR * _get_powered_wheel_diameter() *
                        _get_powered_axle_revolutions()),
                get_tachometer_max_speed() != 0.0 ? get_tachometer_max_speed()
                                                  : _get_max_velocity() * TACHOMETER_MAX_SPEED_FACTOR);

        // the needle jumps once per simulation second, with a small random error; below walking
        // speed it swings at random and stays where it was when the vehicle stops (Train.cpp:8594-8597)
        const double previous_second = std::floor(tachometer_time);
        tachometer_time += p_delta;
        if (std::floor(tachometer_time) != previous_second) {
            if (tachometer_velocity >= TACHOMETER_MOVING_VELOCITY) {
                tachometer_velocity_jump =
                        tachometer_velocity +
                        ((TACHOMETER_JUMP_OFFSET - UtilityFunctions::randf_range(0.0, TACHOMETER_JUMP_RANDOM_RANGE) +
                          UtilityFunctions::randf_range(0.0, TACHOMETER_JUMP_RANDOM_RANGE)) *
                         TACHOMETER_JUMP_SCALE);
            } else if (tachometer_velocity > TACHOMETER_MIN_VELOCITY) {
                tachometer_velocity_jump = UtilityFunctions::randf_range(0.0, TACHOMETER_SWING_RANGE);
            }
        }

        // ticking starts ~1 s after moving off and fades out slowly after stopping
        if (tachometer_velocity > TACHOMETER_MIN_VELOCITY) {
            tachometer_count =
                    std::min(MAX_TACHOMETER_COUNT, tachometer_count + (p_delta * TACHOMETER_COUNT_RISE_RATE));
        } else if (tachometer_count > 0.0) {
            tachometer_count = std::max(0.0, tachometer_count - (p_delta * TACHOMETER_COUNT_FALL_RATE));
        }
        if (tachometer_count >= MAX_TACHOMETER_COUNT) {
            tachometer_clock_active = true;
        } else if (tachometer_count < TACHOMETER_CLOCK_STOP_COUNT) {
            tachometer_clock_active = false;
        }

        // TTrain::add_distance (Train.cpp:10309) - counted towards the occupied cab, and switched
        // off for good whenever the low voltage goes
        if (distance_counter >= 0.0 && _get_low_voltage_available()) {
            distance_counter += controller->get_velocity() * p_delta * _get_occupied_cab();
        } else {
            distance_counter = DISTANCE_COUNTER_OFF;
        }
    }

    double RailVehicleMasterController::get_tachometer_speed() const {
        return tachometer_velocity;
    }

    double RailVehicleMasterController::get_tachometer_speed_jump() const {
        return tachometer_velocity_jump;
    }

    double RailVehicleMasterController::get_tachometer_clock_speed() const {
        return tachometer_clock_active ? tachometer_velocity : 0.0;
    }

    double RailVehicleMasterController::get_distance_counter() const {
        return distance_counter;
    }

    // Original engine: TTrain::OnCommand_distancecounteractivate (Train.cpp:1552), single-press form
    void RailVehicleMasterController::distance_counter_activate(const bool p_pressed) {
        if (p_pressed) {
            distance_counter = 0.0;
        }
    }
} // namespace godot
