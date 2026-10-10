#include "MoverRailVehicleLighting.hpp"
#include "legacy/maszyna-mover/utilities.h"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleLighting::_bind_methods() {}


    void MoverRailVehicleLighting::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        VehicleComponent::_apply_configuration();
        // readLightsList (Mover.cpp:8558) - one row per preset, the table holds LIGHTS_LIST_CAPACITY
        const int presets = std::min(static_cast<int>(light_position_list.size()), LIGHTS_LIST_CAPACITY);
        for (int preset = 0; preset < presets; ++preset) {
            const Ref<RailVehicleLightListItem> item = light_position_list[preset];
            if (item.is_null()) {
                continue;
            }
            // the row as the Mover's light bits (enum light, MOVER.h:189), cabin A's end then cabin B's
            p_mover->Lights[Maszyna::end::front][preset] =
                    (item->get_cabin_a_head_light() ? Maszyna::light::headlight_upper : 0) |
                    (item->get_cabin_a_left_white_signal() ? Maszyna::light::headlight_left : 0) |
                    (item->get_cabin_a_left_red_signal() ? Maszyna::light::redmarker_left : 0) |
                    (item->get_cabin_a_right_white_signal() ? Maszyna::light::headlight_right : 0) |
                    (item->get_cabin_a_right_red_signal() ? Maszyna::light::redmarker_right : 0) |
                    (item->get_cabin_a_end_signals() ? Maszyna::light::rearendsignals : 0) |
                    (item->get_cabin_a_left_auxiliary_light() ? Maszyna::light::auxiliary_left : 0) |
                    (item->get_cabin_a_right_auxiliary_light() ? Maszyna::light::auxiliary_right : 0);
            p_mover->Lights[Maszyna::end::rear][preset] =
                    (item->get_cabin_b_head_light() ? Maszyna::light::headlight_upper : 0) |
                    (item->get_cabin_b_left_white_signal() ? Maszyna::light::headlight_left : 0) |
                    (item->get_cabin_b_left_red_signal() ? Maszyna::light::redmarker_left : 0) |
                    (item->get_cabin_b_right_white_signal() ? Maszyna::light::headlight_right : 0) |
                    (item->get_cabin_b_right_red_signal() ? Maszyna::light::redmarker_right : 0) |
                    (item->get_cabin_b_end_signals() ? Maszyna::light::rearendsignals : 0) |
                    (item->get_cabin_b_left_auxiliary_light() ? Maszyna::light::auxiliary_left : 0) |
                    (item->get_cabin_b_right_auxiliary_light() ? Maszyna::light::auxiliary_right : 0);
        }
        p_mover->LightsPosNo = presets;
        p_mover->LightsWrap = get_lights_wrap_selector();
        // the selector starts at LightsDefPos, set by CheckLocomotiveParameters (Mover.cpp:8885)
        p_mover->LightsDefPos = get_lights_default_selector_position();
        p_mover->LightPowerSource.SourceType = MaszynaMoverVehicleServer::power_source_to_mover(get_light_source());
        p_mover->AlterLightPowerSource.SourceType =
                MaszynaMoverVehicleServer::power_source_to_mover(get_light_alternative_source());
    }

    bool MoverRailVehicleLighting::_light_enabled(
            const TMoverParameters *p_mover, const LightEnd p_end, const LightType p_type) const {
        const int lights = p_mover->iLights[static_cast<int>(light_end_map.at(p_end))];
        return (lights & light_type_mask_map.at(p_type)) != 0;
    }

    /* The end of the cab the driver sits in, active or not - the machine room counts as the front
     * (cab_to_end(), Train.h:220-227). Taken from the active cab, a switch in a cab switched off
     * lit the other end's lamps (docs/findings-archive.md, 2026-10-06) */
    MoverRailVehicleLighting::LightEnd MoverRailVehicleLighting::_active_end(const TMoverParameters *p_mover) {
        return p_mover->CabOccupied < 0 ? LIGHT_END_REAR : LIGHT_END_FRONT;
    }

    MoverRailVehicleLighting::LightEnd MoverRailVehicleLighting::_opposite_end(const TMoverParameters *p_mover) {
        return _active_end(p_mover) == LIGHT_END_FRONT ? LIGHT_END_REAR : LIGHT_END_FRONT;
    }

    int MoverRailVehicleLighting::get_position() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LightsPos : 0;
    }

    double MoverRailVehicleLighting::get_power() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->LightPower : 0.0;
    }

    int MoverRailVehicleLighting::get_power_source() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? MaszynaMoverVehicleServer::power_source_from_mover(mover->LightPowerSource.SourceType)
                                : 0;
    }

    bool MoverRailVehicleLighting::get_front_headlight_upper_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_FRONT, LIGHT_TYPE_HEADLIGHT_UPPER) : false;
    }

    bool MoverRailVehicleLighting::get_front_headlight_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_FRONT, LIGHT_TYPE_HEADLIGHT_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_front_headlight_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_FRONT, LIGHT_TYPE_HEADLIGHT_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_front_redmarker_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_FRONT, LIGHT_TYPE_REDMARKER_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_front_redmarker_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_FRONT, LIGHT_TYPE_REDMARKER_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_rear_headlight_upper_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_REAR, LIGHT_TYPE_HEADLIGHT_UPPER) : false;
    }

    bool MoverRailVehicleLighting::get_rear_headlight_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_REAR, LIGHT_TYPE_HEADLIGHT_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_rear_headlight_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_REAR, LIGHT_TYPE_HEADLIGHT_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_rear_redmarker_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_REAR, LIGHT_TYPE_REDMARKER_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_rear_redmarker_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, LIGHT_END_REAR, LIGHT_TYPE_REDMARKER_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_active_headlight_upper_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _active_end(mover), LIGHT_TYPE_HEADLIGHT_UPPER) : false;
    }

    bool MoverRailVehicleLighting::get_active_headlight_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _active_end(mover), LIGHT_TYPE_HEADLIGHT_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_active_headlight_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _active_end(mover), LIGHT_TYPE_HEADLIGHT_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_active_redmarker_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _active_end(mover), LIGHT_TYPE_REDMARKER_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_active_redmarker_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _active_end(mover), LIGHT_TYPE_REDMARKER_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_opposite_headlight_upper_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _opposite_end(mover), LIGHT_TYPE_HEADLIGHT_UPPER) : false;
    }

    bool MoverRailVehicleLighting::get_opposite_headlight_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _opposite_end(mover), LIGHT_TYPE_HEADLIGHT_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_opposite_headlight_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _opposite_end(mover), LIGHT_TYPE_HEADLIGHT_RIGHT) : false;
    }

    bool MoverRailVehicleLighting::get_opposite_redmarker_left_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _opposite_end(mover), LIGHT_TYPE_REDMARKER_LEFT) : false;
    }

    bool MoverRailVehicleLighting::get_opposite_redmarker_right_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? _light_enabled(mover, _opposite_end(mover), LIGHT_TYPE_REDMARKER_RIGHT) : false;
    }

    void MoverRailVehicleLighting::_fill_state_dictionary(Dictionary &p_state) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_state["light_position"] = get_position();
        // what lights_sw shows - LightsPos runs from 1 (Train.cpp:5220)
        p_state["light_selector_position"] = std::max(0, get_position() - 1);
        p_state["headlights_dimmed"] = get_headlights_dimmed();
        p_state["any_light_enabled"] = get_any_light_enabled();
        p_state["light_power"] = get_power();
        p_state["light_power_source"] = get_power_source();
        p_state["lights/front_headlight_upper_enabled"] = get_front_headlight_upper_enabled();
        p_state["lights/front_headlight_left_enabled"] = get_front_headlight_left_enabled();
        p_state["lights/front_headlight_right_enabled"] = get_front_headlight_right_enabled();
        p_state["lights/front_redmarker_left_enabled"] = get_front_redmarker_left_enabled();
        p_state["lights/front_redmarker_right_enabled"] = get_front_redmarker_right_enabled();
        p_state["lights/rear_headlight_upper_enabled"] = get_rear_headlight_upper_enabled();
        p_state["lights/rear_headlight_left_enabled"] = get_rear_headlight_left_enabled();
        p_state["lights/rear_headlight_right_enabled"] = get_rear_headlight_right_enabled();
        p_state["lights/rear_redmarker_left_enabled"] = get_rear_redmarker_left_enabled();
        p_state["lights/rear_redmarker_right_enabled"] = get_rear_redmarker_right_enabled();
        p_state["lights/active_headlight_upper_enabled"] = get_active_headlight_upper_enabled();
        p_state["lights/active_headlight_left_enabled"] = get_active_headlight_left_enabled();
        p_state["lights/active_headlight_right_enabled"] = get_active_headlight_right_enabled();
        p_state["lights/active_redmarker_left_enabled"] = get_active_redmarker_left_enabled();
        p_state["lights/active_redmarker_right_enabled"] = get_active_redmarker_right_enabled();
        p_state["lights/opposite_headlight_upper_enabled"] = get_opposite_headlight_upper_enabled();
        p_state["lights/opposite_headlight_left_enabled"] = get_opposite_headlight_left_enabled();
        p_state["lights/opposite_headlight_right_enabled"] = get_opposite_headlight_right_enabled();
        p_state["lights/opposite_redmarker_left_enabled"] = get_opposite_redmarker_left_enabled();
        p_state["lights/opposite_redmarker_right_enabled"] = get_opposite_redmarker_right_enabled();
    }

    void MoverRailVehicleLighting::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        VehicleComponent::_fill_config_dictionary(p_config);
        // lights_sw: shows LightsPos - 1 (Train.cpp:5220), one position per preset
        p_config["light_position_max"] = std::max(0, mover->LightsPosNo - 1);
    }

    // Original engine: TTrain::OnCommand_lightspresetactivatenext (Train.cpp:5193) - LightsPos runs
    // 1..LightsPosNo, and wraps around only with LightsWrap
    void MoverRailVehicleLighting::increase_light_selector_position() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (mover->LightsPosNo == 0) {
            return;
        }
        if (mover->LightsPos < mover->LightsPosNo || mover->LightsWrap) {
            mover->LightsPos = mover->LightsPos >= mover->LightsPosNo ? 1 : mover->LightsPos + 1;
            _set_lights(mover);
        }
    }

    // Original engine: TTrain::OnCommand_lightspresetactivateprevious (Train.cpp:5231)
    void MoverRailVehicleLighting::decrease_light_selector_position() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (mover->LightsPosNo == 0) {
            return;
        }
        if (mover->LightsPos > 1 || mover->LightsWrap) {
            mover->LightsPos = mover->LightsPos <= 1 ? mover->LightsPosNo : mover->LightsPos - 1;
            _set_lights(mover);
        }
    }

    /* Original engine: TDynamicObject::SetLights (DynObj.cpp:7322) with the iLights part of
     * RaLightsSet (DynObj.cpp:7350). The preset of the occupied cab lights every vehicle joined to
     * it by a control coupling: the head preset on the end facing the head of the train, the rear
     * one on the other, and nothing on an end coupled to another vehicle. The model's own lamp
     * inventory (iInventory) is not known here, so a rear end that could show either red markers
     * or plates shows the markers. */
    void MoverRailVehicleLighting::_set_lights(TMoverParameters *p_mover) const {
        const int lead_end = p_mover->CabOccupied >= 0 ? Maszyna::end::front : Maszyna::end::rear;
        const int preset = p_mover->LightsPos - 1;
        const int automatic_markers =
                p_mover->CabActive == 0 && (p_mover->InactiveCabFlag & Maszyna::activation::redmarkers) != 0
                        ? Maszyna::light::redmarker_left + Maszyna::light::redmarker_right
                        : 0;
        const int head_lights = automatic_markers > 0 ? automatic_markers : p_mover->Lights[lead_end][preset];
        const int rear_lights = automatic_markers > 0 ? automatic_markers : p_mover->Lights[1 - lead_end][preset];
        const int end_of_train =
                Maszyna::light::redmarker_left | Maszyna::light::redmarker_right | Maszyna::light::rearendsignals;

        // GetFirstDynamic(): out through the head end to the last vehicle joined by control
        TMoverParameters *vehicle = p_mover;
        int head_end = lead_end;
        while (vehicle->Couplers[head_end].Connected != nullptr &&
               Maszyna::TestFlag(vehicle->Couplers[head_end].CouplingFlag, Maszyna::coupling::control)) {
            const int entered = vehicle->Couplers[head_end].ConnectedNr;
            vehicle = vehicle->Couplers[head_end].Connected;
            head_end = 1 - entered;
        }
        while (vehicle != nullptr) {
            const int tail_end = 1 - head_end;
            // Original engine: LightsPos 18 lights both ends whatever is coupled (DynObj.cpp:7337)
            const bool lights_all_ends = p_mover->LightsPos == LIGHTS_POSITION_ALL_ENDS;
            const bool head_free =
                    lights_all_ends || vehicle->Couplers[head_end].Connected == nullptr ||
                    !Maszyna::TestFlag(vehicle->Couplers[head_end].CouplingFlag, Maszyna::coupling::coupler);
            const bool tail_free =
                    lights_all_ends || vehicle->Couplers[tail_end].Connected == nullptr ||
                    !Maszyna::TestFlag(vehicle->Couplers[tail_end].CouplingFlag, Maszyna::coupling::coupler);
            int rear = tail_free ? rear_lights : 0;
            // a powered vehicle with no direction set shows plates, not lights (DynObj.cpp:7360)
            if (rear == end_of_train && vehicle->Power > 1.0 && vehicle->DirActive == 0) {
                rear = Maszyna::light::rearendsignals;
            }
            if (rear == end_of_train) {
                rear = Maszyna::light::redmarker_left | Maszyna::light::redmarker_right;
            }
            vehicle->iLights[head_end] = head_free ? head_lights : 0;
            vehicle->iLights[tail_end] = rear;
            if (vehicle->Couplers[tail_end].Connected == nullptr ||
                !Maszyna::TestFlag(vehicle->Couplers[tail_end].CouplingFlag, Maszyna::coupling::control)) {
                break;
            }
            const int entered = vehicle->Couplers[tail_end].ConnectedNr;
            vehicle = vehicle->Couplers[tail_end].Connected;
            head_end = entered;
        }
    }

    // Original engine: TTrain::OnCommand_headlightsdimenable/disable (Train.cpp:6146-6195)
    void MoverRailVehicleLighting::headlights_dim(const bool p_enabled) {
        headlights_dimmed = p_enabled;
    }

    bool MoverRailVehicleLighting::get_headlights_dimmed() const {
        return headlights_dimmed;
    }

    bool MoverRailVehicleLighting::get_any_light_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr &&
               (mover->iLights[Maszyna::end::front] != 0 || mover->iLights[Maszyna::end::rear] != 0);
    }

    namespace {
        struct LightBit {
                const char *name;
                MoverRailVehicleLighting::LightEnd end;
                MoverRailVehicleLighting::LightType type;
        };

        const LightBit LIGHT_BITS[] = {
                {"front_headlight_upper", MoverRailVehicleLighting::LIGHT_END_FRONT,
                 MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_UPPER},
                {"front_headlight_left", MoverRailVehicleLighting::LIGHT_END_FRONT,
                 MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_LEFT},
                {"front_headlight_right", MoverRailVehicleLighting::LIGHT_END_FRONT,
                 MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_RIGHT},
                {"front_redmarker_left", MoverRailVehicleLighting::LIGHT_END_FRONT,
                 MoverRailVehicleLighting::LIGHT_TYPE_REDMARKER_LEFT},
                {"front_redmarker_right", MoverRailVehicleLighting::LIGHT_END_FRONT,
                 MoverRailVehicleLighting::LIGHT_TYPE_REDMARKER_RIGHT},
                {"rear_headlight_upper", MoverRailVehicleLighting::LIGHT_END_REAR,
                 MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_UPPER},
                {"rear_headlight_left", MoverRailVehicleLighting::LIGHT_END_REAR,
                 MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_LEFT},
                {"rear_headlight_right", MoverRailVehicleLighting::LIGHT_END_REAR,
                 MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_RIGHT},
                {"rear_redmarker_left", MoverRailVehicleLighting::LIGHT_END_REAR,
                 MoverRailVehicleLighting::LIGHT_TYPE_REDMARKER_LEFT},
                {"rear_redmarker_right", MoverRailVehicleLighting::LIGHT_END_REAR,
                 MoverRailVehicleLighting::LIGHT_TYPE_REDMARKER_RIGHT},
        };
    } // namespace

    void MoverRailVehicleLighting::light(const String &p_light, const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);

        for (const LightBit &bit: LIGHT_BITS) {
            if (p_light == bit.name) {
                const int end = static_cast<int>(light_end_map.at(bit.end));
                const int mask = light_type_mask_map.at(bit.type);
                if (p_enabled) {
                    mover->iLights[end] |= mask;
                } else {
                    mover->iLights[end] &= ~mask;
                }
                return;
            }
        }
        UtilityFunctions::push_warning("MoverRailVehicleLighting::light() unknown light name: " + p_light);
    }

    bool MoverRailVehicleLighting::light_is_enabled(const String &p_light) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return false;
        }
        for (const LightBit &bit: LIGHT_BITS) {
            if (p_light == bit.name) {
                return _light_enabled(mover, bit.end, bit.type);
            }
        }
        return false;
    }

    namespace {
        struct LightSwitchMask {
                const char *suffix;
                MoverRailVehicleLighting::LightType type;
        };

        const LightSwitchMask LIGHT_SWITCH_MASKS[] = {
                {"upper", MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_UPPER},
                {"left", MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_LEFT},
                {"right", MoverRailVehicleLighting::LIGHT_TYPE_HEADLIGHT_RIGHT},
                {"leftend", MoverRailVehicleLighting::LIGHT_TYPE_REDMARKER_LEFT},
                {"rightend", MoverRailVehicleLighting::LIGHT_TYPE_REDMARKER_RIGHT},
        };
    } // namespace

    // Confirmed against vehicle/Train.cpp:5267-5316 (OnCommand_headlighttoggleleft/enableleft) -
    // upperlight_sw:/leftlight_sw:/rightlight_sw:/leftend_sw:/rightend_sw: (p_light without a
    // "rear" prefix) toggle the end of the cab the driver sits in (Train->cab_to_end(),
    // _active_end()); rearupperlight_sw:/etc. (p_light with a "rear" prefix, stripped
    // here) toggle the opposite end - see this class's own _do_fetch_state_from_mover() for the
    // matching active_end/opposite_end state this mirrors. NOTE: a real nuance from the original
    // is deliberately NOT reproduced here - OnCommand_headlightenableleft also clears the
    // matching redmarker when the vehicle has no separate *end_sw: declared (a 3-way switch
    // subsuming the marker light), which would require cross-widget awareness this catalog-driven
    // instancer doesn't have.
    void MoverRailVehicleLighting::light_switch(const String &p_light, const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        // a vehicle with a light preset selector (LightsList) sets its lights through it alone -
        // "lights are controlled by preset selector" (Train.cpp:2922-3135, 5300-5519)
        if (mover->LightsPosNo > 0) {
            return;
        }

        const bool is_rear = p_light.begins_with("rear");
        const String suffix = is_rear ? p_light.substr(4) : p_light;
        const LightEnd target_end = is_rear ? _opposite_end(mover) : _active_end(mover);

        for (const LightSwitchMask &entry: LIGHT_SWITCH_MASKS) {
            if (suffix == entry.suffix) {
                const int end = static_cast<int>(light_end_map.at(target_end));
                const int mask = light_type_mask_map.at(entry.type);
                if (p_enabled) {
                    mover->iLights[end] |= mask;
                } else {
                    mover->iLights[end] &= ~mask;
                }
                return;
            }
        }
        UtilityFunctions::push_warning("MoverRailVehicleLighting::light_switch() unknown light name: " + p_light);
    }

} // namespace godot
