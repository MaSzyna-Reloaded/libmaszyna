#include "MoverDieselEngineUnit.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "utils/LibMaszynaUnits.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"

namespace godot {
    /* dizel_nreg_min = dizel_nmin * 0.98 (Mover.cpp:11173) */
    static constexpr double NREG_MIN_SHARE = 0.98;

    double MoverDieselEngineUnit::get_rpm() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngineRPMRatio() * p_mover->EngineMaxRPM() : 0.0;
    }

    bool MoverDieselEngineUnit::get_oil_pump_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.is_active : false;
    }

    bool MoverDieselEngineUnit::get_oil_pump_disabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.is_disabled : false;
    }

    double MoverDieselEngineUnit::get_oil_pump_pressure() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.pressure : 0.0;
    }

    bool MoverDieselEngineUnit::get_fuel_pump_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->FuelPump.is_active : false;
    }

    bool MoverDieselEngineUnit::get_fuel_pump_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->FuelPump.is_enabled : false;
    }

    bool MoverDieselEngineUnit::get_oil_pump_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.is_enabled : false;
    }

    bool MoverDieselEngineUnit::get_heat_malfunction() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.PA : false;
    }

    bool MoverDieselEngineUnit::get_fuel_pump_disabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->FuelPump.is_disabled : false;
    }

    bool MoverDieselEngineUnit::get_startup() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_startup : false;
    }

    bool MoverDieselEngineUnit::get_ignition() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_ignition : false;
    }

    bool MoverDieselEngineUnit::get_spinup() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_spinup : false;
    }

    double MoverDieselEngineUnit::get_output_power() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_Power : 0.0;
    }

    double MoverDieselEngineUnit::get_torque() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_Torque : 0.0;
    }

    double MoverDieselEngineUnit::get_fill() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_fill : 0.0;
    }

    double MoverDieselEngineUnit::get_fill_desired() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->RList[p_mover->MainCtrlPos].R : 0.0;
    }

    double MoverDieselEngineUnit::get_clutch_desired() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->RList[p_mover->MainCtrlPos].Mn : 0.0;
    }

    double MoverDieselEngineUnit::get_clutch_engagement() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_engage : 0.0;
    }

    double MoverDieselEngineUnit::get_water_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.Twy : 0.0;
    }

    double MoverDieselEngineUnit::get_engine_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.Ts : 0.0;
    }

    double MoverDieselEngineUnit::get_retarder_fill() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->hydro_R_Fill : 0.0;
    }

    double MoverDieselEngineUnit::get_max_rpm() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngineMaxRPM() : 0.0;
    }

    void MoverDieselEngineUnit::apply_configuration(const RailVehicleDieselEngine *p_engine) const {
        TMoverParameters *p_mover = owner.get_mover();

        p_mover->OilPump.pressure_minimum = p_engine->get_oil_pump_pressure_minimum();
        p_mover->OilPump.pressure_maximum = p_engine->get_oil_pump_pressure_maximum();
        p_mover->FuelPump.start_type =
                MaszynaMoverVehicleServer::start_mode_to_mover(p_engine->get_fuel_pump_start_mode());
        p_mover->OilPump.start_type =
                MaszynaMoverVehicleServer::start_mode_to_mover(p_engine->get_oil_pump_start_mode());
        p_mover->WaterPump.start_type =
                MaszynaMoverVehicleServer::start_mode_to_mover(p_engine->get_water_pump_start_mode());

        p_mover->dizel_nmin = p_engine->get_mechanical_min_rpm();
        p_mover->dizel_nmax = p_engine->get_mechanical_max_rpm();
        p_mover->TurboTest = p_engine->get_turbo_position();
        p_mover->dizel_nmax_cutoff = p_engine->get_mechanical_fuel_cutoff_rpm();
        p_mover->dizel_AIM = p_engine->get_mechanical_inertia();
        p_mover->engageupspeed = p_engine->get_mechanical_clutch_engage_speed();
        p_mover->engagedownspeed = p_engine->get_mechanical_clutch_disengage_speed();
        p_mover->dizel_nmin_hdrive = p_engine->get_mechanical_min_rpm_hydro_drive();
        p_mover->dizel_nmin_hdrive_factor = p_engine->get_mechanical_min_rpm_hydro_drive_factor();
        p_mover->dizel_nmin_retarder = p_engine->get_mechanical_min_rpm_retarder();
        p_mover->nmax = p_engine->get_mechanical_nominal_max_rpm();
        p_mover->dizel_nreg_acc = p_engine->get_mechanical_regulator_acceleration();
        p_mover->dizel_RevolutionsDecreaseRate = p_engine->get_mechanical_rpm_decrease_rate();
        p_mover->AnPos = p_engine->get_mechanical_shunt_mode_ratio();
        p_mover->dizel_minVelfullengage = p_engine->get_clutch_min_velocity_full_engage();
        p_mover->dizel_engageDia = p_engine->get_clutch_diameter();
        p_mover->dizel_engageMaxForce = p_engine->get_clutch_max_force();
        p_mover->dizel_engagefriction = p_engine->get_clutch_friction();
        p_mover->dizel_maxVelANS = p_engine->get_torque_converter_unlock_velocity();
        p_mover->hydro_R_EngageVel = p_engine->get_retarder_engage_velocity();
        p_mover->hydro_R_Clutch = p_engine->get_retarder_clutch();
        p_mover->hydro_R_ClutchSpeed = p_engine->get_retarder_clutch_speed();
        p_mover->hydro_R_WithIndividual = p_engine->get_retarder_with_individual();

        // LoadFIZ_Engine's cooling, shared by both diesel kinds (Mover.cpp:11340-11373 of the original)
        auto &heat = p_mover->dizel_heat;
        heat.kw = p_engine->get_cooling_heat_kw();
        heat.kv = p_engine->get_cooling_heat_kv();
        heat.kfe = p_engine->get_cooling_heat_kfe();
        heat.kfs = p_engine->get_cooling_heat_kfs();
        heat.kfo = p_engine->get_cooling_heat_kfo();
        heat.kfo2 = p_engine->get_cooling_heat_kfo2();
        heat.water.config.temp_min = static_cast<float>(p_engine->get_cooling_water_min_temperature());
        heat.water.config.temp_max = static_cast<float>(p_engine->get_cooling_water_max_temperature());
        heat.water.config.temp_flow = static_cast<float>(p_engine->get_cooling_water_flow_temperature());
        heat.water.config.temp_cooling = static_cast<float>(p_engine->get_cooling_water_cooling_temperature());
        heat.water.config.shutters = p_engine->get_cooling_water_shutters();
        heat.auxiliary_water_circuit = p_engine->get_cooling_water_aux_circuit();
        heat.water_aux.config.temp_min = static_cast<float>(p_engine->get_cooling_water_aux_min_temperature());
        heat.water_aux.config.temp_max = static_cast<float>(p_engine->get_cooling_water_aux_max_temperature());
        heat.water_aux.config.temp_cooling = static_cast<float>(p_engine->get_cooling_water_aux_cooling_temperature());
        heat.water_aux.config.shutters = p_engine->get_cooling_water_aux_shutters();
        heat.oil.config.temp_min = static_cast<float>(p_engine->get_cooling_oil_min_temperature());
        heat.oil.config.temp_max = static_cast<float>(p_engine->get_cooling_oil_max_temperature());
        heat.fan_speed = p_engine->get_cooling_fan_speed();
        p_mover->WaterHeater.config.temp_min = static_cast<float>(p_engine->get_cooling_heater_min_temperature());
        p_mover->WaterHeater.config.temp_max = static_cast<float>(p_engine->get_cooling_heater_max_temperature());
        heat.powerfactor = static_cast<float>(
                RailVehicleDieselEngine::NOMINAL_COOLING_POWER / p_engine->get_cooling_nominal_power());
        // LoadFIZ_Engine (Mover.cpp:11172-11203): derived from what was read
        p_mover->dizel_nreg_min = p_engine->get_mechanical_min_rpm() * NREG_MIN_SHARE;
        // the mechanical diesel's shunt gear (LoadFIZ_Engine(), Mover.cpp:11201-11205); a
        // diesel-electric's shunt mode is its own (Mover.cpp:11258-11262)
        if (p_mover->EngineType == TEngineType::DieselEngine) {
            p_mover->ShuntModeAllow = p_engine->get_mechanical_shunt_mode_ratio() > 0.0;
        }

        p_mover->hydro_TC = p_engine->get_torque_converter_present();
        p_mover->hydro_TC_TMMax = p_engine->get_torque_converter_max_torque_ratio();
        p_mover->hydro_TC_CouplingPoint = p_engine->get_torque_converter_coupling_point();
        p_mover->hydro_TC_LockupTorque = p_engine->get_torque_converter_lockup_torque();
        p_mover->hydro_TC_LockupRate = p_engine->get_torque_converter_lockup_rate();
        p_mover->hydro_TC_UnlockRate = p_engine->get_torque_converter_unlock_rate();
        p_mover->hydro_TC_FillRateInc = p_engine->get_torque_converter_fill_rate_increase();
        p_mover->hydro_TC_FillRateDec = p_engine->get_torque_converter_fill_rate_decrease();
        p_mover->hydro_TC_TorqueInIn = p_engine->get_torque_converter_torque_in_in();
        p_mover->hydro_TC_TorqueInOut = p_engine->get_torque_converter_torque_in_out();
        p_mover->hydro_TC_TorqueOutOut = p_engine->get_torque_converter_torque_out_out();
        p_mover->hydro_TC_LockupSpeed = p_engine->get_torque_converter_lockup_speed();
        p_mover->hydro_TC_UnlockSpeed = p_engine->get_torque_converter_unlock_speed();

        p_mover->hydro_TC_Table.clear();
        for (int i = 0; i < p_engine->get_torque_converter_table().size(); i++) {
            const Ref<VehicleCurvePointItem> &row = p_engine->get_torque_converter_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: p_engine->get_torque_converter_table() property is null at index " +
                        String::num(i));
                continue;
            }
            p_mover->hydro_TC_Table.emplace(row->get_x(), row->get_y());
        }

        p_mover->dizel_vel2nmax_Table.clear();
        for (int i = 0; i < p_engine->get_vel2nmax_table().size(); i++) {
            const Ref<VehicleCurvePointItem> &row = p_engine->get_vel2nmax_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: p_engine->get_vel2nmax_table() property is null at index " +
                        String::num(i));
                continue;
            }
            // matches readV2NMAXList (Mover.cpp:8476-8489): x unconverted, y (rpm) -> rev/s
            p_mover->dizel_vel2nmax_Table.emplace(row->get_x(), row->get_y() / LibMaszynaUnits::SECONDS_PER_MINUTE);
        }

        p_mover->hydro_R = p_engine->get_retarder_present();
        p_mover->hydro_R_Placement = p_engine->get_retarder_placement();
        p_mover->hydro_R_TorqueInIn = p_engine->get_retarder_torque_in_in();
        p_mover->hydro_R_MaxTorque = p_engine->get_retarder_max_torque();
        p_mover->hydro_R_MaxPower = p_engine->get_retarder_max_power();
        p_mover->hydro_R_FillRateInc = p_engine->get_retarder_fill_rate_increase();
        p_mover->hydro_R_FillRateDec = p_engine->get_retarder_fill_rate_decrease();
        p_mover->hydro_R_MinVel = p_engine->get_retarder_min_velocity();

        /* DList: tabela przepustnicy */
        p_mover->dizel_Mmax = p_engine->get_throttle_table_max_torque();
        p_mover->dizel_nMmax = p_engine->get_throttle_table_max_torque_rpm();
        p_mover->dizel_Mnmax = p_engine->get_throttle_table_max_rpm_torque();
        p_mover->dizel_nominalfill = p_engine->get_throttle_table_nominal_fuel_dose();
        p_mover->dizel_Mstand = p_engine->get_throttle_table_resistance_torque();
        p_mover->dizel_NominalFuelConsumptionRate = p_engine->get_throttle_table_nominal_fuel_consumption_rate();

        constexpr int MAX_THROTTLE_TABLE = Maszyna::ResArraySize + 1;
        const int throttle_table_size = static_cast<int>(p_engine->get_throttle_table_positions().size());
        if (throttle_table_size > MAX_THROTTLE_TABLE) {
            UtilityFunctions::push_warning(
                    "[RailVehicleDieselEngine]: p_engine->get_throttle_table_positions() has " +
                    String::num_int64(throttle_table_size) + " entries, exceeding the p_mover's limit of " +
                    String::num_int64(MAX_THROTTLE_TABLE) + "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_THROTTLE_TABLE, throttle_table_size); i++) {
            const Ref<RailVehicleThrottlePositionItem> &row = p_engine->get_throttle_table_positions()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: p_engine->get_throttle_table_positions() property is null at "
                        "index " +
                        String::num(i));
                continue;
            }
            p_mover->RList[i].Relay = row->get_throttle_position();
            p_mover->RList[i].R = row->get_fuel_dose();
            p_mover->RList[i].Mn = row->get_clutch_behavior();
        }

        /* DMList: charakterystyka momentu obrotowego silnika spalinowego */
        p_mover->dizel_Momentum_Table.clear();
        for (int i = 0; i < p_engine->get_torque_table().size(); i++) {
            const Ref<VehicleCurvePointItem> &row = p_engine->get_torque_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: p_engine->get_torque_table() property is null at index " +
                        String::num(i));
                continue;
            }
            p_mover->dizel_Momentum_Table.emplace(row->get_x() / LibMaszynaUnits::SECONDS_PER_MINUTE, row->get_y());
        }
    }

    // the rotation the running engine idles at [1/s], as engine_rpm_count (enrot) counts it: the one
    // the original's AI and spin-up compare with (Driver.cpp:6187, Mover.cpp:7897)
    double MoverDieselEngineUnit::get_idle_rpm_count() const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr) {
            return 0.0;
        }
        return p_mover->EngineType == TEngineType::DieselEngine
                       ? p_mover->dizel_nmin
                       : p_mover->DElist[0].RPM / LibMaszynaUnits::SECONDS_PER_MINUTE;
    }

    void MoverDieselEngineUnit::fill_config(Dictionary &p_config) const {
        if (owner.get_mover() == nullptr) {
            return;
        }
        p_config["engine_idle_rpm_count"] = get_idle_rpm_count();
    }

    void MoverDieselEngineUnit::oil_pump(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->OilPumpSwitch(p_enabled);
    }

    void MoverDieselEngineUnit::fuel_pump(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->FuelPumpSwitch(p_enabled);
    }

    void MoverDieselEngineUnit::oil_pump_switch_off(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->OilPumpSwitchOff(p_enabled);
    }

    void MoverDieselEngineUnit::fuel_pump_switch_off(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->FuelPumpSwitchOff(p_enabled);
    }

    bool MoverDieselEngineUnit::get_water_pump_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterPump.is_enabled : false;
    }

    bool MoverDieselEngineUnit::get_water_pump_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterPump.is_active : false;
    }

    bool MoverDieselEngineUnit::get_water_pump_breaker() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterPump.breaker : false;
    }

    bool MoverDieselEngineUnit::get_water_heater_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterHeater.is_enabled : false;
    }

    bool MoverDieselEngineUnit::get_water_heater_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterHeater.is_active : false;
    }

    bool MoverDieselEngineUnit::get_water_heater_breaker() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterHeater.breaker : false;
    }

    bool MoverDieselEngineUnit::get_water_circuits_link() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterCircuitsLink : false;
    }

    double MoverDieselEngineUnit::get_main_circuit_water_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.temperatura1 : 0.0;
    }

    double MoverDieselEngineUnit::get_auxiliary_circuit_water_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.temperatura2 : 0.0;
    }

    double MoverDieselEngineUnit::get_oil_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.To : 0.0;
    }

    void MoverDieselEngineUnit::water_pump(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterPumpSwitch(p_enabled);
    }

    void MoverDieselEngineUnit::water_pump_switch_off(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterPumpSwitchOff(p_enabled);
    }

    void MoverDieselEngineUnit::water_pump_breaker(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterPumpBreakerSwitch(p_enabled);
    }

    void MoverDieselEngineUnit::water_heater(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterHeaterSwitch(p_enabled);
    }

    void MoverDieselEngineUnit::water_heater_breaker(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterHeaterBreakerSwitch(p_enabled);
    }

    void MoverDieselEngineUnit::water_circuits_link(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterCircuitsLinkSwitch(p_enabled);
    }
} // namespace godot
