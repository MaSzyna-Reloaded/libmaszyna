#include "TrainSwitches.hpp"

namespace godot {
    void TrainSwitches::_bind_methods() {
        BIND_PROPERTY(
                Variant::BOOL, "pantograph_impulse", "pantograph_impulse", &TrainSwitches::set_pantograph_impulse,
                &TrainSwitches::get_pantograph_impulse, "pantograph_impulse");
        BIND_PROPERTY(
                Variant::BOOL, "converter_impulse", "converter_impulse", &TrainSwitches::set_converter_impulse,
                &TrainSwitches::get_converter_impulse, "converter_impulse");
        BIND_PROPERTY(
                Variant::BOOL, "motor_connectors_impulse", "motor_connectors_impulse",
                &TrainSwitches::set_motor_connectors_impulse, &TrainSwitches::get_motor_connectors_impulse,
                "motor_connectors_impulse");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "relay_reset_button_1", "relay_reset_button/1", &TrainSwitches::set_relay_reset_button_1,
                &TrainSwitches::get_relay_reset_button_1, "relay_reset_button_1", PROPERTY_HINT_FLAGS,
                "Main Circuit Diff,Aux Circuit Diff,Traction Motor Overload,Main Converter Overload,"
                "Aux Converter Overload,Fan Overload,Heating Overload,ED Brake Overload");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "relay_reset_button_2", "relay_reset_button/2", &TrainSwitches::set_relay_reset_button_2,
                &TrainSwitches::get_relay_reset_button_2, "relay_reset_button_2", PROPERTY_HINT_FLAGS,
                "Main Circuit Diff,Aux Circuit Diff,Traction Motor Overload,Main Converter Overload,"
                "Aux Converter Overload,Fan Overload,Heating Overload,ED Brake Overload");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "relay_reset_button_3", "relay_reset_button/3", &TrainSwitches::set_relay_reset_button_3,
                &TrainSwitches::get_relay_reset_button_3, "relay_reset_button_3", PROPERTY_HINT_FLAGS,
                "Main Circuit Diff,Aux Circuit Diff,Traction Motor Overload,Main Converter Overload,"
                "Aux Converter Overload,Fan Overload,Heating Overload,ED Brake Overload");
        BIND_PROPERTY(
                Variant::PACKED_INT32_ARRAY, "pantograph_presets", "pantograph_presets",
                &TrainSwitches::set_pantograph_presets, &TrainSwitches::get_pantograph_presets, "pantograph_presets");
        BIND_PROPERTY(
                Variant::INT, "pantograph_preset_default", "pantograph_preset_default",
                &TrainSwitches::set_pantograph_preset_default, &TrainSwitches::get_pantograph_preset_default,
                "pantograph_preset_default");
        BIND_PROPERTY(
                Variant::BOOL, "modern_dimmer", "modern_dimmer", &TrainSwitches::set_modern_dimmer,
                &TrainSwitches::get_modern_dimmer, "modern_dimmer");
        BIND_PROPERTY(
                Variant::BOOL, "dimmer_list_cycle", "dimmer_list/cycle", &TrainSwitches::set_dimmer_list_cycle,
                &TrainSwitches::get_dimmer_list_cycle, "dimmer_list_cycle");
        BIND_PROPERTY(
                Variant::INT, "dimmer_list_default_position", "dimmer_list/default_position",
                &TrainSwitches::set_dimmer_list_default_position, &TrainSwitches::get_dimmer_list_default_position,
                "dimmer_list_default_position");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "dimmer_list", "dimmer_list/positions", &TrainSwitches::set_dimmer_list,
                &TrainSwitches::get_dimmer_list, "dimmer_list", PROPERTY_HINT_TYPE_STRING, "DimmerListItem");
    }

    void TrainSwitches::_do_update_internal_mover(TMoverParameters *p_mover) {
        ASSERT_MOVER(p_mover);
        TrainPart::_do_update_internal_mover(p_mover);

        p_mover->PantSwitchType = pantograph_impulse ? "impulse" : "";
        p_mover->ConvSwitchType = converter_impulse ? "impulse" : "";
        p_mover->StLinSwitchType = motor_connectors_impulse ? "impulse" : "toggle";
    }

    void TrainSwitches::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        // RelayResetButtonX/PantographPresets/ModernDimmer/DimmerList are not wired to the
        // mover: see the class-level note in TrainSwitches.hpp.
    }
} // namespace godot
