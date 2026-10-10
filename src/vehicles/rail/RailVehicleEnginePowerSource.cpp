#include "RailVehicleEnginePowerSource.hpp"

namespace godot {
    const char *RailVehicleEnginePowerSource::pantograph_up_signal = "pantograph_up";
    const char *RailVehicleEnginePowerSource::pantograph_down_signal = "pantograph_down";

    // Original engine: sPantUp plays when a pantograph's voltage rises from zero - it has
    // just touched the wire (DynObj.cpp:3881-3934) - and sPantDown when a raised pantograph
    // stops being active (DynObj.cpp:4007-4036). Detected once per tick against this component's
    // own members, never in a getter.
    void RailVehicleEnginePowerSource::_do_process_component(const double p_delta) {
        const bool live[2] = {
                get_collector_pantograph_first_voltage() > 0.0, get_collector_pantograph_second_voltage() > 0.0};
        const bool active[2] = {get_collector_pantograph_first_active(), get_collector_pantograph_second_active()};
        for (int selector = PANTOGRAPH_FIRST; selector <= PANTOGRAPH_SECOND; ++selector) {
            if (live[selector] && !previous_pantograph_live[selector]) {
                emit_signal(pantograph_up_signal, selector);
            }
            if (!active[selector] && previous_pantograph_active[selector]) {
                emit_signal(pantograph_down_signal, selector);
            }
            previous_pantograph_live[selector] = live[selector];
            previous_pantograph_active[selector] = active[selector];
        }
    }

    bool RailVehicleEnginePowerSource::has_accumulator() const {
        return get_source_type() == RailVehicleController::POWER_SOURCE_ACCUMULATOR;
    }

    bool RailVehicleEnginePowerSource::has_power_cable() const {
        return get_source_type() == RailVehicleController::POWER_SOURCE_POWERCABLE;
    }

    void RailVehicleEnginePowerSource::_bind_methods() {
        const char *const sources =
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main";
        const char *const start_modes = "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction";
        BIND_PROPERTY_W_HINT(RailVehicleEnginePowerSource, Variant::INT, source_type, PROPERTY_HINT_ENUM, sources);
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::INT, current_collector_number_of_collectors,
                "current_collector");
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_max_voltage, "current_collector");
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_max_current, "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_min_collector_lifting,
                "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_max_collector_lifting,
                "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_sliding_width, "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_min_main_switch_voltage,
                "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_min_pantograph_tank_pressure,
                "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_max_pantograph_tank_pressure,
                "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::BOOL, current_collector_overvoltage_relay, "current_collector");
        BIND_PROPERTY(
                RailVehicleEnginePowerSource, Variant::FLOAT, current_collector_required_main_switch_voltage,
                "current_collector");
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::BOOL, current_collector_fake_power, "current_collector");
        BIND_PROPERTY_W_HINT(
                RailVehicleEnginePowerSource, Variant::INT, current_collector_physical_layout, "current_collector",
                PROPERTY_HINT_FLAGS, "Front,Rear");
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::FLOAT, transducer_input_voltage, "transducer");
        BIND_PROPERTY_W_HINT(
                RailVehicleEnginePowerSource, Variant::INT, accumulator_recharge_source, "accumulator",
                PROPERTY_HINT_ENUM, sources);
        BIND_PROPERTY_W_HINT(
                RailVehicleEnginePowerSource, Variant::INT, power_cable_source, "power_cable", PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"NoPower", RailVehicleController::POWER_TYPE_NONE},
                         {"BioPower", RailVehicleController::POWER_TYPE_BIO},
                         {"MechPower", RailVehicleController::POWER_TYPE_MECH},
                         {"ElectricPower", RailVehicleController::POWER_TYPE_ELECTRIC},
                         {"SteamPower", RailVehicleController::POWER_TYPE_STEAM}}));
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::FLOAT, power_cable_steam_pressure, "power_cable");
        BIND_PROPERTY_W_HINT(
                RailVehicleEnginePowerSource, Variant::INT, cntrl_pantograph_compressor_start_mode, "cntrl",
                PROPERTY_HINT_ENUM, start_modes);
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::BOOL, cntrl_pantograph_auto_valve, "cntrl");
        BIND_PROPERTY_W_HINT(
                RailVehicleEnginePowerSource, Variant::INT, cntrl_pantographs_valve_start_mode, "cntrl",
                PROPERTY_HINT_ENUM, start_modes);
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::BOOL, cntrl_pantographs_valve_spring, "cntrl");
        BIND_PROPERTY_W_HINT(
                RailVehicleEnginePowerSource, Variant::INT, cntrl_pantograph_valve_start_mode, "cntrl",
                PROPERTY_HINT_ENUM, start_modes);
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::BOOL, cntrl_pantograph_valve_spring, "cntrl");
        BIND_PROPERTY(RailVehicleEnginePowerSource, Variant::BOOL, cntrl_pantograph_valve_solenoid, "cntrl");

        ClassDB::bind_method(D_METHOD("has_accumulator"), &RailVehicleEnginePowerSource::has_accumulator);
        ClassDB::bind_method(D_METHOD("has_power_cable"), &RailVehicleEnginePowerSource::has_power_cable);
        ClassDB::bind_method(
                D_METHOD("pantographs_valve", "enabled"), &RailVehicleEnginePowerSource::pantographs_valve);
        ClassDB::bind_method(
                D_METHOD("pantographs_valve_operate", "operation"),
                &RailVehicleEnginePowerSource::pantographs_valve_operate);
        ClassDB::bind_method(
                D_METHOD("pantographs_drop_all", "enabled"), &RailVehicleEnginePowerSource::pantographs_drop_all);
        ClassDB::bind_method(
                D_METHOD("pantograph_compressor", "enabled"), &RailVehicleEnginePowerSource::pantograph_compressor);
        ClassDB::bind_method(
                D_METHOD("pantograph_compressor_valve", "to_compressor"),
                &RailVehicleEnginePowerSource::pantograph_compressor_valve);
        ClassDB::bind_method(D_METHOD("pantograph", "selector", "enabled"), &RailVehicleEnginePowerSource::pantograph);
        ClassDB::bind_method(
                D_METHOD("pantograph_valve_operate", "selector", "operation"),
                &RailVehicleEnginePowerSource::pantograph_valve_operate);
        ClassDB::bind_method(
                D_METHOD("set_pantograph_wire_voltage", "selector", "voltage"),
                &RailVehicleEnginePowerSource::set_pantograph_wire_voltage);
        ClassDB::bind_method(
                D_METHOD("set_collector_voltage", "voltage"), &RailVehicleEnginePowerSource::set_collector_voltage);

        ClassDB::bind_method(
                D_METHOD("get_collector_max_voltage"), &RailVehicleEnginePowerSource::get_collector_max_voltage);
        ClassDB::bind_method(
                D_METHOD("get_collector_max_current"), &RailVehicleEnginePowerSource::get_collector_max_current);
        ClassDB::bind_method(
                D_METHOD("get_collector_max_lifting"), &RailVehicleEnginePowerSource::get_collector_max_lifting);
        ClassDB::bind_method(
                D_METHOD("get_collector_min_lifting"), &RailVehicleEnginePowerSource::get_collector_min_lifting);
        ClassDB::bind_method(
                D_METHOD("get_collector_sliding_width"), &RailVehicleEnginePowerSource::get_collector_sliding_width);
        ClassDB::bind_method(
                D_METHOD("get_collector_min_main_switch_voltage"),
                &RailVehicleEnginePowerSource::get_collector_min_main_switch_voltage);
        ClassDB::bind_method(
                D_METHOD("get_collector_min_pantograph_tank_pressure"),
                &RailVehicleEnginePowerSource::get_collector_min_pantograph_tank_pressure);
        ClassDB::bind_method(
                D_METHOD("get_collector_max_pantograph_tank_pressure"),
                &RailVehicleEnginePowerSource::get_collector_max_pantograph_tank_pressure);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_tank_pressure"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_tank_pressure);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_pressure_switch_armed"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_pressure_switch_armed);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_pressure_lock_active"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_pressure_lock_active);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_compressor_valve"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_compressor_valve);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_compressor_enabled"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_compressor_enabled);
        ClassDB::bind_method(
                D_METHOD("get_collector_overvoltage_relay"),
                &RailVehicleEnginePowerSource::get_collector_overvoltage_relay);
        ClassDB::bind_method(
                D_METHOD("get_collector_required_main_switch_voltage"),
                &RailVehicleEnginePowerSource::get_collector_required_main_switch_voltage);
        ClassDB::bind_method(
                D_METHOD("get_collector_valve_active"), &RailVehicleEnginePowerSource::get_collector_valve_active);
        ClassDB::bind_method(
                D_METHOD("get_collector_valve_enabled"), &RailVehicleEnginePowerSource::get_collector_valve_enabled);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantographs_dropped"),
                &RailVehicleEnginePowerSource::get_collector_pantographs_dropped);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_first_active"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_first_active);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_second_active"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_second_active);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_first_valve_enabled"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_first_valve_enabled);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_second_valve_enabled"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_second_valve_enabled);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_first_valve_active"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_first_valve_active);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_second_valve_active"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_second_valve_active);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_first_voltage"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_first_voltage);
        ClassDB::bind_method(
                D_METHOD("get_collector_pantograph_second_voltage"),
                &RailVehicleEnginePowerSource::get_collector_pantograph_second_voltage);
        ClassDB::bind_method(D_METHOD("get_collector_voltage"), &RailVehicleEnginePowerSource::get_collector_voltage);
        ClassDB::bind_method(
                D_METHOD("get_collector_trainset_high_voltage"),
                &RailVehicleEnginePowerSource::get_collector_trainset_high_voltage);
        ClassDB::bind_method(D_METHOD("get_energy_drawn"), &RailVehicleEnginePowerSource::get_energy_drawn);
        ClassDB::bind_method(D_METHOD("get_energy_returned"), &RailVehicleEnginePowerSource::get_energy_returned);

        ADD_SIGNAL(MethodInfo(pantograph_up_signal, PropertyInfo(Variant::INT, "selector")));
        ADD_SIGNAL(MethodInfo(pantograph_down_signal, PropertyInfo(Variant::INT, "selector")));

        BIND_ENUM_CONSTANT(PANTOGRAPH_FIRST);
        BIND_ENUM_CONSTANT(PANTOGRAPH_SECOND);
        BIND_ENUM_CONSTANT(PANTOGRAPH_TYPE_NONE);
        BIND_ENUM_CONSTANT(PANTOGRAPH_TYPE_AKP_4E);
        BIND_ENUM_CONSTANT(PANTOGRAPH_TYPE_DSAX);
        BIND_ENUM_CONSTANT(PANTOGRAPH_TYPE_EC160_200);
        BIND_ENUM_CONSTANT(PANTOGRAPH_TYPE_WBL85);
        ClassDB::bind_method(
                D_METHOD("set_current_collector_pantograph_type", "value"),
                &RailVehicleEnginePowerSource::set_current_collector_pantograph_type);
        ClassDB::bind_method(
                D_METHOD("get_current_collector_pantograph_type"),
                &RailVehicleEnginePowerSource::get_current_collector_pantograph_type);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "current_collector_pantograph_type", PROPERTY_HINT_ENUM,
                        "None,AKP_4E,DSAx,EC160_200,WBL85"),
                "set_current_collector_pantograph_type", "get_current_collector_pantograph_type");
        BIND_ENUM_CONSTANT(VALVE_OPERATION_NONE);
        BIND_ENUM_CONSTANT(VALVE_OPERATION_ENABLE);
        BIND_ENUM_CONSTANT(VALVE_OPERATION_DISABLE);
        BIND_ENUM_CONSTANT(VALVE_OPERATION_ENABLE_ON);
        BIND_ENUM_CONSTANT(VALVE_OPERATION_ENABLE_OFF);
        BIND_ENUM_CONSTANT(VALVE_OPERATION_DISABLE_ON);
        BIND_ENUM_CONSTANT(VALVE_OPERATION_DISABLE_OFF);
    }

    void RailVehicleEnginePowerSource::_register_commands() {
        VehicleComponent::_register_commands();
        register_command("pantographs_valve", Callable(this, "pantographs_valve"));
        register_command("pantographs_valve_operate", Callable(this, "pantographs_valve_operate"));
        register_command("pantographs_drop_all", Callable(this, "pantographs_drop_all"));
        register_command("pantograph_compressor", Callable(this, "pantograph_compressor"));
        register_command("pantograph_compressor_valve", Callable(this, "pantograph_compressor_valve"));
        register_command("pantograph", Callable(this, "pantograph"));
        register_command("pantograph_valve_operate", Callable(this, "pantograph_valve_operate"));
    }

    void RailVehicleEnginePowerSource::_unregister_commands() {
        VehicleComponent::_unregister_commands();
        unregister_command("pantographs_valve");
        unregister_command("pantographs_valve_operate");
        unregister_command("pantographs_drop_all");
        unregister_command("pantograph_compressor");
        unregister_command("pantograph_compressor_valve");
        unregister_command("pantograph");
        unregister_command("pantograph_valve_operate");
    }

    void RailVehicleEnginePowerSource::_fill_state_dictionary(Dictionary &p_state) const {
        if (!is_simulation_ready()) {
            return;
        }
        p_state["power_source"] = get_source_type();
        if (has_accumulator()) {
            p_state["accumulator/recharge_source"] = get_accumulator_recharge_source();
        }
        p_state["current_collector/max_voltage"] = get_collector_max_voltage();
        p_state["current_collector/max_current"] = get_collector_max_current();
        p_state["current_collector/max_collector_lifting"] = get_collector_max_lifting();
        p_state["current_collector/min_collector_lifting"] = get_collector_min_lifting();
        p_state["current_collector/collector_sliding_width"] = get_collector_sliding_width();
        p_state["current_collector/min_main_switch_voltage"] = get_collector_min_main_switch_voltage();
        p_state["current_collector/min_pantograph_tank_pressure"] = get_collector_min_pantograph_tank_pressure();
        p_state["current_collector/max_pantograph_tank_pressure"] = get_collector_max_pantograph_tank_pressure();
        p_state["current_collector/pantograph_tank_pressure"] = get_collector_pantograph_tank_pressure();
        p_state["current_collector/pantograph_pressure_switch_armed"] =
                get_collector_pantograph_pressure_switch_armed();
        p_state["current_collector/pantograph_pressure_lock_active"] = get_collector_pantograph_pressure_lock_active();
        p_state["current_collector/pantograph_compressor_valve"] = get_collector_pantograph_compressor_valve();
        p_state["current_collector/pantograph_compressor_enabled"] = get_collector_pantograph_compressor_enabled();
        p_state["current_collector/overvoltage_relay"] = get_collector_overvoltage_relay();
        p_state["current_collector/required_main_switch_voltage"] = get_collector_required_main_switch_voltage();
        p_state["current_collector/valve_active"] = get_collector_valve_active();
        p_state["current_collector/valve_enabled"] = get_collector_valve_enabled();
        p_state["current_collector/pantographs_dropped"] = get_collector_pantographs_dropped();
        p_state["current_collector/pantograph_first_active"] = get_collector_pantograph_first_active();
        p_state["current_collector/pantograph_first_valve_enabled"] = get_collector_pantograph_first_valve_enabled();
        p_state["current_collector/pantograph_second_valve_enabled"] = get_collector_pantograph_second_valve_enabled();
        p_state["pantograph_first_valve_active"] = get_collector_pantograph_first_valve_active();
        p_state["pantograph_second_valve_active"] = get_collector_pantograph_second_valve_active();
        p_state["current_collector/pantograph_first_voltage"] = get_collector_pantograph_first_voltage();
        p_state["current_collector/pantograph_second_active"] = get_collector_pantograph_second_active();
        p_state["current_collector/pantograph_second_voltage"] = get_collector_pantograph_second_voltage();
        p_state["current_collector/voltage"] = get_collector_voltage();
        p_state["current_collector/trainset_high_voltage"] = get_collector_trainset_high_voltage();
        // Train.cpp:810
        p_state["power_drawn"] = get_energy_drawn();
        p_state["power_returned"] = get_energy_returned();
        p_state["transducer/input_voltage"] = get_transducer_input_voltage();
        if (has_power_cable()) {
            p_state["power_cable/source"] = get_power_cable_source();
            p_state["power_cable/steam_pressure"] = get_power_cable_steam_pressure();
        }
    }
    void RailVehicleEnginePowerSource::set_current_collector_pantograph_type(const PantographType p_value) {
        current_collector_pantograph_type = p_value;
    }

    RailVehicleEnginePowerSource::PantographType
    RailVehicleEnginePowerSource::get_current_collector_pantograph_type() const {
        return current_collector_pantograph_type;
    }
} // namespace godot
