#include "MoverRailVehicleSpeedControl.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/rail/RailVehicleEngine.hpp"
#include "vehicles/rail/RailVehicleMasterController.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>
#include <type_traits>

namespace godot {
    void MoverRailVehicleSpeedControl::_bind_methods() {}


    /* Mover.cpp:11097 - an induction motor with a second controller has it whatever its FIZ says;
     * the engine and the master controller are other components of the vehicle */
    void MoverRailVehicleSpeedControl::apply_vehicle_config() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        const Ref<RailVehicleEngine> engine =
                train_controller_node->get_component(VehicleComponentType::COMPONENT_ENGINE);
        const Ref<RailVehicleMasterController> master_controller = get_rail_vehicle_controller()->get_rail_component(
                RailVehicleComponentType::COMPONENT_MASTER_CONTROLLER);
        p_mover->SpeedCtrl = get_speed_control_enabled() ||
                             (engine.is_valid() && engine->get_type() == RailVehicleEngine::ELECTRIC_INDUCTION_MOTOR &&
                              master_controller.is_valid() && master_controller->get_second_position_count() > 0);
    }

    void MoverRailVehicleSpeedControl::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();

        p_mover->SpeedCtrlDelay = get_delay();
        p_mover->SpeedCtrlTypeTime = get_impulse_lever();
        p_mover->SpeedCtrlAutoTurnOffFlag = get_disables_on();

        constexpr int MAX_PRESET_SPEEDS = static_cast<int>(std::extent_v<decltype(TMoverParameters::SpeedCtrlButtons)>);
        const int preset_speeds_size = static_cast<int>(get_preset_speeds().size());
        if (preset_speeds_size > MAX_PRESET_SPEEDS) {
            UtilityFunctions::push_warning(
                    "[RailVehicleSpeedControl]: get_preset_speeds() has " + String::num_int64(preset_speeds_size) +
                    " entries, exceeding the mover's limit of " + String::num_int64(MAX_PRESET_SPEEDS) +
                    "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_PRESET_SPEEDS, preset_speeds_size); i++) {
            p_mover->SpeedCtrlButtons[i] = get_preset_speeds()[i];
        }

        p_mover->SpeedCtrlUnit.ManualStateOverride = get_override_manual_power();
        p_mover->SpeedCtrlUnit.InitialPower = get_initial_power();
        p_mover->SpeedCtrlUnit.FullPowerVelocity = get_full_power_velocity();
        p_mover->SpeedCtrlUnit.StartVelocity = get_start_velocity();
        p_mover->SpeedCtrlUnit.VelocityStep = get_velocity_step();
        p_mover->SpeedCtrlUnit.PowerStep = get_power_step();
        p_mover->SpeedCtrlUnit.MinPower = get_min_power();
        p_mover->SpeedCtrlUnit.MaxPower = get_max_power();
        p_mover->SpeedCtrlUnit.MinVelocity = get_min_velocity();
        p_mover->SpeedCtrlUnit.MaxVelocity = get_max_velocity();
        p_mover->SpeedCtrlUnit.Offset = get_offset();
        p_mover->SpeedCtrlUnit.FactorPpos = get_proportional_gain_positive();
        p_mover->SpeedCtrlUnit.FactorPneg = get_proportional_gain_negative();
        p_mover->SpeedCtrlUnit.FactorIpos = get_integral_gain_positive();
        p_mover->SpeedCtrlUnit.FactorIneg = get_integral_gain_negative();
        p_mover->SpeedCtrlUnit.BrakeIntervention = get_brake_intervention();
        p_mover->SpeedCtrlUnit.BrakeInterventionVel = get_brake_intervention_max_velocity();
        p_mover->SpeedCtrlUnit.PowerUpSpeed = get_power_up_speed();
        p_mover->SpeedCtrlUnit.PowerDownSpeed = get_power_down_speed();
    }


    bool MoverRailVehicleSpeedControl::get_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpeedCtrlUnit.IsActive : false;
    }

    double MoverRailVehicleSpeedControl::get_desired_velocity() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpeedCtrlUnit.DesiredVelocity : 0.0;
    }

    double MoverRailVehicleSpeedControl::get_desired_power() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpeedCtrlUnit.DesiredPower : 0.0;
    }

    double MoverRailVehicleSpeedControl::get_selected_velocity() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpeedCtrlValue : 0.0;
    }

    double MoverRailVehicleSpeedControl::get_set_velocity() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->NewSpeed : 0.0;
    }

    bool MoverRailVehicleSpeedControl::get_standby() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->SpeedCtrlUnit.Standby;
    }

    void MoverRailVehicleSpeedControl::speed_control_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpeedCtrlInc();
    }

    void MoverRailVehicleSpeedControl::speed_control_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpeedCtrlDec();
    }

    void MoverRailVehicleSpeedControl::speed_control_power_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpeedCtrlPowerInc();
    }

    void MoverRailVehicleSpeedControl::speed_control_power_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpeedCtrlPowerDec();
    }

    void MoverRailVehicleSpeedControl::speed_control_button(const int p_button) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpeedCtrlButton(p_button);
    }

    void MoverRailVehicleSpeedControl::speed_control_set(const double p_velocity) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->RunCommand("SpeedCntrl", p_velocity, mover->CabActive);
    }

    void MoverRailVehicleSpeedControl::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["speed_control/active"] = get_active();
        p_state["speed_control/desired_velocity"] = get_desired_velocity();
        p_state["speed_control/desired_power"] = get_desired_power();
        p_state["speed_control/selected_velocity"] = get_selected_velocity();
        p_state["speed_control/set_velocity"] = get_set_velocity();
        p_state["speed_control/standby"] = get_standby();
    }
} // namespace godot
