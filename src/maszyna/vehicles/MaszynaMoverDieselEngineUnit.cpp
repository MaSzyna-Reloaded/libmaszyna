#include "MaszynaMoverDieselEngineUnit.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"
#include "maszyna/vehicles/MaszynaMoverVehicleServer.hpp"
#include "utils/Units.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"

namespace godot {
    /* dizel_nreg_min = dizel_nmin * 0.98 (Mover.cpp:11173) */
    static constexpr double NREG_MIN_SHARE = 0.98;

    double MaszynaMoverDieselEngineUnit::get_rpm() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngineRPMRatio() * p_mover->EngineMaxRPM() : 0.0;
    }

    bool MaszynaMoverDieselEngineUnit::get_oil_pump_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.is_active : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_oil_pump_disabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.is_disabled : false;
    }

    double MaszynaMoverDieselEngineUnit::get_oil_pump_pressure() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.pressure : 0.0;
    }

    bool MaszynaMoverDieselEngineUnit::get_fuel_pump_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->FuelPump.is_active : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_fuel_pump_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->FuelPump.is_enabled : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_oil_pump_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->OilPump.is_enabled : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_heat_malfunction() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.PA : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_fuel_pump_disabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->FuelPump.is_disabled : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_startup() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_startup : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_ignition() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_ignition : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_spinup() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_spinup : false;
    }

    double MaszynaMoverDieselEngineUnit::get_output_power() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_Power : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_torque() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_Torque : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_fill() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_fill : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_fill_desired() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->RList[p_mover->MainCtrlPos].R : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_clutch_desired() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->RList[p_mover->MainCtrlPos].Mn : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_clutch_engagement() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_engage : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_water_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.Twy : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_engine_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.Ts : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_retarder_fill() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->hydro_R_Fill : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_max_rpm() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->EngineMaxRPM() : 0.0;
    }

    void MaszynaMoverDieselEngineUnit::apply_configuration(const RailVehicleDieselEngine *p_engine) const {
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
        p_mover->dizel_AIM = configuration->get_mechanical_inertia();
        p_mover->engageupspeed = configuration->get_mechanical_clutch_engage_speed();
        p_mover->engagedownspeed = configuration->get_mechanical_clutch_disengage_speed();
        p_mover->dizel_nmin_hdrive = configuration->get_mechanical_min_rpm_hydro_drive();
        p_mover->dizel_nmin_hdrive_factor = configuration->get_mechanical_min_rpm_hydro_drive_factor();
        p_mover->dizel_nmin_retarder = configuration->get_mechanical_min_rpm_retarder();
        p_mover->nmax = p_engine->get_mechanical_nominal_max_rpm();
        p_mover->dizel_nreg_acc = configuration->get_mechanical_regulator_acceleration();
        p_mover->dizel_RevolutionsDecreaseRate = configuration->get_mechanical_rpm_decrease_rate();
        p_mover->AnPos = p_engine->get_mechanical_shunt_mode_ratio();
        p_mover->dizel_minVelfullengage = configuration->get_clutch_min_velocity_full_engage();
        p_mover->dizel_engageDia = configuration->get_clutch_diameter();
        p_mover->dizel_engageMaxForce = configuration->get_clutch_max_force();
        p_mover->dizel_engagefriction = configuration->get_clutch_friction();
        p_mover->dizel_maxVelANS = configuration->get_torque_converter_unlock_velocity();
        p_mover->hydro_R_EngageVel = configuration->get_retarder_engage_velocity();
        p_mover->hydro_R_Clutch = p_engine->get_retarder_clutch();
        p_mover->hydro_R_ClutchSpeed = configuration->get_retarder_clutch_speed();
        p_mover->hydro_R_WithIndividual = p_engine->get_retarder_with_individual();

        // LoadFIZ_Engine's cooling, shared by both diesel kinds (Mover.cpp:11340-11373 of the original)
        auto &heat = p_mover->dizel_heat;
        heat.kw = configuration->get_cooling_heat_kw();
        heat.kv = configuration->get_cooling_heat_kv();
        heat.kfe = configuration->get_cooling_heat_kfe();
        heat.kfs = configuration->get_cooling_heat_kfs();
        heat.kfo = configuration->get_cooling_heat_kfo();
        heat.kfo2 = configuration->get_cooling_heat_kfo2();
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
        heat.fan_speed = configuration->get_cooling_fan_speed();
        p_mover->WaterHeater.config.temp_min = static_cast<float>(p_engine->get_cooling_heater_min_temperature());
        p_mover->WaterHeater.config.temp_max = static_cast<float>(p_engine->get_cooling_heater_max_temperature());
        heat.powerfactor = static_cast<float>(
                MaszynaMoverDieselConfiguration::NOMINAL_COOLING_POWER / configuration->get_cooling_nominal_power());
        // LoadFIZ_Engine (Mover.cpp:11172-11203): derived from what was read
        p_mover->dizel_nreg_min = p_engine->get_mechanical_min_rpm() * NREG_MIN_SHARE;
        // the mechanical diesel's shunt gear (LoadFIZ_Engine(), Mover.cpp:11201-11205); a
        // diesel-electric's shunt mode is its own (Mover.cpp:11258-11262)
        if (p_mover->EngineType == TEngineType::DieselEngine) {
            p_mover->ShuntModeAllow = p_engine->get_mechanical_shunt_mode_ratio() > 0.0;
        }

        p_mover->hydro_TC = p_engine->get_torque_converter_present();
        p_mover->hydro_TC_TMMax = configuration->get_torque_converter_max_torque_ratio();
        p_mover->hydro_TC_CouplingPoint = configuration->get_torque_converter_coupling_point();
        p_mover->hydro_TC_LockupTorque = configuration->get_torque_converter_lockup_torque();
        p_mover->hydro_TC_LockupRate = configuration->get_torque_converter_lockup_rate();
        p_mover->hydro_TC_UnlockRate = configuration->get_torque_converter_unlock_rate();
        p_mover->hydro_TC_FillRateInc = configuration->get_torque_converter_fill_rate_increase();
        p_mover->hydro_TC_FillRateDec = configuration->get_torque_converter_fill_rate_decrease();
        p_mover->hydro_TC_TorqueInIn = configuration->get_torque_converter_torque_in_in();
        p_mover->hydro_TC_TorqueInOut = configuration->get_torque_converter_torque_in_out();
        p_mover->hydro_TC_TorqueOutOut = configuration->get_torque_converter_torque_out_out();
        p_mover->hydro_TC_LockupSpeed = configuration->get_torque_converter_lockup_speed();
        p_mover->hydro_TC_UnlockSpeed = configuration->get_torque_converter_unlock_speed();

        p_mover->hydro_TC_Table.clear();
        for (int i = 0; i < configuration->get_torque_converter_table().size(); i++) {
            const Ref<VehicleCurvePointItem> &row = configuration->get_torque_converter_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: configuration->get_torque_converter_table() property is null at "
                        "index " +
                        String::num(i));
                continue;
            }
            p_mover->hydro_TC_Table.emplace(row->get_x(), row->get_y());
        }

        p_mover->dizel_vel2nmax_Table.clear();
        for (int i = 0; i < configuration->get_vel2nmax_table().size(); i++) {
            const Ref<VehicleCurvePointItem> &row = configuration->get_vel2nmax_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: configuration->get_vel2nmax_table() property is null at index " +
                        String::num(i));
                continue;
            }
            // matches readV2NMAXList (Mover.cpp:8476-8489): x unconverted, y (rpm) -> rev/s
            p_mover->dizel_vel2nmax_Table.emplace(row->get_x(), row->get_y() / Units::SECONDS_PER_MINUTE);
        }

        p_mover->hydro_R = p_engine->get_retarder_present();
        p_mover->hydro_R_Placement = configuration->get_retarder_placement();
        p_mover->hydro_R_TorqueInIn = configuration->get_retarder_torque_in_in();
        p_mover->hydro_R_MaxTorque = p_engine->get_retarder_max_torque();
        p_mover->hydro_R_MaxPower = p_engine->get_retarder_max_power();
        p_mover->hydro_R_FillRateInc = configuration->get_retarder_fill_rate_increase();
        p_mover->hydro_R_FillRateDec = configuration->get_retarder_fill_rate_decrease();
        p_mover->hydro_R_MinVel = configuration->get_retarder_min_velocity();

        /* DList: tabela przepustnicy */
        p_mover->dizel_Mmax = p_engine->get_throttle_table_max_torque();
        p_mover->dizel_nMmax = p_engine->get_throttle_table_max_torque_rpm();
        p_mover->dizel_Mnmax = p_engine->get_throttle_table_max_rpm_torque();
        p_mover->dizel_nominalfill = configuration->get_throttle_table_nominal_fuel_dose();
        p_mover->dizel_Mstand = configuration->get_throttle_table_resistance_torque();
        p_mover->dizel_NominalFuelConsumptionRate = p_engine->get_throttle_table_nominal_fuel_consumption_rate();

        constexpr int MAX_THROTTLE_TABLE = Maszyna::ResArraySize + 1;
        const int throttle_table_size = static_cast<int>(configuration->get_throttle_table_positions().size());
        if (throttle_table_size > MAX_THROTTLE_TABLE) {
            UtilityFunctions::push_warning(
                    "[RailVehicleDieselEngine]: configuration->get_throttle_table_positions() has " +
                    String::num_int64(throttle_table_size) + " entries, exceeding the p_mover's limit of " +
                    String::num_int64(MAX_THROTTLE_TABLE) + "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_THROTTLE_TABLE, throttle_table_size); i++) {
            const Ref<MaszynaMoverThrottlePositionItem> &row = configuration->get_throttle_table_positions()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[RailVehicleDieselEngine]: configuration->get_throttle_table_positions() property is null at "
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
            p_mover->dizel_Momentum_Table.emplace(row->get_x() / Units::SECONDS_PER_MINUTE, row->get_y());
        }
    }

    // the rotation the running engine idles at [1/s], as engine_rpm_count (enrot) counts it: the one
    // the original's AI and spin-up compare with (Driver.cpp:6187, Mover.cpp:7897)
    double MaszynaMoverDieselEngineUnit::get_idle_rpm_count() const {
        TMoverParameters *p_mover = owner.get_mover();
        if (p_mover == nullptr) {
            return 0.0;
        }
        return p_mover->EngineType == TEngineType::DieselEngine ? p_mover->dizel_nmin
                                                                : p_mover->DElist[0].RPM / Units::SECONDS_PER_MINUTE;
    }

    void MaszynaMoverDieselEngineUnit::fill_config(Dictionary &p_config) const {
        if (owner.get_mover() == nullptr) {
            return;
        }
        p_config["engine_idle_rpm_count"] = get_idle_rpm_count();
    }

    void MaszynaMoverDieselEngineUnit::fill_state(Dictionary &p_state) const {
        p_state["diesel_clutch_desired"] = get_clutch_desired();
    }

    void MaszynaMoverDieselEngineUnit::oil_pump(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->OilPumpSwitch(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::fuel_pump(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->FuelPumpSwitch(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::oil_pump_switch_off(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->OilPumpSwitchOff(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::fuel_pump_switch_off(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->FuelPumpSwitchOff(p_enabled);
    }

    bool MaszynaMoverDieselEngineUnit::get_water_pump_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterPump.is_enabled : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_water_pump_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterPump.is_active : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_water_pump_breaker() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterPump.breaker : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_water_heater_enabled() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterHeater.is_enabled : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_water_heater_active() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterHeater.is_active : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_water_heater_breaker() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterHeater.breaker : false;
    }

    bool MaszynaMoverDieselEngineUnit::get_water_circuits_link() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->WaterCircuitsLink : false;
    }

    double MaszynaMoverDieselEngineUnit::get_main_circuit_water_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.temperatura1 : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_auxiliary_circuit_water_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.temperatura2 : 0.0;
    }

    double MaszynaMoverDieselEngineUnit::get_oil_temperature() const {
        TMoverParameters *p_mover = owner.get_mover();
        return p_mover != nullptr ? p_mover->dizel_heat.To : 0.0;
    }

    void MaszynaMoverDieselEngineUnit::water_pump(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterPumpSwitch(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::water_pump_switch_off(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterPumpSwitchOff(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::water_pump_breaker(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterPumpBreakerSwitch(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::water_heater(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterHeaterSwitch(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::water_heater_breaker(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterHeaterBreakerSwitch(p_enabled);
    }

    void MaszynaMoverDieselEngineUnit::water_circuits_link(const bool p_enabled) const {
        TMoverParameters *p_mover = owner.get_mover();
        ASSERT_MOVER(p_mover);
        p_mover->WaterCircuitsLinkSwitch(p_enabled);
    }
} // namespace godot
