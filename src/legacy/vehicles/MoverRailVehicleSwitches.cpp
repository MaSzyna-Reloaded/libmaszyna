#include "MoverRailVehicleSwitches.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/rail/RailVehicleSwitches.hpp"

namespace godot {
    void MoverRailVehicleSwitches::_bind_methods() {}


    void MoverRailVehicleSwitches::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();

        p_mover->PantSwitchType = get_pantograph_impulse() ? "impulse" : "";
        p_mover->ConvSwitchType = get_converter_impulse() ? "impulse" : "";
        p_mover->StLinSwitchType = get_motor_connectors_impulse() ? "impulse" : "toggle";
        // LoadFIZ_Switches (Mover.cpp:11395-11397)
        p_mover->UniversalResetButtonFlag[RELAY_RESET_BUTTON_1] = get_relay_reset_button_1();
        p_mover->UniversalResetButtonFlag[RELAY_RESET_BUTTON_2] = get_relay_reset_button_2();
        p_mover->UniversalResetButtonFlag[RELAY_RESET_BUTTON_3] = get_relay_reset_button_3();
        // LoadFIZ_Switches (Mover.cpp:11399-11403) keeps the presets as their digits
        p_mover->PantsPreset.first.clear();
        for (const int preset: get_pantograph_presets()) {
            p_mover->PantsPreset.first.push_back(static_cast<char>('0' + preset));
        }
    }

    // the selector has one position per preset (Train.cpp:3537)
    void MoverRailVehicleSwitches::_fill_config_dictionary(Dictionary &p_config) const {
        RailVehicleSwitches::_fill_config_dictionary(p_config);
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_config["pantograph_preset_max"] = std::max(0, static_cast<int>(mover->PantsPreset.first.size()) - 1);
    }

    int MoverRailVehicleSwitches::get_pantograph_preset_position(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->PantsPreset.second[p_end] : 0;
    }

    // Train.cpp:3522 - a preset is its digit
    RailVehicleSwitches::PantographPreset
    MoverRailVehicleSwitches::get_pantograph_preset(const RailVehicleController::CouplerEnd p_end) const {
        const TMoverParameters *mover = get_mover();
        // a selection past the presets reconfigured since is none
        if (mover == nullptr || mover->PantsPreset.second[p_end] >= static_cast<int>(mover->PantsPreset.first.size())) {
            return PANTOGRAPH_PRESET_NONE;
        }
        return static_cast<PantographPreset>(mover->PantsPreset.first[mover->PantsPreset.second[p_end]] - '0');
    }

    // Train.cpp:3532 change_pantograph_selection - within the presets
    void MoverRailVehicleSwitches::next_pantograph_preset(const RailVehicleController::CouplerEnd p_end) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        int &selection = mover->PantsPreset.second[p_end];
        selection = std::clamp(selection + 1, 0, std::max(static_cast<int>(mover->PantsPreset.first.size()) - 1, 0));
    }

    void MoverRailVehicleSwitches::previous_pantograph_preset(const RailVehicleController::CouplerEnd p_end) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        int &selection = mover->PantsPreset.second[p_end];
        selection = std::clamp(selection - 1, 0, std::max(static_cast<int>(mover->PantsPreset.first.size()) - 1, 0));
    }


    bool MoverRailVehicleSwitches::get_sand_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SandDose : false;
    }

    void MoverRailVehicleSwitches::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["sand_active"] = get_sand_active();
        p_state["pantograph_preset_position_front"] =
                get_pantograph_preset_position(RailVehicleController::COUPLER_END_FRONT);
        p_state["pantograph_preset_position_rear"] =
                get_pantograph_preset_position(RailVehicleController::COUPLER_END_REAR);
        p_state["pantograph_preset_front"] = get_pantograph_preset(RailVehicleController::COUPLER_END_FRONT);
        p_state["pantograph_preset_rear"] = get_pantograph_preset(RailVehicleController::COUPLER_END_REAR);
    }

    // Train.cpp:5175 OnCommand_universalrelayreset - the occupied vehicle's own buttons
    void MoverRailVehicleSwitches::universal_relay_reset(const RelayResetButton p_button) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->UniversalResetButton(p_button);
    }

    void MoverRailVehicleSwitches::sand(const bool p_active) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        // Train.cpp:1917-1939 (OnCommand_sandboxactivate) -> SandboxManual(State),
        // "sand_bt:"/ggSandButton (Train.cpp:10044) - momentary, active only while held.
        mover->SandboxManual(p_active);
    }


} // namespace godot
