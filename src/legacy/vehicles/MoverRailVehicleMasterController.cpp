#include "MoverRailVehicleMasterController.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <cmath>

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

    double MoverRailVehicleMasterController::_get_powered_wheel_diameter() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->WheelDiameter : 0.0;
    }

    double MoverRailVehicleMasterController::_get_powered_axle_revolutions() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->nrot : 0.0;
    }

    double MoverRailVehicleMasterController::_get_max_velocity() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Vmax : 0.0;
    }

    bool MoverRailVehicleMasterController::_get_low_voltage_available() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && (mover->Power24vIsAvailable || mover->Power110vIsAvailable);
    }

    int MoverRailVehicleMasterController::_get_occupied_cab() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->CabOccupied : 0;
    }
} // namespace godot
