#include "TrainSpeedControl.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainSpeedControl::_bind_methods() {
        BIND_PROPERTY(
                Variant::BOOL, "speed_control_enabled", "speed_control_enabled",
                &TrainSpeedControl::set_speed_control_enabled, &TrainSpeedControl::get_speed_control_enabled,
                "enabled");
        BIND_PROPERTY(
                Variant::FLOAT, "delay", "delay", &TrainSpeedControl::set_delay, &TrainSpeedControl::get_delay,
                "delay");
        BIND_PROPERTY(
                Variant::BOOL, "impulse_lever", "impulse_lever", &TrainSpeedControl::set_impulse_lever,
                &TrainSpeedControl::get_impulse_lever, "impulse_lever");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "disables_on", "disables_on", &TrainSpeedControl::set_disables_on,
                &TrainSpeedControl::get_disables_on, "disables_on", PROPERTY_HINT_FLAGS,
                "Main Controller Movement,Braking");
        BIND_PROPERTY(
                Variant::PACKED_FLOAT64_ARRAY, "preset_speeds", "preset_speeds", &TrainSpeedControl::set_preset_speeds,
                &TrainSpeedControl::get_preset_speeds, "preset_speeds");
        BIND_PROPERTY(
                Variant::BOOL, "override_manual_power", "override_manual_power",
                &TrainSpeedControl::set_override_manual_power, &TrainSpeedControl::get_override_manual_power,
                "override_manual_power");
        BIND_PROPERTY(
                Variant::FLOAT, "initial_power", "initial_power", &TrainSpeedControl::set_initial_power,
                &TrainSpeedControl::get_initial_power, "initial_power");
        BIND_PROPERTY(
                Variant::FLOAT, "full_power_velocity", "full_power_velocity",
                &TrainSpeedControl::set_full_power_velocity, &TrainSpeedControl::get_full_power_velocity,
                "full_power_velocity");
        BIND_PROPERTY(
                Variant::FLOAT, "start_velocity", "start_velocity", &TrainSpeedControl::set_start_velocity,
                &TrainSpeedControl::get_start_velocity, "start_velocity");
        BIND_PROPERTY(
                Variant::FLOAT, "velocity_step", "velocity_step", &TrainSpeedControl::set_velocity_step,
                &TrainSpeedControl::get_velocity_step, "velocity_step");
        BIND_PROPERTY(
                Variant::FLOAT, "power_step", "power_step", &TrainSpeedControl::set_power_step,
                &TrainSpeedControl::get_power_step, "power_step");
        BIND_PROPERTY(
                Variant::FLOAT, "min_power", "min_power", &TrainSpeedControl::set_min_power,
                &TrainSpeedControl::get_min_power, "min_power");
        BIND_PROPERTY(
                Variant::FLOAT, "max_power", "max_power", &TrainSpeedControl::set_max_power,
                &TrainSpeedControl::get_max_power, "max_power");
        BIND_PROPERTY(
                Variant::FLOAT, "min_velocity", "min_velocity", &TrainSpeedControl::set_min_velocity,
                &TrainSpeedControl::get_min_velocity, "min_velocity");
        BIND_PROPERTY(
                Variant::FLOAT, "max_velocity", "max_velocity", &TrainSpeedControl::set_max_velocity,
                &TrainSpeedControl::get_max_velocity, "max_velocity");
        BIND_PROPERTY(
                Variant::FLOAT, "offset", "offset", &TrainSpeedControl::set_offset, &TrainSpeedControl::get_offset,
                "offset");
        BIND_PROPERTY(
                Variant::FLOAT, "proportional_gain_positive", "proportional_gain_positive",
                &TrainSpeedControl::set_proportional_gain_positive,
                &TrainSpeedControl::get_proportional_gain_positive, "proportional_gain_positive");
        BIND_PROPERTY(
                Variant::FLOAT, "proportional_gain_negative", "proportional_gain_negative",
                &TrainSpeedControl::set_proportional_gain_negative,
                &TrainSpeedControl::get_proportional_gain_negative, "proportional_gain_negative");
        BIND_PROPERTY(
                Variant::FLOAT, "integral_gain_positive", "integral_gain_positive",
                &TrainSpeedControl::set_integral_gain_positive, &TrainSpeedControl::get_integral_gain_positive,
                "integral_gain_positive");
        BIND_PROPERTY(
                Variant::FLOAT, "integral_gain_negative", "integral_gain_negative",
                &TrainSpeedControl::set_integral_gain_negative, &TrainSpeedControl::get_integral_gain_negative,
                "integral_gain_negative");
        BIND_PROPERTY(
                Variant::BOOL, "brake_intervention", "brake_intervention", &TrainSpeedControl::set_brake_intervention,
                &TrainSpeedControl::get_brake_intervention, "brake_intervention");
        BIND_PROPERTY(
                Variant::FLOAT, "brake_intervention_max_velocity", "brake_intervention_max_velocity",
                &TrainSpeedControl::set_brake_intervention_max_velocity,
                &TrainSpeedControl::get_brake_intervention_max_velocity, "brake_intervention_max_velocity");
        BIND_PROPERTY(
                Variant::FLOAT, "power_up_speed", "power_up_speed", &TrainSpeedControl::set_power_up_speed,
                &TrainSpeedControl::get_power_up_speed, "power_up_speed");
        BIND_PROPERTY(
                Variant::FLOAT, "power_down_speed", "power_down_speed", &TrainSpeedControl::set_power_down_speed,
                &TrainSpeedControl::get_power_down_speed, "power_down_speed");
    }

    void TrainSpeedControl::_do_update_internal_mover(TMoverParameters *p_mover) {
        ASSERT_MOVER(p_mover);
        TrainPart::_do_update_internal_mover(p_mover);

        p_mover->SpeedCtrl = speed_control_enabled;
        p_mover->SpeedCtrlDelay = delay;
        p_mover->SpeedCtrlTypeTime = impulse_lever;
        p_mover->SpeedCtrlAutoTurnOffFlag = disables_on;

        constexpr int MAX_PRESET_SPEEDS = 10;
        const int preset_speeds_size = preset_speeds.size();
        if (preset_speeds_size > MAX_PRESET_SPEEDS) {
            UtilityFunctions::push_warning(
                    "[TrainSpeedControl]: preset_speeds has " + String::num_int64(preset_speeds_size) +
                    " entries, exceeding the mover's limit of " + String::num_int64(MAX_PRESET_SPEEDS) +
                    "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_PRESET_SPEEDS, preset_speeds_size); i++) {
            p_mover->SpeedCtrlButtons[i] = preset_speeds[i];
        }

        p_mover->SpeedCtrlUnit.ManualStateOverride = override_manual_power;
        p_mover->SpeedCtrlUnit.InitialPower = initial_power;
        p_mover->SpeedCtrlUnit.FullPowerVelocity = full_power_velocity;
        p_mover->SpeedCtrlUnit.StartVelocity = start_velocity;
        p_mover->SpeedCtrlUnit.VelocityStep = velocity_step;
        p_mover->SpeedCtrlUnit.PowerStep = power_step;
        p_mover->SpeedCtrlUnit.MinPower = min_power;
        p_mover->SpeedCtrlUnit.MaxPower = max_power;
        p_mover->SpeedCtrlUnit.MinVelocity = min_velocity;
        p_mover->SpeedCtrlUnit.MaxVelocity = max_velocity;
        p_mover->SpeedCtrlUnit.Offset = offset;
        p_mover->SpeedCtrlUnit.FactorPpos = proportional_gain_positive;
        p_mover->SpeedCtrlUnit.FactorPneg = proportional_gain_negative;
        p_mover->SpeedCtrlUnit.FactorIpos = integral_gain_positive;
        p_mover->SpeedCtrlUnit.FactorIneg = integral_gain_negative;
        p_mover->SpeedCtrlUnit.BrakeIntervention = brake_intervention;
        p_mover->SpeedCtrlUnit.BrakeInterventionVel = brake_intervention_max_velocity;
        p_mover->SpeedCtrlUnit.PowerUpSpeed = power_up_speed;
        p_mover->SpeedCtrlUnit.PowerDownSpeed = power_down_speed;
    }

    void TrainSpeedControl::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        ASSERT_MOVER(p_mover);
        p_state["speed_control/active"] = p_mover->SpeedCtrlUnit.IsActive;
        p_state["speed_control/desired_velocity"] = p_mover->SpeedCtrlUnit.DesiredVelocity;
        p_state["speed_control/desired_power"] = p_mover->SpeedCtrlUnit.DesiredPower;
        p_state["speed_control/selected_velocity"] = p_mover->SpeedCtrlValue;
    }
} // namespace godot
