#include "TrainElectricSeriesEngine.hpp"
#include "macros.hpp"

#include <algorithm>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainElectricSeriesEngine::_bind_methods() {
        BIND_PROPERTY(
                Variant::FLOAT, "nominal_voltage", "nominal_voltage", &TrainElectricSeriesEngine::set_nominal_voltage,
                &TrainElectricSeriesEngine::get_nominal_voltage, "nominal_voltage");
        BIND_PROPERTY(
                Variant::FLOAT, "winding_resistance", "winding_resistance",
                &TrainElectricSeriesEngine::set_winding_resistance, &TrainElectricSeriesEngine::get_winding_resistance,
                "winding_resistance");
        BIND_PROPERTY(
                Variant::FLOAT, "max_rpm", "max_rpm", &TrainElectricSeriesEngine::set_max_rpm,
                &TrainElectricSeriesEngine::get_max_rpm, "max_rpm");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "fan_type", "resistor_fan/type", &TrainElectricSeriesEngine::set_fan_type,
                &TrainElectricSeriesEngine::get_fan_type, "fan_type", PROPERTY_HINT_ENUM, "None,Yes,Automatic");
        BIND_PROPERTY(
                Variant::FLOAT, "fan_max_rpm", "resistor_fan/max_rpm", &TrainElectricSeriesEngine::set_fan_max_rpm,
                &TrainElectricSeriesEngine::get_fan_max_rpm, "fan_max_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "fan_cutoff_resistance", "resistor_fan/cutoff_resistance",
                &TrainElectricSeriesEngine::set_fan_cutoff_resistance,
                &TrainElectricSeriesEngine::get_fan_cutoff_resistance, "fan_cutoff_resistance");
        BIND_PROPERTY(
                Variant::FLOAT, "fan_min_current", "resistor_fan/min_current",
                &TrainElectricSeriesEngine::set_fan_min_current, &TrainElectricSeriesEngine::get_fan_min_current,
                "fan_min_current");
        BIND_PROPERTY(
                Variant::FLOAT, "fan_speed", "resistor_fan/speed", &TrainElectricSeriesEngine::set_fan_speed,
                &TrainElectricSeriesEngine::get_fan_speed, "fan_speed");
        BIND_PROPERTY(
                Variant::FLOAT, "dynamic_brake_resistance", "dynamic_brake_resistance",
                &TrainElectricSeriesEngine::set_dynamic_brake_resistance,
                &TrainElectricSeriesEngine::get_dynamic_brake_resistance, "dynamic_brake_resistance");
        BIND_PROPERTY(
                Variant::FLOAT, "dynamic_brake_resistance_1", "dynamic_brake_resistance_1",
                &TrainElectricSeriesEngine::set_dynamic_brake_resistance_1,
                &TrainElectricSeriesEngine::get_dynamic_brake_resistance_1, "dynamic_brake_resistance_1");
        BIND_PROPERTY(
                Variant::FLOAT, "dynamic_brake_resistance_2", "dynamic_brake_resistance_2",
                &TrainElectricSeriesEngine::set_dynamic_brake_resistance_2,
                &TrainElectricSeriesEngine::get_dynamic_brake_resistance_2, "dynamic_brake_resistance_2");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "relay_list", "relay_list", &TrainElectricSeriesEngine::set_relay_list,
                &TrainElectricSeriesEngine::get_relay_list, "relay_list", PROPERTY_HINT_TYPE_STRING, "RelayListItem");

        BIND_ENUM_CONSTANT(FAN_TYPE_NONE);
        BIND_ENUM_CONSTANT(FAN_TYPE_YES);
        BIND_ENUM_CONSTANT(FAN_TYPE_AUTOMATIC);
    }

    TrainEngine::EngineType TrainElectricSeriesEngine::get_engine_type() {
        return TrainEngine::EngineType::ELECTRIC_SERIES_MOTOR;
    }

    void TrainElectricSeriesEngine::_do_update_internal_mover(TMoverParameters *p_mover) {
        TrainElectricEngine::_do_update_internal_mover(p_mover);
        p_mover->NominalVoltage = nominal_voltage;
        p_mover->WindingRes = winding_resistance;
        p_mover->nmax = max_rpm / 60.0;

        p_mover->RVentType = static_cast<int>(fan_type);
        p_mover->RVentnmax = fan_max_rpm;
        p_mover->RVentCutOff = fan_cutoff_resistance;
        p_mover->RVentMinI = fan_min_current;
        p_mover->RVentSpeed = fan_speed;
        p_mover->DynamicBrakeRes = dynamic_brake_resistance;
        p_mover->DynamicBrakeRes1 = dynamic_brake_resistance_1;
        p_mover->DynamicBrakeRes2 = dynamic_brake_resistance_2;

        /* RList: lista rezystorow rozruchowych i polaczen silnikow (rozruch samoczynny) */
        constexpr int MAX_RELAY_LIST = Maszyna::ResArraySize + 1;
        const int relay_list_size = static_cast<int>(relay_list.size());
        if (relay_list_size > MAX_RELAY_LIST) {
            UtilityFunctions::push_warning(
                    "[TrainElectricSeriesEngine]: relay_list has " + String::num(relay_list_size) +
                    " entries, exceeding the mover's limit of " + String::num(MAX_RELAY_LIST) + "; truncating.");
        }
        p_mover->RlistSize = std::min(MAX_RELAY_LIST, relay_list_size);
        for (int i = 0; i < p_mover->RlistSize; i++) {
            const Ref<RelayListItem> &row = relay_list[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[TrainElectricSeriesEngine]: relay_list property is null at index " + String::num(i));
                continue;
            }
            p_mover->RList[i].Relay = row->get_relay_position();
            p_mover->RList[i].R = row->get_resistance();
            p_mover->RList[i].Bn = row->get_branch_count();
            p_mover->RList[i].Mn = row->get_motors_per_branch();
            p_mover->RList[i].AutoSwitch = row->get_auto_switch();
            p_mover->RList[i].ScndAct = row->get_shunt_index();
        }
    }

} // namespace godot
