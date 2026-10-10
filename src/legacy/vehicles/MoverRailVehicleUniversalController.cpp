#include "MoverRailVehicleUniversalController.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleUniversalController::_bind_methods() {}


    void MoverRailVehicleUniversalController::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();

        p_mover->UniCtrlIntegratedBrakePNCtrl = get_integrated_brake_pn();
        p_mover->UniCtrlIntegratedBrakeCtrl = get_integrated_brake();
        p_mover->UniCtrlIntegratedLocalBrakeCtrl = get_integrated_local_brake();

        constexpr int MAX_POSITIONS = Maszyna::UniversalCtrlArraySize;
        const int requested_size = static_cast<int>(get_positions().size());
        if (requested_size > MAX_POSITIONS) {
            UtilityFunctions::push_warning(
                    "[RailVehicleUniversalController]: get_positions() has " + String::num_int64(requested_size) +
                    " entries, exceeding the mover's limit of " + String::num_int64(MAX_POSITIONS) + "; truncating.");
        }
        p_mover->UniCtrlListSize = std::min(MAX_POSITIONS, requested_size);

        for (int i = 0; i < p_mover->UniCtrlListSize; ++i) {
            Ref<RailVehicleUniversalControllerListItem> item = get_positions()[i];
            if (item.is_valid()) {
                p_mover->UniCtrlList[i].mode = item->get_pneumatic_brake_position();
                p_mover->UniCtrlList[i].MinCtrlVal = item->get_min_percentage();
                p_mover->UniCtrlList[i].MaxCtrlVal = item->get_max_percentage();
                p_mover->UniCtrlList[i].SetCtrlVal = item->get_target_value();
                p_mover->UniCtrlList[i].SpeedUp = item->get_increase_speed();
                p_mover->UniCtrlList[i].SpeedDown = item->get_decrease_speed();
                p_mover->UniCtrlList[i].ReturnPosition = item->get_bounce_back_position();
                p_mover->UniCtrlList[i].NextPosFastInc = item->get_nearest_stable_up();
                p_mover->UniCtrlList[i].PrevPosFastDec = item->get_nearest_stable_down();
            }
        }
    }


    void MoverRailVehicleUniversalController::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["selector_position"] = get_selector_position();
    }

    void MoverRailVehicleUniversalController::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        VehicleComponent::_fill_config_dictionary(p_config);
    }
} // namespace godot
