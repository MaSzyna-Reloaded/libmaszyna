#include "TrainUniversalController.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainUniversalController::_bind_methods() {
        BIND_PROPERTY(TrainUniversalController, Variant::BOOL, integrated_brake_pn);
        BIND_PROPERTY(TrainUniversalController, Variant::BOOL, integrated_brake);
        BIND_PROPERTY(TrainUniversalController, Variant::INT, selector_position);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                TrainUniversalController, Variant::ARRAY, positions, PROPERTY_HINT_TYPE_STRING,
                "UniversalControllerListItem");
    }

    void TrainUniversalController::_do_update_internal_mover(TMoverParameters *p_mover) {
        ASSERT_MOVER(p_mover);
        TrainPart::_do_update_internal_mover(p_mover);

        p_mover->UniCtrlIntegratedBrakePNCtrl = integrated_brake_pn;
        p_mover->UniCtrlIntegratedBrakeCtrl = integrated_brake;

        constexpr int MAX_POSITIONS = Maszyna::UniversalCtrlArraySize;
        const int requested_size = static_cast<int>(positions.size());
        if (requested_size > MAX_POSITIONS) {
            UtilityFunctions::push_warning(
                    "[TrainUniversalController]: positions has " + String::num_int64(requested_size) +
                    " entries, exceeding the mover's limit of " + String::num_int64(MAX_POSITIONS) + "; truncating.");
        }
        p_mover->UniCtrlListSize = std::min(MAX_POSITIONS, requested_size);

        for (int i = 0; i < p_mover->UniCtrlListSize; ++i) {
            Ref<UniversalControllerListItem> item = positions[i];
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

        p_mover->MainCtrlPos = selector_position;
    }

    void TrainUniversalController::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        ASSERT_MOVER(p_mover);
        p_state["selector_position"] = p_mover->MainCtrlPos;
    }

    void TrainUniversalController::_do_fetch_config_from_mover(TMoverParameters *p_mover, Dictionary &p_config) {
        TrainPart::_do_fetch_config_from_mover(p_mover, p_config);
    }
} // namespace godot
