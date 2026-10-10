#include "MoverRailVehicleElectroPneumaticDynamicBrake.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/rail/RailVehicleElectroPneumaticDynamicBrake.hpp"

namespace godot {
    void MoverRailVehicleElectroPneumaticDynamicBrake::_bind_methods() {}


    double MoverRailVehicleElectroPneumaticDynamicBrake::get_ed_braking_ep_delay() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->DCEMUED_EP_delay : 0.0;
    }

    double MoverRailVehicleElectroPneumaticDynamicBrake::get_ep_max_brake_engagement_speed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->DCEMUED_EP_max_Vel : 0.0;
    }

    double MoverRailVehicleElectroPneumaticDynamicBrake::get_ep_min_regenerative_braking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->DCEMUED_EP_min_Im : 0.0;
    }

    double MoverRailVehicleElectroPneumaticDynamicBrake::get_ep_force() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EpForce : 0.0;
    }

    bool MoverRailVehicleElectroPneumaticDynamicBrake::get_ep_fuse() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->EpFuse : false;
    }

    void MoverRailVehicleElectroPneumaticDynamicBrake::_fill_state_dictionary(Dictionary &p_state) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_state["dcemued/coupler_check"] = get_coupler_check();
        p_state["dcemued/ed_braking_ep_delay"] = get_ed_braking_ep_delay();
        p_state["dcemued/ep_max_brake_engagement_speed"] = get_ep_max_brake_engagement_speed();
        p_state["dcemued/ep_min_regenerative_braking"] = get_ep_min_regenerative_braking();
        p_state["dcemued/ep_force"] = get_ep_force();
        p_state["dcemued/ep_fuse"] = get_ep_fuse();
    }


    void MoverRailVehicleElectroPneumaticDynamicBrake::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->DCEMUED_CC = get_coupler_check();
        p_mover->DCEMUED_EP_delay = get_electro_pneumatic_brake_delay();
        p_mover->DCEMUED_EP_max_Vel = get_electro_pneumatic_max_ep_brake_engagement_speed();
        p_mover->DCEMUED_EP_min_Im = get_electro_pneumatic_min_regenerative_braking();
        p_mover->EpFuseSwitch(get_ep_brake_fuse());

        p_mover->MED_Vmax = get_blending_max_velocity();
        p_mover->MED_Vmin = get_blending_min_velocity();
        p_mover->MED_Vref = get_blending_reference_velocity();
        p_mover->MED_amax = get_blending_max_deceleration();
        p_mover->MED_EPVC = get_blending_velocity_correction();
        p_mover->MED_Ncor = get_blending_load_correction();
        p_mover->MED_MinBrakeReqED = get_blending_min_ed_brake_request();
    }

    void MoverRailVehicleElectroPneumaticDynamicBrake::set_ep_brake_force(const int p_value) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->SwitchEPBrake(p_value);
    }


    void MoverRailVehicleElectroPneumaticDynamicBrake::switch_ep_fuse(const bool p_value) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->EpFuseSwitch(p_value);
    }
} // namespace godot
