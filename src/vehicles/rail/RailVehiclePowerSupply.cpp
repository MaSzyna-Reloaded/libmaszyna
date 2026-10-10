#include "RailVehiclePowerSupply.hpp"

namespace godot {
    const char *RailVehiclePowerSupply::power_changed_signal = "power_changed";

    void RailVehiclePowerSupply::_bind_methods() {
        BIND_PROPERTY_W_HINT(RailVehiclePowerSupply, Variant::FLOAT, battery_voltage, PROPERTY_HINT_RANGE, "0,500,1");
        BIND_PROPERTY_W_HINT(
                RailVehiclePowerSupply, Variant::INT, cntrl_battery_start_mode, "cntrl", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                RailVehiclePowerSupply, Variant::INT, cntrl_converter_start_mode, "cntrl", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(RailVehiclePowerSupply, Variant::FLOAT, cntrl_converter_start_delay, "cntrl");

        ClassDB::bind_method(D_METHOD("battery", "enabled"), &RailVehiclePowerSupply::battery);
        ClassDB::bind_method(D_METHOD("converter", "enabled"), &RailVehiclePowerSupply::converter);

        ClassDB::bind_method(D_METHOD("get_live_battery_voltage"), &RailVehiclePowerSupply::get_live_battery_voltage);
        ClassDB::bind_method(D_METHOD("get_battery_enabled"), &RailVehiclePowerSupply::get_battery_enabled);
        ClassDB::bind_method(D_METHOD("get_converter_enabled"), &RailVehiclePowerSupply::get_converter_enabled);
        ClassDB::bind_method(D_METHOD("get_converter_allowed"), &RailVehiclePowerSupply::get_converter_allowed);
        ClassDB::bind_method(
                D_METHOD("get_converter_time_to_start"), &RailVehiclePowerSupply::get_converter_time_to_start);
        ClassDB::bind_method(D_METHOD("get_power24_voltage"), &RailVehiclePowerSupply::get_power24_voltage);
        ClassDB::bind_method(D_METHOD("get_power24_available"), &RailVehiclePowerSupply::get_power24_available);
        ClassDB::bind_method(D_METHOD("get_power110_available"), &RailVehiclePowerSupply::get_power110_available);

        ADD_SIGNAL(MethodInfo(power_changed_signal, PropertyInfo(Variant::BOOL, "is_powered")));
    }

    void RailVehiclePowerSupply::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("battery", Callable(this, "battery"));
        register_command("converter", Callable(this, "converter"));
    }

    void RailVehiclePowerSupply::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("battery");
        unregister_command("converter");
    }

    void RailVehiclePowerSupply::_do_process_component(const double p_delta) {
        if (const bool powered = get_power24_available() || get_power110_available(); powered != previous_powered) {
            previous_powered = powered;
            emit_signal(power_changed_signal, powered);
        }
    }

    void RailVehiclePowerSupply::_fill_state_dictionary(Dictionary &p_state) const {
        if (!is_simulation_ready()) {
            return;
        }
        p_state["battery_enabled"] = get_battery_enabled();
        p_state["battery_voltage"] = get_live_battery_voltage();
        p_state["converter_enabled"] = get_converter_enabled();
        p_state["converter_allowed"] = get_converter_allowed();
        p_state["converter_time_to_start"] = get_converter_time_to_start();
        p_state["power24_voltage"] = get_power24_voltage();
        p_state["power24_available"] = get_power24_available();
        p_state["power110_available"] = get_power110_available();
    }
} // namespace godot
