#include "RailVehicleElectricSeriesEngine.hpp"
#include "macros.hpp"

#include <algorithm>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void RailVehicleElectricSeriesEngine::_bind_methods() {
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, nominal_voltage);
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, winding_resistance);
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, max_rpm);
        BIND_PROPERTY_W_HINT(
                RailVehicleElectricSeriesEngine, Variant::INT, resistor_fan_type, "resistor_fan", PROPERTY_HINT_ENUM,
                "None,Yes,Automatic");
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, resistor_fan_max_rpm, "resistor_fan");
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, resistor_fan_cutoff_resistance, "resistor_fan");
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, resistor_fan_min_current, "resistor_fan");
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, resistor_fan_speed, "resistor_fan");
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, dynamic_brake_resistance);
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, dynamic_brake_resistance_1);
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::FLOAT, dynamic_brake_resistance_2);
        BIND_PROPERTY(RailVehicleElectricSeriesEngine, Variant::BOOL, direction_switches_circuit_imin_high);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleElectricSeriesEngine, Variant::ARRAY, relay_list, PROPERTY_HINT_TYPE_STRING,
                "RailVehicleRelayListItem");

        BIND_ENUM_CONSTANT(FAN_TYPE_NONE);
        BIND_ENUM_CONSTANT(FAN_TYPE_YES);
        BIND_ENUM_CONSTANT(FAN_TYPE_AUTOMATIC);

        ClassDB::bind_method(
                D_METHOD("get_resistor_fan_rotation"), &RailVehicleElectricSeriesEngine::get_resistor_fan_rotation);
        ClassDB::bind_method(D_METHOD("get_circuit_imin"), &RailVehicleElectricSeriesEngine::get_circuit_imin);
        ClassDB::bind_method(
                D_METHOD("get_circuit_imin_high_enabled"),
                &RailVehicleElectricSeriesEngine::get_circuit_imin_high_enabled);
        ClassDB::bind_method(
                D_METHOD("get_next_position_velocity", "main_controller"),
                &RailVehicleElectricSeriesEngine::get_next_position_velocity);
    }

    RailVehicleEngine::EngineType RailVehicleElectricSeriesEngine::get_type() const {
        return RailVehicleEngine::EngineType::ELECTRIC_SERIES_MOTOR;
    }

    void RailVehicleElectricSeriesEngine::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleElectricEngine::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["resistor_fan_rotation"] = get_resistor_fan_rotation();
        p_state["circuit_imin"] = get_circuit_imin();
        p_state["circuit_imin_high_enabled"] = get_circuit_imin_high_enabled();
    }

    void RailVehicleElectricSeriesEngine::_fill_config_dictionary(Dictionary &p_config) const {
        RailVehicleElectricEngine::_fill_config_dictionary(p_config);
        p_config["direction_switches_circuit_imin_high"] = direction_switches_circuit_imin_high;
        // the reverser key's last position: "forward", or the high start past it (Train.cpp:9451-9458)
        p_config["direction_position_max"] = 1 + static_cast<int>(direction_switches_circuit_imin_high);
    }

    void RailVehicleElectricSeriesEngine::set_direction_switches_circuit_imin_high(const bool p_value) {
        direction_switches_circuit_imin_high = p_value;
    }

    bool RailVehicleElectricSeriesEngine::get_direction_switches_circuit_imin_high() const {
        return direction_switches_circuit_imin_high;
    }

} // namespace godot
