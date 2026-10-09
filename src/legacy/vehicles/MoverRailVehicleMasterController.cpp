#include "MoverRailVehicleMasterController.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <cmath>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleMasterController::_bind_methods() {}

    void MoverRailVehicleMasterController::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();

        p_mover->MainCtrlPosNo = get_main_position_count();
        p_mover->ScndCtrlPosNo = get_second_position_count();
        p_mover->MainCtrlMaxDirChangePos = get_direction_change_max_position();
        p_mover->CoupledCtrl = get_coupled_controllers();
        p_mover->InitialCtrlDelay = get_initial_delay();
        p_mover->CtrlDelay = get_step_delay();
        p_mover->CtrlDownDelay = get_step_down_delay();
    }

    void MoverRailVehicleMasterController::_fill_config_dictionary(Dictionary &p_config) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        VehicleComponent::_fill_config_dictionary(p_config);
        p_config["main_controller_position_max"] = mover->MainCtrlPosNo;
        p_config["second_controller_position_max"] = mover->ScndCtrlPosNo;
        // the cab's master controller: with a coupled controller its shaft goes on into the field
        // shunt past the last main position (Train.cpp:985, 1133; Mover.cpp:2335)
        p_config["master_controller_position_max"] =
                mover->CoupledCtrl ? mover->MainCtrlPosNo + mover->ScndCtrlPosNo : mover->MainCtrlPosNo;
    }

    /* Where the cab's master controller stands - the shunt steps counted on with a coupled
     * controller (Train.cpp:9410) */
    void MoverRailVehicleMasterController::_fill_state_dictionary(Dictionary &p_state) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        RailVehicleMasterController::_fill_state_dictionary(p_state);
        p_state["master_controller_position"] =
                mover->CoupledCtrl ? mover->MainCtrlPos + mover->ScndCtrlPos : mover->MainCtrlPos;
    }

    void MoverRailVehicleMasterController::_do_process_component(const double p_delta) {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        // Original engine: TTrain::Update() Hasler block (Train.cpp:6917-6940) and its tachoclock
        // sound gate (Train.cpp:8323-8335).
        const double max_tachometer = 3.0;
        tachometer_velocity = std::min(std::abs(11.31 * mover->WheelDiameter * mover->nrot), mover->Vmax * 1.05);

        // the needle jumps once per simulation second, with a small random error
        const double previous_second = std::floor(tachometer_time);
        tachometer_time += p_delta;
        if (std::floor(tachometer_time) != previous_second) {
            tachometer_velocity_jump = tachometer_velocity > 1.0
                                               ? tachometer_velocity + ((2.0 - UtilityFunctions::randf_range(0.0, 3.0) +
                                                                         UtilityFunctions::randf_range(0.0, 3.0)) *
                                                                        0.5)
                                               : 0.0;
        }

        // ticking starts ~1 s after moving off and fades out slowly after stopping
        if (tachometer_velocity > 1.0) {
            tachometer_count = std::min(max_tachometer, tachometer_count + (p_delta * 3.0));
        } else if (tachometer_count > 0.0) {
            tachometer_count = std::max(0.0, tachometer_count - (p_delta * 0.66));
        }
        if (tachometer_count >= 3.0) {
            tachometer_clock_active = true;
        } else if (tachometer_count < 1.0) {
            tachometer_clock_active = false;
        }

        // TTrain::add_distance (Train.cpp:10309) - counted towards the occupied cab, and switched
        // off for good whenever the low voltage goes
        if (distance_counter >= 0.0 && (mover->Power24vIsAvailable || mover->Power110vIsAvailable)) {
            distance_counter += mover->V * p_delta * mover->CabOccupied;
        } else {
            distance_counter = DISTANCE_COUNTER_OFF;
        }
    }

    int MoverRailVehicleMasterController::get_main_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->MainCtrlPos : 0;
    }

    int MoverRailVehicleMasterController::get_second_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ScndCtrlPos : 0;
    }

    int MoverRailVehicleMasterController::get_joint_position() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0;
        }
        if (mover->LocalBrakePosA > 0.0) {
            return static_cast<int>(std::round(-mover->LocalBrakePosA * LocalBrakePosNo));
        }
        return mover->CoupledCtrl ? mover->MainCtrlPos + mover->ScndCtrlPos : mover->MainCtrlPos;
    }

    int MoverRailVehicleMasterController::get_main_actual_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->MainCtrlActualPos : 0;
    }

    int MoverRailVehicleMasterController::get_second_actual_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->ScndCtrlActualPos : 0;
    }

    bool MoverRailVehicleMasterController::get_main_delayed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->DelayCtrlFlag : false;
    }

    int MoverRailVehicleMasterController::get_main_no_power_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->MainCtrlNoPowerPos() : 0;
    }

    int MoverRailVehicleMasterController::get_cabin() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->CabActive : 0;
    }

    bool MoverRailVehicleMasterController::get_cabin_controleable() const {
        TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->IsCabMaster() : false;
    }

    double MoverRailVehicleMasterController::get_tachometer_speed() const {
        return get_mover() != nullptr ? tachometer_velocity : 0.0;
    }

    double MoverRailVehicleMasterController::get_tachometer_speed_jump() const {
        return get_mover() != nullptr ? tachometer_velocity_jump : 0.0;
    }

    double MoverRailVehicleMasterController::get_tachometer_clock_speed() const {
        if (get_mover() == nullptr || !tachometer_clock_active) {
            return 0.0;
        }
        return tachometer_velocity;
    }

    double MoverRailVehicleMasterController::get_distance_counter() const {
        return distance_counter;
    }

    // Original engine: TTrain::OnCommand_distancecounteractivate (Train.cpp:1552), single-press form
    void MoverRailVehicleMasterController::distance_counter_activate(const bool p_pressed) {
        if (p_pressed) {
            distance_counter = 0.0;
        }
    }
} // namespace godot
