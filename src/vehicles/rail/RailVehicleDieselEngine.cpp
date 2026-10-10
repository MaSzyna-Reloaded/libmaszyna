#include "RailVehicleDieselEngine.hpp"
#include "macros.hpp"

#include <algorithm>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    double RailVehicleDieselEngine::get_rpm() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_rpm() : 0.0;
    }
    bool RailVehicleDieselEngine::get_oil_pump_active() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_oil_pump_active() : false;
    }
    bool RailVehicleDieselEngine::get_oil_pump_disabled() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_oil_pump_disabled() : false;
    }
    double RailVehicleDieselEngine::get_oil_pump_pressure() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_oil_pump_pressure() : 0.0;
    }
    bool RailVehicleDieselEngine::get_fuel_pump_active() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_fuel_pump_active() : false;
    }
    bool RailVehicleDieselEngine::get_fuel_pump_disabled() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_fuel_pump_disabled() : false;
    }
    bool RailVehicleDieselEngine::get_fuel_pump_enabled() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_fuel_pump_enabled() : false;
    }
    bool RailVehicleDieselEngine::get_oil_pump_enabled() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_oil_pump_enabled() : false;
    }
    bool RailVehicleDieselEngine::get_heat_malfunction() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_heat_malfunction() : false;
    }
    bool RailVehicleDieselEngine::get_startup() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_startup() : false;
    }
    bool RailVehicleDieselEngine::get_ignition() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_ignition() : false;
    }
    bool RailVehicleDieselEngine::get_spinup() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_spinup() : false;
    }
    double RailVehicleDieselEngine::get_output_power() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_output_power() : 0.0;
    }
    double RailVehicleDieselEngine::get_torque() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_torque() : 0.0;
    }
    double RailVehicleDieselEngine::get_fill() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_fill() : 0.0;
    }
    double RailVehicleDieselEngine::get_fill_desired() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_fill_desired() : 0.0;
    }
    double RailVehicleDieselEngine::get_clutch_desired() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_clutch_desired() : 0.0;
    }
    double RailVehicleDieselEngine::get_clutch_engagement() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_clutch_engagement() : 0.0;
    }
    double RailVehicleDieselEngine::get_water_temperature() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_temperature() : 0.0;
    }
    double RailVehicleDieselEngine::get_engine_temperature() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_engine_temperature() : 0.0;
    }
    double RailVehicleDieselEngine::get_retarder_fill() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_retarder_fill() : 0.0;
    }
    double RailVehicleDieselEngine::get_max_rpm() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_max_rpm() : 0.0;
    }
    double RailVehicleDieselEngine::get_idle_rpm_count() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_idle_rpm_count() : 0.0;
    }
    bool RailVehicleDieselEngine::get_water_pump_enabled() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_pump_enabled() : false;
    }
    bool RailVehicleDieselEngine::get_water_pump_active() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_pump_active() : false;
    }
    bool RailVehicleDieselEngine::get_water_pump_breaker() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_pump_breaker() : false;
    }
    bool RailVehicleDieselEngine::get_water_heater_enabled() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_heater_enabled() : false;
    }
    bool RailVehicleDieselEngine::get_water_heater_active() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_heater_active() : false;
    }
    bool RailVehicleDieselEngine::get_water_heater_breaker() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_heater_breaker() : false;
    }
    bool RailVehicleDieselEngine::get_water_circuits_link() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_water_circuits_link() : false;
    }
    double RailVehicleDieselEngine::get_main_circuit_water_temperature() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_main_circuit_water_temperature() : 0.0;
    }
    double RailVehicleDieselEngine::get_auxiliary_circuit_water_temperature() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_auxiliary_circuit_water_temperature() : 0.0;
    }
    double RailVehicleDieselEngine::get_oil_temperature() const {
        return diesel_engine_unit != nullptr ? diesel_engine_unit->get_oil_temperature() : 0.0;
    }
    void RailVehicleDieselEngine::_apply_configuration() {
        RailVehicleEngine::_apply_configuration();
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->apply_configuration(this);
        }
    }
    void RailVehicleDieselEngine::_fill_config_dictionary(Dictionary &p_config) const {
        RailVehicleEngine::_fill_config_dictionary(p_config);
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->fill_config(p_config);
        }
    }

    void RailVehicleDieselEngine::set_turbo_position(const int p_value) {
        turbo_position = p_value;
    }

    int RailVehicleDieselEngine::get_turbo_position() const {
        return turbo_position;
    }

    void RailVehicleDieselEngine::_bind_methods() {
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::INT, turbo_position);
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, oil_pump_pressure_minimum, "oil_pump");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, oil_pump_pressure_maximum, "oil_pump");
        BIND_PROPERTY_W_HINT(
                RailVehicleDieselEngine, Variant::INT, fuel_pump_start_mode, "fuel_pump", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                RailVehicleDieselEngine, Variant::INT, oil_pump_start_mode, "oil_pump", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                RailVehicleDieselEngine, Variant::INT, water_pump_start_mode, "water_pump", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_min_rpm, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_max_rpm, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_fuel_cutoff_rpm, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_inertia, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_clutch_engage_speed, "mechanical/clutch");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_clutch_disengage_speed, "mechanical/clutch");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_min_rpm_hydro_drive, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_min_rpm_hydro_drive_factor, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_min_rpm_retarder, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_nominal_max_rpm, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_regulator_acceleration, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_rpm_decrease_rate, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, mechanical_shunt_mode_ratio, "mechanical");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, clutch_min_velocity_full_engage, "mechanical/clutch");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, clutch_diameter, "mechanical/clutch");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, clutch_max_force, "mechanical/clutch");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, clutch_friction, "mechanical/clutch");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_unlock_velocity, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_engage_velocity, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, retarder_clutch, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_clutch_speed, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, retarder_with_individual, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, torque_converter_present, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_max_torque_ratio, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_coupling_point, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_lockup_torque, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_lockup_rate, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_unlock_rate, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_fill_rate_increase, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_fill_rate_decrease, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_torque_in_in, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_torque_in_out, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_torque_out_out, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_lockup_speed, "torque_converter");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, torque_converter_unlock_speed, "torque_converter");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleDieselEngine, Variant::ARRAY, torque_converter_table, "torque_converter",
                PROPERTY_HINT_TYPE_STRING, "VehicleCurvePointItem");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleDieselEngine, Variant::ARRAY, vel2nmax_table, PROPERTY_HINT_TYPE_STRING,
                "VehicleCurvePointItem");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, retarder_present, "retarder");
        BIND_PROPERTY_W_HINT(
                RailVehicleDieselEngine, Variant::INT, retarder_placement, "retarder", PROPERTY_HINT_ENUM,
                "AfterGearbox,BetweenGearboxAndTC,BetweenTCAndEngine");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_torque_in_in, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_max_torque, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_max_power, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_fill_rate_increase, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_fill_rate_decrease, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, retarder_min_velocity, "retarder");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, throttle_table_max_torque, "throttle_table_positions");
        BIND_PROPERTY(
                RailVehicleDieselEngine, Variant::FLOAT, throttle_table_max_torque_rpm, "throttle_table_positions");
        BIND_PROPERTY(
                RailVehicleDieselEngine, Variant::FLOAT, throttle_table_max_rpm_torque, "throttle_table_positions");
        BIND_PROPERTY(
                RailVehicleDieselEngine, Variant::FLOAT, throttle_table_nominal_fuel_dose, "throttle_table_positions");
        BIND_PROPERTY(
                RailVehicleDieselEngine, Variant::FLOAT, throttle_table_resistance_torque, "throttle_table_positions");
        BIND_PROPERTY(
                RailVehicleDieselEngine, Variant::FLOAT, throttle_table_nominal_fuel_consumption_rate,
                "throttle_table_positions");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleDieselEngine, Variant::ARRAY, throttle_table_positions, "throttle_table_positions",
                PROPERTY_HINT_TYPE_STRING, "RailVehicleThrottlePositionItem");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleDieselEngine, Variant::ARRAY, torque_table, PROPERTY_HINT_TYPE_STRING,
                "VehicleCurvePointItem");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heat_kw, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heat_kv, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heat_kfe, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heat_kfs, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heat_kfo, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heat_kfo2, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_min_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_max_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_flow_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_cooling_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, cooling_water_shutters, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, cooling_water_aux_circuit, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_aux_min_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_aux_max_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_water_aux_cooling_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::BOOL, cooling_water_aux_shutters, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_oil_min_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_oil_max_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_fan_speed, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heater_min_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_heater_max_temperature, "cooling");
        BIND_PROPERTY(RailVehicleDieselEngine, Variant::FLOAT, cooling_nominal_power, "cooling");
        ClassDB::bind_method(D_METHOD("fuel_pump", "enabled"), &RailVehicleDieselEngine::fuel_pump);
        ClassDB::bind_method(D_METHOD("oil_pump", "enabled"), &RailVehicleDieselEngine::oil_pump);
        ClassDB::bind_method(
                D_METHOD("fuel_pump_switch_off", "enabled"), &RailVehicleDieselEngine::fuel_pump_switch_off);
        ClassDB::bind_method(D_METHOD("oil_pump_switch_off", "enabled"), &RailVehicleDieselEngine::oil_pump_switch_off);
        ClassDB::bind_method(D_METHOD("get_fuel_pump_enabled"), &RailVehicleDieselEngine::get_fuel_pump_enabled);
        ClassDB::bind_method(D_METHOD("get_oil_pump_enabled"), &RailVehicleDieselEngine::get_oil_pump_enabled);
        ClassDB::bind_method(D_METHOD("get_heat_malfunction"), &RailVehicleDieselEngine::get_heat_malfunction);

        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_AFTER_GEARBOX);
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC);
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_BETWEEN_TC_AND_ENGINE);

        ClassDB::bind_method(D_METHOD("get_rpm"), &RailVehicleDieselEngine::get_rpm);
        ClassDB::bind_method(D_METHOD("get_oil_pump_active"), &RailVehicleDieselEngine::get_oil_pump_active);
        ClassDB::bind_method(D_METHOD("get_oil_pump_disabled"), &RailVehicleDieselEngine::get_oil_pump_disabled);
        ClassDB::bind_method(D_METHOD("get_oil_pump_pressure"), &RailVehicleDieselEngine::get_oil_pump_pressure);
        ClassDB::bind_method(D_METHOD("get_fuel_pump_active"), &RailVehicleDieselEngine::get_fuel_pump_active);
        ClassDB::bind_method(D_METHOD("get_fuel_pump_disabled"), &RailVehicleDieselEngine::get_fuel_pump_disabled);
        ClassDB::bind_method(D_METHOD("get_startup"), &RailVehicleDieselEngine::get_startup);
        ClassDB::bind_method(D_METHOD("get_ignition"), &RailVehicleDieselEngine::get_ignition);
        ClassDB::bind_method(D_METHOD("get_spinup"), &RailVehicleDieselEngine::get_spinup);
        ClassDB::bind_method(D_METHOD("get_output_power"), &RailVehicleDieselEngine::get_output_power);
        ClassDB::bind_method(D_METHOD("get_torque"), &RailVehicleDieselEngine::get_torque);
        ClassDB::bind_method(D_METHOD("get_fill"), &RailVehicleDieselEngine::get_fill);
        ClassDB::bind_method(D_METHOD("get_fill_desired"), &RailVehicleDieselEngine::get_fill_desired);
        ClassDB::bind_method(D_METHOD("get_clutch_desired"), &RailVehicleDieselEngine::get_clutch_desired);
        ClassDB::bind_method(D_METHOD("get_clutch_engagement"), &RailVehicleDieselEngine::get_clutch_engagement);
        ClassDB::bind_method(D_METHOD("get_water_temperature"), &RailVehicleDieselEngine::get_water_temperature);
        ClassDB::bind_method(D_METHOD("get_engine_temperature"), &RailVehicleDieselEngine::get_engine_temperature);
        ClassDB::bind_method(D_METHOD("get_retarder_fill"), &RailVehicleDieselEngine::get_retarder_fill);
        ClassDB::bind_method(D_METHOD("get_max_rpm"), &RailVehicleDieselEngine::get_max_rpm);
        ClassDB::bind_method(D_METHOD("get_idle_rpm_count"), &RailVehicleDieselEngine::get_idle_rpm_count);
        ClassDB::bind_method(D_METHOD("get_water_pump_enabled"), &RailVehicleDieselEngine::get_water_pump_enabled);
        ClassDB::bind_method(D_METHOD("get_water_pump_active"), &RailVehicleDieselEngine::get_water_pump_active);
        ClassDB::bind_method(D_METHOD("get_water_pump_breaker"), &RailVehicleDieselEngine::get_water_pump_breaker);
        ClassDB::bind_method(D_METHOD("get_water_heater_enabled"), &RailVehicleDieselEngine::get_water_heater_enabled);
        ClassDB::bind_method(D_METHOD("get_water_heater_active"), &RailVehicleDieselEngine::get_water_heater_active);
        ClassDB::bind_method(D_METHOD("get_water_heater_breaker"), &RailVehicleDieselEngine::get_water_heater_breaker);
        ClassDB::bind_method(D_METHOD("get_water_circuits_link"), &RailVehicleDieselEngine::get_water_circuits_link);
        ClassDB::bind_method(
                D_METHOD("get_main_circuit_water_temperature"),
                &RailVehicleDieselEngine::get_main_circuit_water_temperature);
        ClassDB::bind_method(
                D_METHOD("get_auxiliary_circuit_water_temperature"),
                &RailVehicleDieselEngine::get_auxiliary_circuit_water_temperature);
        ClassDB::bind_method(D_METHOD("get_oil_temperature"), &RailVehicleDieselEngine::get_oil_temperature);
        ClassDB::bind_method(D_METHOD("water_pump", "enabled"), &RailVehicleDieselEngine::water_pump);
        ClassDB::bind_method(
                D_METHOD("water_pump_switch_off", "enabled"), &RailVehicleDieselEngine::water_pump_switch_off);
        ClassDB::bind_method(D_METHOD("water_pump_breaker", "enabled"), &RailVehicleDieselEngine::water_pump_breaker);
        ClassDB::bind_method(D_METHOD("water_heater", "enabled"), &RailVehicleDieselEngine::water_heater);
        ClassDB::bind_method(
                D_METHOD("water_heater_breaker", "enabled"), &RailVehicleDieselEngine::water_heater_breaker);
        ClassDB::bind_method(D_METHOD("water_circuits_link", "enabled"), &RailVehicleDieselEngine::water_circuits_link);
    }

    RailVehicleEngine::EngineType RailVehicleDieselEngine::get_type() const {
        return RailVehicleEngine::EngineType::DIESEL;
    }


    void RailVehicleDieselEngine::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleEngine::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["engine_rpm"] = get_rpm();
        p_state["oil_pump_active"] = get_oil_pump_active();
        p_state["oil_pump_disabled"] = get_oil_pump_disabled();
        p_state["oil_pump_pressure"] = get_oil_pump_pressure();
        p_state["fuel_pump_active"] = get_fuel_pump_active();
        p_state["fuel_pump_disabled"] = get_fuel_pump_disabled();
        p_state["diesel_heat_malfunction"] = get_heat_malfunction();
        p_state["fuel_pump_enabled"] = get_fuel_pump_enabled();
        p_state["oil_pump_enabled"] = get_oil_pump_enabled();
        p_state["diesel_startup"] = get_startup();
        p_state["diesel_ignition"] = get_ignition();
        p_state["diesel_spinup"] = get_spinup();
        p_state["diesel_power"] = get_output_power();
        p_state["diesel_torque"] = get_torque();
        p_state["diesel_fill"] = get_fill();
        p_state["diesel_fill_desired"] = get_fill_desired();
        p_state["diesel_clutch_desired"] = get_clutch_desired();
        p_state["diesel_clutch_engagement"] = get_clutch_engagement();
        p_state["diesel_water_temperature"] = get_water_temperature();
        p_state["diesel_engine_temperature"] = get_engine_temperature();
        p_state["diesel_retarder_fill"] = get_retarder_fill();
        p_state["diesel_max_rpm"] = get_max_rpm();
        p_state["water_pump_enabled"] = get_water_pump_enabled();
        p_state["water_pump_active"] = get_water_pump_active();
        p_state["water_pump_breaker"] = get_water_pump_breaker();
        p_state["water_heater_enabled"] = get_water_heater_enabled();
        p_state["water_heater_active"] = get_water_heater_active();
        p_state["water_heater_breaker"] = get_water_heater_breaker();
        p_state["water_circuits_link"] = get_water_circuits_link();
        p_state["main_circuit_water_temperature"] = get_main_circuit_water_temperature();
        p_state["auxiliary_circuit_water_temperature"] = get_auxiliary_circuit_water_temperature();
        p_state["oil_temperature"] = get_oil_temperature();
    }

    void RailVehicleDieselEngine::oil_pump(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->oil_pump(p_enabled);
        }
    }

    void RailVehicleDieselEngine::fuel_pump(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->fuel_pump(p_enabled);
        }
    }

    void RailVehicleDieselEngine::oil_pump_switch_off(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->oil_pump_switch_off(p_enabled);
        }
    }

    void RailVehicleDieselEngine::fuel_pump_switch_off(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->fuel_pump_switch_off(p_enabled);
        }
    }

    void RailVehicleDieselEngine::water_pump(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->water_pump(p_enabled);
        }
    }

    void RailVehicleDieselEngine::water_pump_switch_off(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->water_pump_switch_off(p_enabled);
        }
    }

    void RailVehicleDieselEngine::water_pump_breaker(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->water_pump_breaker(p_enabled);
        }
    }

    void RailVehicleDieselEngine::water_heater(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->water_heater(p_enabled);
        }
    }

    void RailVehicleDieselEngine::water_heater_breaker(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->water_heater_breaker(p_enabled);
        }
    }

    void RailVehicleDieselEngine::water_circuits_link(const bool p_enabled) {
        if (diesel_engine_unit != nullptr) {
            diesel_engine_unit->water_circuits_link(p_enabled);
        }
    }

    void RailVehicleDieselEngine::_register_commands() {
        RailVehicleEngine::_register_commands();
        register_command("oil_pump", Callable(this, "oil_pump"));
        register_command("fuel_pump", Callable(this, "fuel_pump"));
        register_command("oil_pump_switch_off", Callable(this, "oil_pump_switch_off"));
        register_command("fuel_pump_switch_off", Callable(this, "fuel_pump_switch_off"));
        register_command("water_pump", Callable(this, "water_pump"));
        register_command("water_pump_switch_off", Callable(this, "water_pump_switch_off"));
        register_command("water_pump_breaker", Callable(this, "water_pump_breaker"));
        register_command("water_heater", Callable(this, "water_heater"));
        register_command("water_heater_breaker", Callable(this, "water_heater_breaker"));
        register_command("water_circuits_link", Callable(this, "water_circuits_link"));
    }

    void RailVehicleDieselEngine::_unregister_commands() {
        RailVehicleEngine::_unregister_commands();
        unregister_command("oil_pump");
        unregister_command("fuel_pump");
        unregister_command("oil_pump_switch_off");
        unregister_command("fuel_pump_switch_off");
        unregister_command("water_pump");
        unregister_command("water_pump_switch_off");
        unregister_command("water_pump_breaker");
        unregister_command("water_heater");
        unregister_command("water_heater_breaker");
        unregister_command("water_circuits_link");
    }
} // namespace godot
