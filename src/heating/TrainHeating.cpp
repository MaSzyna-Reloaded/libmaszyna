#include "TrainHeating.hpp"
#include "../core/TrainController.hpp"

namespace godot {
    void TrainHeating::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                Variant::INT, "heating_source", "heating/source", &TrainHeating::set_heating_source,
                &TrainHeating::get_heating_source, "heating_source", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "generator_engine", "heating/generator/engine", &TrainHeating::set_generator_engine,
                &TrainHeating::get_generator_engine, "generator_engine", PROPERTY_HINT_ENUM,
                "None,Dumb,WheelsDriven,ElectricSeriesMotor,ElectricInductionMotor,DieselEngine,SteamEngine,"
                "DieselElectric,Main");
        BIND_PROPERTY(
                Variant::FLOAT, "generator_min_rpm", "heating/generator/min_rpm", &TrainHeating::set_generator_min_rpm,
                &TrainHeating::get_generator_min_rpm, "generator_min_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "generator_min_voltage", "heating/generator/min_voltage",
                &TrainHeating::set_generator_min_voltage, &TrainHeating::get_generator_min_voltage,
                "generator_min_voltage");
        BIND_PROPERTY(
                Variant::FLOAT, "generator_max_rpm", "heating/generator/max_rpm", &TrainHeating::set_generator_max_rpm,
                &TrainHeating::get_generator_max_rpm, "generator_max_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "generator_max_voltage", "heating/generator/max_voltage",
                &TrainHeating::set_generator_max_voltage, &TrainHeating::get_generator_max_voltage,
                "generator_max_voltage");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "power_cable_power_type", "heating/power_cable/type",
                &TrainHeating::set_power_cable_power_type, &TrainHeating::get_power_cable_power_type,
                "power_cable_power_type", PROPERTY_HINT_ENUM, "NoPower,BioPower,MechPower,ElectricPower,SteamPower");
        BIND_PROPERTY(
                Variant::FLOAT, "max_voltage", "heating/max_voltage", &TrainHeating::set_max_voltage,
                &TrainHeating::get_max_voltage, "max_voltage");
    }

    void TrainHeating::_do_update_internal_mover(TMoverParameters *p_mover) {
        ASSERT_MOVER(p_mover);
        TrainPart::_do_update_internal_mover(p_mover);

        p_mover->HeatingPowerSource.SourceType = train_controller_node->power_source_map.at(heating_source);
        p_mover->HeatingPowerSource.MaxVoltage = max_voltage;

        switch (heating_source) {
            case TrainController::POWER_SOURCE_GENERATOR: {
                p_mover->HeatingPowerSource.EngineGenerator.revolutions_min = generator_min_rpm / 60.0;
                p_mover->HeatingPowerSource.EngineGenerator.revolutions_max = generator_max_rpm / 60.0;
                p_mover->HeatingPowerSource.EngineGenerator.voltage_min = generator_min_voltage;
                p_mover->HeatingPowerSource.EngineGenerator.voltage_max = generator_max_voltage;
                break;
            }
            case TrainController::POWER_SOURCE_POWERCABLE: {
                p_mover->HeatingPowerSource.RPowerCable.PowerTrans =
                        train_controller_node->power_type_map.at(power_cable_power_type);
                break;
            }
            default:
                break;
        }
    }

    void TrainHeating::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        ASSERT_MOVER(p_mover);
        p_state["heating_enabled"] = p_mover->Heating;
        p_state["heating_power"] = p_mover->HeatingPower;
    }
} // namespace godot
