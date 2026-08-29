#include "TrainElectricEngine.hpp"
#include "macros.hpp"

#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>

namespace godot {
    void TrainElectricEngine::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                Variant::INT, "engine_power_source", "power/source", &TrainElectricEngine::set_engine_power_source,
                &TrainElectricEngine::get_engine_power_source, "power_source", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY(
                Variant::INT, "number_of_collectors", "power/current_collector/number_of_collectors",
                &TrainElectricEngine::set_collectors_no, &TrainElectricEngine::get_collectors_no,
                "number_of_collectors");
        BIND_PROPERTY(
                Variant::FLOAT, "max_voltage", "power/current_collector/max_voltage",
                &TrainElectricEngine::set_max_voltage, &TrainElectricEngine::get_max_voltage, "max_voltage");
        BIND_PROPERTY(
                Variant::FLOAT, "max_current", "power/current_collector/max_current",
                &TrainElectricEngine::set_max_current, &TrainElectricEngine::get_max_current, "max_current");
        BIND_PROPERTY(
                Variant::FLOAT, "max_collector_lifting", "power/current_collector/max_collector_lifting",
                &TrainElectricEngine::set_max_collector_lifting, &TrainElectricEngine::get_max_collector_lifting,
                "max_collector_lifting");
        BIND_PROPERTY(
                Variant::FLOAT, "min_collector_lifting", "power/current_collector/min_collector_lifting",
                &TrainElectricEngine::set_min_collector_lifting, &TrainElectricEngine::get_min_collector_lifting,
                "min_collector_lifting");
        BIND_PROPERTY(
                Variant::FLOAT, "collector_sliding_width", "power/current_collector/collector_sliding_width",
                &TrainElectricEngine::set_collector_sliding_width, &TrainElectricEngine::get_collector_sliding_width,
                "collector_sliding_width");
        BIND_PROPERTY(
                Variant::FLOAT, "min_main_switch_voltage", "power/current_collector/min_main_switch_voltage",
                &TrainElectricEngine::set_min_main_switch_voltage, &TrainElectricEngine::get_min_main_switch_voltage,
                "min_main_switch_voltage");
        BIND_PROPERTY(
                Variant::FLOAT, "min_pantograph_tank_pressure", "power/current_collector/min_pantograph_tank_pressure",
                &TrainElectricEngine::set_min_pantograph_tank_pressure,
                &TrainElectricEngine::get_min_pantograph_tank_pressure, "min_pantograph_tank_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "max_pantograph_tank_pressure", "power/current_collector/max_pantograph_tank_pressure",
                &TrainElectricEngine::set_max_pantograph_tank_pressure,
                &TrainElectricEngine::get_max_pantograph_tank_pressure, "max_pantograph_tank_pressure");
        BIND_PROPERTY(
                Variant::BOOL, "overvoltage_relay", "power/current_collector/overvoltage_relay",
                &TrainElectricEngine::set_overvoltage_relay, &TrainElectricEngine::get_overvoltage_relay,
                "overvoltage_relay");
        BIND_PROPERTY(
                Variant::FLOAT, "required_main_switch_voltage", "power/current_collector/required_main_switch_voltage",
                &TrainElectricEngine::set_required_main_switch_voltage,
                &TrainElectricEngine::get_required_main_switch_voltage, "required_main_switch_voltage");
        BIND_PROPERTY(
                Variant::FLOAT, "transducer_input_voltage", "power/transducer/input_voltage",
                &TrainElectricEngine::set_transducer_input_voltage, &TrainElectricEngine::get_transducer_input_voltage,
                "transducer_input_voltage");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "accumulator_recharge_source", "power/accumulator/recharge_source",
                &TrainElectricEngine::set_accumulator_recharge_source,
                &TrainElectricEngine::get_accumulator_recharge_source, "accumulator_recharge_source",
                PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "power_cable_power_source", "power/power_cable/source",
                &TrainElectricEngine::set_power_cable_power_source, &TrainElectricEngine::get_power_cable_power_source,
                "power_cable_power_source", PROPERTY_HINT_ENUM,
                "NoPower,BioPower,MechPower,ElectricPower,SteamPower,Main");
        BIND_PROPERTY(
                Variant::FLOAT, "power_cable_steam_pressure", "power/power_cable/steam_pressure",
                &TrainElectricEngine::set_power_cable_steam_pressure,
                &TrainElectricEngine::get_power_cable_steam_pressure, "power_cable_steam_pressure");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "physical_layout", "power/current_collector/physical_layout",
                &TrainElectricEngine::set_physical_layout, &TrainElectricEngine::get_physical_layout,
                "physical_layout", PROPERTY_HINT_FLAGS, "Front,Rear");
        BIND_PROPERTY(
                Variant::FLOAT, "circuit_resistance", "circuit/resistance", &TrainElectricEngine::set_circuit_resistance,
                &TrainElectricEngine::get_circuit_resistance, "circuit_resistance");
        BIND_PROPERTY(
                Variant::INT, "imax_low", "circuit/imax_low", &TrainElectricEngine::set_imax_low,
                &TrainElectricEngine::get_imax_low, "imax_low");
        BIND_PROPERTY(
                Variant::INT, "imax_high", "circuit/imax_high", &TrainElectricEngine::set_imax_high,
                &TrainElectricEngine::get_imax_high, "imax_high");
        BIND_PROPERTY(
                Variant::INT, "imin_low", "circuit/imin_low", &TrainElectricEngine::set_imin_low,
                &TrainElectricEngine::get_imin_low, "imin_low");
        BIND_PROPERTY(
                Variant::INT, "imin_high", "circuit/imin_high", &TrainElectricEngine::set_imin_high,
                &TrainElectricEngine::get_imin_high, "imin_high");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_sum", "circuit/tuhex/sum", &TrainElectricEngine::set_tuhex_sum,
                &TrainElectricEngine::get_tuhex_sum, "tuhex_sum");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_diff", "circuit/tuhex/diff", &TrainElectricEngine::set_tuhex_diff,
                &TrainElectricEngine::get_tuhex_diff, "tuhex_diff");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_min_current", "circuit/tuhex/min_current",
                &TrainElectricEngine::set_tuhex_min_current, &TrainElectricEngine::get_tuhex_min_current,
                "tuhex_min_current");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_max_current", "circuit/tuhex/max_current",
                &TrainElectricEngine::set_tuhex_max_current, &TrainElectricEngine::get_tuhex_max_current,
                "tuhex_max_current");
        BIND_PROPERTY(
                Variant::INT, "tuhex_stages", "circuit/tuhex/stages", &TrainElectricEngine::set_tuhex_stages,
                &TrainElectricEngine::get_tuhex_stages, "tuhex_stages");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_sum_1", "circuit/tuhex/sum_1", &TrainElectricEngine::set_tuhex_sum_1,
                &TrainElectricEngine::get_tuhex_sum_1, "tuhex_sum_1");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_sum_2", "circuit/tuhex/sum_2", &TrainElectricEngine::set_tuhex_sum_2,
                &TrainElectricEngine::get_tuhex_sum_2, "tuhex_sum_2");
        BIND_PROPERTY(
                Variant::FLOAT, "tuhex_sum_3", "circuit/tuhex/sum_3", &TrainElectricEngine::set_tuhex_sum_3,
                &TrainElectricEngine::get_tuhex_sum_3, "tuhex_sum_3");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "converter_start_mode", "cntrl/converter_start_mode",
                &TrainElectricEngine::set_converter_start_mode, &TrainElectricEngine::get_converter_start_mode,
                "converter_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                Variant::FLOAT, "converter_start_delay", "cntrl/converter_start_delay",
                &TrainElectricEngine::set_converter_start_delay, &TrainElectricEngine::get_converter_start_delay,
                "converter_start_delay");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "converter_overload_relay_start_mode", "cntrl/converter_overload_relay_start_mode",
                &TrainElectricEngine::set_converter_overload_relay_start_mode,
                &TrainElectricEngine::get_converter_overload_relay_start_mode, "converter_overload_relay_start_mode",
                PROPERTY_HINT_ENUM, "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                Variant::BOOL, "converter_overload_relay_off_when_main_is_off",
                "cntrl/converter_overload_relay_off_when_main_is_off",
                &TrainElectricEngine::set_converter_overload_relay_off_when_main_is_off,
                &TrainElectricEngine::get_converter_overload_relay_off_when_main_is_off,
                "converter_overload_relay_off_when_main_is_off");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "pantograph_compressor_start_mode", "cntrl/pantograph_compressor_start_mode",
                &TrainElectricEngine::set_pantograph_compressor_start_mode,
                &TrainElectricEngine::get_pantograph_compressor_start_mode, "pantograph_compressor_start_mode",
                PROPERTY_HINT_ENUM, "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                Variant::BOOL, "pantograph_auto_valve", "cntrl/pantograph_auto_valve",
                &TrainElectricEngine::set_pantograph_auto_valve, &TrainElectricEngine::get_pantograph_auto_valve,
                "pantograph_auto_valve");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "main_switch_start_mode", "cntrl/main_switch_start_mode",
                &TrainElectricEngine::set_main_switch_start_mode, &TrainElectricEngine::get_main_switch_start_mode,
                "main_switch_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        ClassDB::bind_method(D_METHOD("compressor", "enabled"), &TrainElectricEngine::compressor);
        ClassDB::bind_method(D_METHOD("converter", "enabled"), &TrainElectricEngine::converter);
    }

    void TrainElectricEngine::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        TrainEngine::_do_fetch_state_from_mover(p_mover, p_state);
        p_state["compressor_enabled"] = p_mover->CompressorFlag;
        p_state["compressor_allowed"] = p_mover->CompressorAllow;
        p_state["converter_enabled"] = p_mover->ConverterFlag;
        p_state["converted_allowed"] = p_mover->ConverterAllow;
        p_state["converter_time_to_start"] = p_mover->ConverterStartDelayTimer;
        p_state["power_source"] = train_controller_node->tpower_source_map.at(p_mover->EnginePowerSource.SourceType);
        p_state["accumulator/recharge_source"] =
                train_controller_node->tpower_source_map.at(p_mover->EnginePowerSource.RAccumulator.RechargeSource);
        p_state["current_collector/max_voltage"] = p_mover->EnginePowerSource.MaxVoltage;
        p_state["current_collector/max_current"] = p_mover->EnginePowerSource.MaxCurrent;
        p_state["current_collector/max_collector_lifting"] = p_mover->EnginePowerSource.CollectorParameters.MaxH;
        p_state["current_collector/min_collector_lifting"] = p_mover->EnginePowerSource.CollectorParameters.MinH;
        p_state["current_collector/collector_sliding_width"] = p_mover->EnginePowerSource.CollectorParameters.CSW;
        p_state["current_collector/min_main_switch_voltage"] = p_mover->EnginePowerSource.CollectorParameters.MinV;
        p_state["current_collector/min_pantograph_tank_pressure"] =
                p_mover->EnginePowerSource.CollectorParameters.MinPress;
        p_state["current_collector/max_pantograph_tank_pressure"] =
                p_mover->EnginePowerSource.CollectorParameters.MaxPress;
        p_state["current_collector/overvoltage_relay"] = p_mover->EnginePowerSource.CollectorParameters.OVP;
        p_state["current_collector/required_main_switch_voltage"] =
                p_mover->EnginePowerSource.CollectorParameters.InsetV;
        p_state["transducer/input_voltage"] = p_mover->EnginePowerSource.Transducer.InputVoltage;
        p_state["power_cable/source"] =
                train_controller_node->tpower_type_map.at(p_mover->EnginePowerSource.RPowerCable.PowerTrans);
        p_state["power_cable/steam_pressure"] = p_mover->EnginePowerSource.RPowerCable.SteamPressure;
    }

    void TrainElectricEngine::_do_update_internal_mover(TMoverParameters *p_mover) {
        TrainEngine::_do_update_internal_mover(p_mover);
        p_mover->EnginePowerSource.SourceType = train_controller_node->power_source_map.at(power_source);

        switch (power_source) {
            case TrainController::POWER_SOURCE_INTERNAL: {
                const std::map<TrainController::TrainPowerType, TPowerType>::const_iterator lookup =
                        train_controller_node->power_type_map.find(power_cable_power_source);
                p_mover->EnginePowerSource.PowerType =
                        lookup != train_controller_node->power_type_map.end() ? lookup->second : TPowerType::NoPower;
                break;
            }
            case TrainController::POWER_SOURCE_TRANSDUCER: {
                p_mover->EnginePowerSource.Transducer.InputVoltage = transducer_input_voltage;
                break;
            }
            case TrainController::POWER_SOURCE_GENERATOR: {
                engine_generator &generator_params{p_mover->EnginePowerSource.EngineGenerator};
                // GeneratorParams.engine_revolutions = &enrot; @TODO: Figure what the fuck is &enrot
                break;
            }
            case TrainController::POWER_SOURCE_ACCUMULATOR: {
                p_mover->EnginePowerSource.RAccumulator.RechargeSource =
                        train_controller_node->power_source_map.at(accumulator_recharge_source);
                break;
            }
            case TrainController::POWER_SOURCE_CURRENTCOLLECTOR: {
                p_mover->EnginePowerSource.CollectorParameters.MinH = min_collector_lifting;
                p_mover->EnginePowerSource.CollectorParameters.MaxH = max_collector_lifting;
                p_mover->EnginePowerSource.CollectorParameters.CSW = collector_sliding_width;
                p_mover->EnginePowerSource.CollectorParameters.MinV = min_main_switch_voltage;
                p_mover->EnginePowerSource.CollectorParameters.MinPress = min_pantograph_tank_pressure;
                p_mover->EnginePowerSource.CollectorParameters.MaxPress = max_pantograph_tank_pressure;
                p_mover->EnginePowerSource.CollectorParameters.OVP = overvoltage_relay;
                p_mover->EnginePowerSource.CollectorParameters.CollectorsNo = collectors_no;
                p_mover->EnginePowerSource.MaxVoltage = max_voltage;
                p_mover->EnginePowerSource.MaxCurrent = max_current;
                p_mover->EnginePowerSource.CollectorParameters.InsetV = required_main_switch_voltage;
                p_mover->EnginePowerSource.CollectorParameters.PhysicalLayout = physical_layout;
                break;
            }
            case TrainController::POWER_SOURCE_POWERCABLE: {
                p_mover->EnginePowerSource.RPowerCable.PowerTrans =
                        train_controller_node->power_type_map.at(power_cable_power_source);
                if (p_mover->EnginePowerSource.RPowerCable.PowerTrans == TPowerType::SteamPower) {
                    p_mover->EnginePowerSource.RPowerCable.SteamPressure = power_cable_steam_pressure;
                }
                break;
            }
            case TrainController::POWER_SOURCE_HEATER:; // Not finished on MaSzyna's side
            case TrainController::POWER_SOURCE_NOT_DEFINED:;
            default:;
        }

        /* Circuit: (elektryczny obwod napedowy), tylko pojazdy elektryczne i spalinowo-elektryczne */
        p_mover->CircuitRes = circuit_resistance;
        p_mover->ImaxLo = imax_low;
        p_mover->ImaxHi = imax_high;
        p_mover->IminLo = imin_low;
        p_mover->IminHi = imin_high;
        p_mover->TUHEX_Sum = tuhex_sum;
        p_mover->TUHEX_Diff = tuhex_diff;
        p_mover->TUHEX_MinIw = tuhex_min_current;
        p_mover->TUHEX_MaxIw = tuhex_max_current;
        p_mover->TUHEX_Stages = tuhex_stages;
        p_mover->TUHEX_Sum1 = tuhex_sum_1;
        p_mover->TUHEX_Sum2 = tuhex_sum_2;
        p_mover->TUHEX_Sum3 = tuhex_sum_3;

        p_mover->ConverterStart = start_mode_map.at(converter_start_mode);
        p_mover->ConverterStartDelay = static_cast<float>(converter_start_delay);
        p_mover->ConverterOverloadRelayStart = start_mode_map.at(converter_overload_relay_start_mode);
        p_mover->ConverterOverloadRelayOffWhenMainIsOff = converter_overload_relay_off_when_main_is_off;
        p_mover->PantographCompressorStart = start_mode_map.at(pantograph_compressor_start_mode);
        p_mover->PantAutoValve = pantograph_auto_valve;
        p_mover->MainsStart = start_mode_map.at(main_switch_start_mode);
    }

    void TrainElectricEngine::converter(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->ConverterSwitch(p_enabled);
    }

    void TrainElectricEngine::compressor(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->CompressorSwitch(p_enabled);
    }

    void TrainElectricEngine::_register_commands() {
        TrainEngine::_register_commands();
        register_command("converter", Callable(this, "converter"));
        register_command("compressor", Callable(this, "compressor"));
    }

    void TrainElectricEngine::_unregister_commands() {
        TrainEngine::_unregister_commands();
        unregister_command("converter", Callable(this, "converter"));
        unregister_command("compressor", Callable(this, "compressor"));
    }


    void TrainElectricEngine::set_engine_power_source(const TrainController::TrainPowerSource p_source) {
        power_source = p_source;
        dirty = true;
    }

    TrainController::TrainPowerSource TrainElectricEngine::get_engine_power_source() const {
        return power_source;
    }
} // namespace godot
