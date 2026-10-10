#include "RailVehicleSwitches.hpp"

namespace godot {
    void RailVehicleSwitches::_bind_methods() {
        BIND_ENUM_CONSTANT(RELAY_RESET_BUTTON_1);
        BIND_ENUM_CONSTANT(RELAY_RESET_BUTTON_2);
        BIND_ENUM_CONSTANT(RELAY_RESET_BUTTON_3);
        BIND_ENUM_CONSTANT(PANTOGRAPH_PRESET_NONE);
        BIND_ENUM_CONSTANT(PANTOGRAPH_PRESET_OWN_END);
        BIND_ENUM_CONSTANT(PANTOGRAPH_PRESET_OTHER_END);
        BIND_ENUM_CONSTANT(PANTOGRAPH_PRESET_BOTH);
        BIND_PROPERTY(RailVehicleSwitches, Variant::BOOL, pantograph_impulse);
        BIND_PROPERTY(RailVehicleSwitches, Variant::BOOL, converter_impulse);
        BIND_PROPERTY(RailVehicleSwitches, Variant::BOOL, motor_connectors_impulse);
        BIND_PROPERTY_W_HINT(
                RailVehicleSwitches, Variant::INT, relay_reset_button_1, "relay_reset_button", PROPERTY_HINT_FLAGS,
                "Main Circuit Diff,Aux Circuit Diff,Traction Motor Overload,Main Converter Overload,"
                "Aux Converter Overload,Fan Overload,Heating Overload,ED Brake Overload");
        BIND_PROPERTY_W_HINT(
                RailVehicleSwitches, Variant::INT, relay_reset_button_2, "relay_reset_button", PROPERTY_HINT_FLAGS,
                "Main Circuit Diff,Aux Circuit Diff,Traction Motor Overload,Main Converter Overload,"
                "Aux Converter Overload,Fan Overload,Heating Overload,ED Brake Overload");
        BIND_PROPERTY_W_HINT(
                RailVehicleSwitches, Variant::INT, relay_reset_button_3, "relay_reset_button", PROPERTY_HINT_FLAGS,
                "Main Circuit Diff,Aux Circuit Diff,Traction Motor Overload,Main Converter Overload,"
                "Aux Converter Overload,Fan Overload,Heating Overload,ED Brake Overload");
        BIND_PROPERTY(RailVehicleSwitches, Variant::PACKED_INT32_ARRAY, pantograph_presets);
        BIND_PROPERTY(RailVehicleSwitches, Variant::INT, pantograph_preset_default);
        BIND_PROPERTY(RailVehicleSwitches, Variant::BOOL, modern_dimmer);
        BIND_PROPERTY(RailVehicleSwitches, Variant::BOOL, dimmer_list_cycle, "dimmer_list_positions");
        BIND_PROPERTY(RailVehicleSwitches, Variant::INT, dimmer_list_default_position, "dimmer_list_positions");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleSwitches, Variant::ARRAY, dimmer_list_positions, "dimmer_list_positions",
                PROPERTY_HINT_TYPE_STRING, "RailVehicleDimmerListItem");
        ClassDB::bind_method(D_METHOD("sand", "active"), &RailVehicleSwitches::sand);
        ClassDB::bind_method(D_METHOD("universal_relay_reset", "button"), &RailVehicleSwitches::universal_relay_reset);
        ClassDB::bind_method(D_METHOD("next_pantograph_preset", "end"), &RailVehicleSwitches::next_pantograph_preset);
        ClassDB::bind_method(
                D_METHOD("previous_pantograph_preset", "end"), &RailVehicleSwitches::previous_pantograph_preset);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_preset_position", "end"),
                &RailVehicleSwitches::get_pantograph_preset_position);
        ClassDB::bind_method(D_METHOD("get_pantograph_preset", "end"), &RailVehicleSwitches::get_pantograph_preset);

        ClassDB::bind_method(D_METHOD("get_sand_active"), &RailVehicleSwitches::get_sand_active);
    }

    void RailVehicleSwitches::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("sand", Callable(this, "sand"));
        register_command("universal_relay_reset", Callable(this, "universal_relay_reset"));
        register_command("pantograph_next_preset", Callable(this, "next_pantograph_preset"));
        register_command("pantograph_previous_preset", Callable(this, "previous_pantograph_preset"));
    }

    void RailVehicleSwitches::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("sand");
        unregister_command("universal_relay_reset");
        unregister_command("pantograph_next_preset");
        unregister_command("pantograph_previous_preset");
    }
    // how the cab operates the pantographs (PantSwitchType, Train.cpp:3175, 3285)
    void RailVehicleSwitches::_fill_config_dictionary(Dictionary &p_config) const {
        p_config["pantograph_switch_impulse"] = get_pantograph_impulse();
        // ConvSwitchType (Train.cpp:4387, 4419)
        p_config["converter_switch_impulse"] = get_converter_impulse();
        // StLinSwitchType (Train.cpp:5045)
        p_config["motor_connectors_switch_impulse"] = get_motor_connectors_impulse();
    }
} // namespace godot
