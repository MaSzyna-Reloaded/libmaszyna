#include "MaszynaMoverDieselConfiguration.hpp"

namespace godot {
    void MaszynaMoverDieselConfiguration::_bind_methods() {
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_AFTER_GEARBOX);
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC);
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_BETWEEN_TC_AND_ENGINE);
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_clutch_engage_speed, "mechanical/clutch");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_clutch_disengage_speed,
                "mechanical/clutch");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_min_rpm_hydro_drive_factor, "mechanical");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_regulator_acceleration, "mechanical");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_rpm_decrease_rate, "mechanical");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, clutch_min_velocity_full_engage, "mechanical/clutch");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, clutch_diameter, "mechanical/clutch");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, clutch_max_force, "mechanical/clutch");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, clutch_friction, "mechanical/clutch");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_unlock_velocity, "torque_converter");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, retarder_engage_velocity, "retarder");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, retarder_clutch_speed, "retarder");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_max_torque_ratio, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_coupling_point, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_lockup_torque, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_lockup_rate, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_unlock_rate, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_fill_rate_increase,
                "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_fill_rate_decrease,
                "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_torque_in_in, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_torque_in_out, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_torque_out_out, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_lockup_speed, "torque_converter");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, torque_converter_unlock_speed, "torque_converter");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverDieselConfiguration, Variant::ARRAY, vel2nmax_table, PROPERTY_HINT_TYPE_STRING,
                "VehicleCurvePointItem");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, retarder_torque_in_in, "retarder");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, retarder_fill_rate_increase, "retarder");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, retarder_fill_rate_decrease, "retarder");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, retarder_min_velocity, "retarder");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, throttle_table_nominal_fuel_dose,
                "throttle_table_positions");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverDieselConfiguration, Variant::ARRAY, throttle_table_positions, "throttle_table_positions",
                PROPERTY_HINT_TYPE_STRING, "MaszynaMoverThrottlePositionItem");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_heat_kw, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_heat_kv, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_heat_kfe, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_heat_kfs, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_heat_kfo, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_heat_kfo2, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_fan_speed, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, cooling_nominal_power, "cooling");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_inertia, "mechanical");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_min_rpm_hydro_drive, "mechanical");
        BIND_PROPERTY(MaszynaMoverDieselConfiguration, Variant::FLOAT, mechanical_min_rpm_retarder, "mechanical");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverDieselConfiguration, Variant::ARRAY, torque_converter_table, "torque_converter",
                PROPERTY_HINT_TYPE_STRING, "VehicleCurvePointItem");
        BIND_PROPERTY_W_HINT(
                MaszynaMoverDieselConfiguration, Variant::INT, retarder_placement, "retarder", PROPERTY_HINT_ENUM,
                "AfterGearbox,BetweenGearboxAndTC,BetweenTCAndEngine");
        BIND_PROPERTY(
                MaszynaMoverDieselConfiguration, Variant::FLOAT, throttle_table_resistance_torque,
                "throttle_table_positions");
    }
} // namespace godot
