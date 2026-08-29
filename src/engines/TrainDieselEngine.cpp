#include "TrainDieselEngine.hpp"
#include "macros.hpp"

#include <algorithm>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainDieselEngine::_bind_methods() {
        BIND_PROPERTY(
                Variant::FLOAT, "oil_min_pressure", "oil_pump/pressure_minimum",
                &TrainDieselEngine::set_oil_min_pressure, &TrainDieselEngine::get_oil_min_pressure, "oil_min_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "oil_max_pressure", "oil_pump/pressure_maximum",
                &TrainDieselEngine::set_oil_max_pressure, &TrainDieselEngine::get_oil_max_pressure, "oil_max_pressure");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "fuel_pump_start_mode", "fuel_pump/start_mode", &TrainDieselEngine::set_fuel_pump_start_mode,
                &TrainDieselEngine::get_fuel_pump_start_mode, "fuel_pump_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "oil_pump_start_mode", "oil_pump/start_mode", &TrainDieselEngine::set_oil_pump_start_mode,
                &TrainDieselEngine::get_oil_pump_start_mode, "oil_pump_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "water_pump_start_mode", "water_pump/start_mode",
                &TrainDieselEngine::set_water_pump_start_mode, &TrainDieselEngine::get_water_pump_start_mode,
                "water_pump_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                Variant::FLOAT, "min_rpm", "mechanical/min_rpm", &TrainDieselEngine::set_min_rpm,
                &TrainDieselEngine::get_min_rpm, "min_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "max_rpm", "mechanical/max_rpm", &TrainDieselEngine::set_max_rpm,
                &TrainDieselEngine::get_max_rpm, "max_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "fuel_cutoff_rpm", "mechanical/fuel_cutoff_rpm", &TrainDieselEngine::set_fuel_cutoff_rpm,
                &TrainDieselEngine::get_fuel_cutoff_rpm, "fuel_cutoff_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "inertia", "mechanical/inertia", &TrainDieselEngine::set_inertia,
                &TrainDieselEngine::get_inertia, "inertia");
        BIND_PROPERTY(
                Variant::FLOAT, "clutch_engage_speed", "mechanical/clutch/engage_speed",
                &TrainDieselEngine::set_clutch_engage_speed, &TrainDieselEngine::get_clutch_engage_speed,
                "clutch_engage_speed");
        BIND_PROPERTY(
                Variant::FLOAT, "clutch_disengage_speed", "mechanical/clutch/disengage_speed",
                &TrainDieselEngine::set_clutch_disengage_speed, &TrainDieselEngine::get_clutch_disengage_speed,
                "clutch_disengage_speed");
        BIND_PROPERTY(
                Variant::BOOL, "has_torque_converter", "torque_converter/present",
                &TrainDieselEngine::set_has_torque_converter, &TrainDieselEngine::get_has_torque_converter,
                "has_torque_converter");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_max_torque_ratio", "torque_converter/max_torque_ratio",
                &TrainDieselEngine::set_tc_max_torque_ratio, &TrainDieselEngine::get_tc_max_torque_ratio,
                "tc_max_torque_ratio");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_coupling_point", "torque_converter/coupling_point",
                &TrainDieselEngine::set_tc_coupling_point, &TrainDieselEngine::get_tc_coupling_point,
                "tc_coupling_point");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_lockup_torque", "torque_converter/lockup_torque",
                &TrainDieselEngine::set_tc_lockup_torque, &TrainDieselEngine::get_tc_lockup_torque,
                "tc_lockup_torque");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_lockup_rate", "torque_converter/lockup_rate", &TrainDieselEngine::set_tc_lockup_rate,
                &TrainDieselEngine::get_tc_lockup_rate, "tc_lockup_rate");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_unlock_rate", "torque_converter/unlock_rate", &TrainDieselEngine::set_tc_unlock_rate,
                &TrainDieselEngine::get_tc_unlock_rate, "tc_unlock_rate");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_fill_rate_increase", "torque_converter/fill_rate_increase",
                &TrainDieselEngine::set_tc_fill_rate_increase, &TrainDieselEngine::get_tc_fill_rate_increase,
                "tc_fill_rate_increase");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_fill_rate_decrease", "torque_converter/fill_rate_decrease",
                &TrainDieselEngine::set_tc_fill_rate_decrease, &TrainDieselEngine::get_tc_fill_rate_decrease,
                "tc_fill_rate_decrease");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_torque_in_in", "torque_converter/torque_in_in",
                &TrainDieselEngine::set_tc_torque_in_in, &TrainDieselEngine::get_tc_torque_in_in, "tc_torque_in_in");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_torque_in_out", "torque_converter/torque_in_out",
                &TrainDieselEngine::set_tc_torque_in_out, &TrainDieselEngine::get_tc_torque_in_out, "tc_torque_in_out");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_torque_out_out", "torque_converter/torque_out_out",
                &TrainDieselEngine::set_tc_torque_out_out, &TrainDieselEngine::get_tc_torque_out_out,
                "tc_torque_out_out");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_lockup_speed", "torque_converter/lockup_speed",
                &TrainDieselEngine::set_tc_lockup_speed, &TrainDieselEngine::get_tc_lockup_speed, "tc_lockup_speed");
        BIND_PROPERTY(
                Variant::FLOAT, "tc_unlock_speed", "torque_converter/unlock_speed",
                &TrainDieselEngine::set_tc_unlock_speed, &TrainDieselEngine::get_tc_unlock_speed, "tc_unlock_speed");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "torque_converter_table", "torque_converter/table",
                &TrainDieselEngine::set_torque_converter_table, &TrainDieselEngine::get_torque_converter_table,
                "torque_converter_table", PROPERTY_HINT_TYPE_STRING, "CurvePointItem");
        BIND_PROPERTY(
                Variant::BOOL, "has_retarder", "retarder/present", &TrainDieselEngine::set_has_retarder,
                &TrainDieselEngine::get_has_retarder, "has_retarder");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "retarder_placement", "retarder/placement", &TrainDieselEngine::set_retarder_placement,
                &TrainDieselEngine::get_retarder_placement, "retarder_placement", PROPERTY_HINT_ENUM,
                "AfterGearbox,BetweenGearboxAndTC,BetweenTCAndEngine");
        BIND_PROPERTY(
                Variant::FLOAT, "retarder_torque_in_in", "retarder/torque_in_in",
                &TrainDieselEngine::set_retarder_torque_in_in, &TrainDieselEngine::get_retarder_torque_in_in,
                "retarder_torque_in_in");
        BIND_PROPERTY(
                Variant::FLOAT, "retarder_max_torque", "retarder/max_torque", &TrainDieselEngine::set_retarder_max_torque,
                &TrainDieselEngine::get_retarder_max_torque, "retarder_max_torque");
        BIND_PROPERTY(
                Variant::FLOAT, "retarder_max_power", "retarder/max_power", &TrainDieselEngine::set_retarder_max_power,
                &TrainDieselEngine::get_retarder_max_power, "retarder_max_power");
        BIND_PROPERTY(
                Variant::FLOAT, "retarder_fill_rate_increase", "retarder/fill_rate_increase",
                &TrainDieselEngine::set_retarder_fill_rate_increase,
                &TrainDieselEngine::get_retarder_fill_rate_increase, "retarder_fill_rate_increase");
        BIND_PROPERTY(
                Variant::FLOAT, "retarder_fill_rate_decrease", "retarder/fill_rate_decrease",
                &TrainDieselEngine::set_retarder_fill_rate_decrease,
                &TrainDieselEngine::get_retarder_fill_rate_decrease, "retarder_fill_rate_decrease");
        BIND_PROPERTY(
                Variant::FLOAT, "retarder_min_velocity", "retarder/min_velocity",
                &TrainDieselEngine::set_retarder_min_velocity, &TrainDieselEngine::get_retarder_min_velocity,
                "retarder_min_velocity");
        BIND_PROPERTY(
                Variant::FLOAT, "max_torque", "throttle_table/max_torque", &TrainDieselEngine::set_max_torque,
                &TrainDieselEngine::get_max_torque, "max_torque");
        BIND_PROPERTY(
                Variant::FLOAT, "max_torque_rpm", "throttle_table/max_torque_rpm",
                &TrainDieselEngine::set_max_torque_rpm, &TrainDieselEngine::get_max_torque_rpm, "max_torque_rpm");
        BIND_PROPERTY(
                Variant::FLOAT, "max_rpm_torque", "throttle_table/max_rpm_torque",
                &TrainDieselEngine::set_max_rpm_torque, &TrainDieselEngine::get_max_rpm_torque, "max_rpm_torque");
        BIND_PROPERTY(
                Variant::FLOAT, "nominal_fuel_dose", "throttle_table/nominal_fuel_dose",
                &TrainDieselEngine::set_nominal_fuel_dose, &TrainDieselEngine::get_nominal_fuel_dose,
                "nominal_fuel_dose");
        BIND_PROPERTY(
                Variant::FLOAT, "resistance_torque", "throttle_table/resistance_torque",
                &TrainDieselEngine::set_resistance_torque, &TrainDieselEngine::get_resistance_torque,
                "resistance_torque");
        BIND_PROPERTY(
                Variant::FLOAT, "nominal_fuel_consumption_rate", "throttle_table/nominal_fuel_consumption_rate",
                &TrainDieselEngine::set_nominal_fuel_consumption_rate,
                &TrainDieselEngine::get_nominal_fuel_consumption_rate, "nominal_fuel_consumption_rate");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "throttle_table", "throttle_table/positions", &TrainDieselEngine::set_throttle_table,
                &TrainDieselEngine::get_throttle_table, "throttle_table", PROPERTY_HINT_TYPE_STRING,
                "ThrottlePositionItem");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "torque_table", "torque_table", &TrainDieselEngine::set_torque_table,
                &TrainDieselEngine::get_torque_table, "torque_table", PROPERTY_HINT_TYPE_STRING, "CurvePointItem");
        ClassDB::bind_method(D_METHOD("fuel_pump", "enabled"), &TrainDieselEngine::fuel_pump);
        ClassDB::bind_method(D_METHOD("oil_pump", "enabled"), &TrainDieselEngine::oil_pump);

        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_AFTER_GEARBOX);
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC);
        BIND_ENUM_CONSTANT(RETARDER_PLACEMENT_BETWEEN_TC_AND_ENGINE);
    }

    TrainEngine::EngineType TrainDieselEngine::get_engine_type() {
        return TrainEngine::EngineType::DIESEL;
    }

    void TrainDieselEngine::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        TrainEngine::_do_fetch_state_from_mover(p_mover, p_state);

        p_state["engine_rpm"] = p_mover->EngineRPMRatio() * p_mover->EngineMaxRPM();
        p_state["oil_pump_active"] = p_mover->OilPump.is_active;
        p_state["oil_pump_disabled"] = p_mover->OilPump.is_disabled;
        p_state["oil_pump_pressure"] = p_mover->OilPump.pressure;

        p_state["fuel_pump_active"] = p_mover->FuelPump.is_active;
        p_state["fuel_pump_disabled"] = p_mover->FuelPump.is_disabled;

        p_state["diesel_startup"] = p_mover->dizel_startup;
        p_state["diesel_ignition"] = p_mover->dizel_ignition;
        p_state["diesel_spinup"] = p_mover->dizel_spinup;
        p_state["diesel_power"] = p_mover->dizel_Power;
        p_state["diesel_torque"] = p_mover->dizel_Torque;
        p_state["diesel_fill"] = p_mover->dizel_fill;
    }

    void TrainDieselEngine::_do_update_internal_mover(TMoverParameters *p_mover) {
        TrainEngine::_do_update_internal_mover(p_mover);

        // FIXME: test data
        p_mover->EnginePowerSource.SourceType = TPowerSource::Accumulator;
        // end test data

        p_mover->OilPump.pressure_minimum = oil_min_pressure;
        p_mover->OilPump.pressure_maximum = oil_max_pressure;
        p_mover->FuelPump.start_type = start_mode_map.at(fuel_pump_start_mode);
        p_mover->OilPump.start_type = start_mode_map.at(oil_pump_start_mode);
        p_mover->WaterPump.start_type = start_mode_map.at(water_pump_start_mode);

        p_mover->dizel_nmin = min_rpm;
        p_mover->dizel_nmax = max_rpm;
        p_mover->dizel_nmax_cutoff = fuel_cutoff_rpm;
        p_mover->dizel_AIM = inertia;
        p_mover->engageupspeed = clutch_engage_speed;
        p_mover->engagedownspeed = clutch_disengage_speed;

        p_mover->hydro_TC = has_torque_converter;
        p_mover->hydro_TC_TMMax = tc_max_torque_ratio;
        p_mover->hydro_TC_CouplingPoint = tc_coupling_point;
        p_mover->hydro_TC_LockupTorque = tc_lockup_torque;
        p_mover->hydro_TC_LockupRate = tc_lockup_rate;
        p_mover->hydro_TC_UnlockRate = tc_unlock_rate;
        p_mover->hydro_TC_FillRateInc = tc_fill_rate_increase;
        p_mover->hydro_TC_FillRateDec = tc_fill_rate_decrease;
        p_mover->hydro_TC_TorqueInIn = tc_torque_in_in;
        p_mover->hydro_TC_TorqueInOut = tc_torque_in_out;
        p_mover->hydro_TC_TorqueOutOut = tc_torque_out_out;
        p_mover->hydro_TC_LockupSpeed = tc_lockup_speed;
        p_mover->hydro_TC_UnlockSpeed = tc_unlock_speed;

        p_mover->hydro_TC_Table.clear();
        for (int i = 0; i < torque_converter_table.size(); i++) {
            const Ref<CurvePointItem> &row = torque_converter_table[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[TrainDieselEngine]: torque_converter_table property is null at index " + String::num(i));
                continue;
            }
            p_mover->hydro_TC_Table.emplace(row->get_x(), row->get_y());
        }

        p_mover->hydro_R = has_retarder;
        p_mover->hydro_R_Placement = retarder_placement;
        p_mover->hydro_R_TorqueInIn = retarder_torque_in_in;
        p_mover->hydro_R_MaxTorque = retarder_max_torque;
        p_mover->hydro_R_MaxPower = retarder_max_power;
        p_mover->hydro_R_FillRateInc = retarder_fill_rate_increase;
        p_mover->hydro_R_FillRateDec = retarder_fill_rate_decrease;
        p_mover->hydro_R_MinVel = retarder_min_velocity;

        /* DList: tabela przepustnicy */
        p_mover->dizel_Mmax = max_torque;
        p_mover->dizel_nMmax = max_torque_rpm;
        p_mover->dizel_Mnmax = max_rpm_torque;
        p_mover->dizel_nominalfill = nominal_fuel_dose;
        p_mover->dizel_Mstand = resistance_torque;
        p_mover->dizel_NominalFuelConsumptionRate = nominal_fuel_consumption_rate;

        constexpr int MAX_THROTTLE_TABLE = Maszyna::ResArraySize + 1;
        const int throttle_table_size = static_cast<int>(throttle_table.size());
        if (throttle_table_size > MAX_THROTTLE_TABLE) {
            UtilityFunctions::push_warning(
                    "[TrainDieselEngine]: throttle_table has " + String::num_int64(throttle_table_size) +
                    " entries, exceeding the mover's limit of " + String::num_int64(MAX_THROTTLE_TABLE) +
                    "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_THROTTLE_TABLE, throttle_table_size); i++) {
            const Ref<ThrottlePositionItem> &row = throttle_table[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[TrainDieselEngine]: throttle_table property is null at index " + String::num(i));
                continue;
            }
            p_mover->RList[i].Relay = row->get_throttle_position();
            p_mover->RList[i].R = row->get_fuel_dose();
            p_mover->RList[i].Mn = row->get_clutch_behavior();
        }

        /* DMList: charakterystyka momentu obrotowego silnika spalinowego */
        p_mover->dizel_Momentum_Table.clear();
        for (int i = 0; i < torque_table.size(); i++) {
            const Ref<CurvePointItem> &row = torque_table[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[TrainDieselEngine]: torque_table property is null at index " + String::num(i));
                continue;
            }
            p_mover->dizel_Momentum_Table.emplace(row->get_x() / 60.0, row->get_y());
        }
    }

    void TrainDieselEngine::oil_pump(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->OilPumpSwitch(p_enabled);
    }

    void TrainDieselEngine::fuel_pump(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->FuelPumpSwitch(p_enabled);
    }

    void TrainDieselEngine::_register_commands() {
        TrainEngine::_register_commands();
        register_command("oil_pump", Callable(this, "oil_pump"));
        register_command("fuel_pump", Callable(this, "fuel_pump"));
    }

    void TrainDieselEngine::_unregister_commands() {
        TrainEngine::_unregister_commands();
        unregister_command("oil_pump", Callable(this, "oil_pump"));
        unregister_command("fuel_pump", Callable(this, "fuel_pump"));
    }
} // namespace godot
