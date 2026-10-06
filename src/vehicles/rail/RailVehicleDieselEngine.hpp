#pragma once
#include "RailVehicleDieselEngineUnit.hpp"
#include "RailVehicleEngine.hpp"
#include "macros.hpp"
#include "vehicles/base/VehicleCurvePointItem.hpp"
#include "vehicles/rail/RailVehicleThrottlePositionItem.hpp"

namespace godot {
    class VehicleController;

    class RailVehicleDieselEngine : public RailVehicleEngine {
            GDCLASS(RailVehicleDieselEngine, RailVehicleEngine)

        protected:
            /* The diesel engine unit, installed by the implementation that owns it */
            const RailVehicleDieselEngineUnit *diesel_engine_unit = nullptr;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Live state, read straight from the engine's units - nothing is stored. */
            double get_rpm() const;
            bool get_oil_pump_active() const;
            bool get_oil_pump_disabled() const;
            double get_oil_pump_pressure() const;
            bool get_fuel_pump_active() const;
            bool get_fuel_pump_disabled() const;
            /* The engine overheat alarm (dizel_heat.PA, the i-malfunction lamp, Train.cpp:9210) */
            bool get_heat_malfunction() const;
            /* The pump switches' own state (FuelPump/OilPump.is_enabled) - what a two-state
             * switch flips (Train.cpp:3891, 3990) */
            bool get_fuel_pump_enabled() const;
            bool get_oil_pump_enabled() const;
            bool get_startup() const;
            bool get_ignition() const;
            bool get_spinup() const;
            double get_output_power() const;
            double get_torque() const;
            double get_fill() const;
            double get_fill_desired() const;
            double get_clutch_desired() const;
            double get_clutch_engagement() const;
            double get_water_temperature() const;
            double get_engine_temperature() const;
            double get_retarder_fill() const;
            double get_max_rpm() const;
            double get_idle_rpm_count() const;
            /* The cooling water's pump, its breaker, the water heater, its breaker and the link of
             * the two water circuits (WaterPump, WaterHeater, WaterCircuitsLink) */
            bool get_water_pump_enabled() const;
            bool get_water_pump_active() const;
            bool get_water_pump_breaker() const;
            bool get_water_heater_enabled() const;
            bool get_water_heater_active() const;
            bool get_water_heater_breaker() const;
            bool get_water_circuits_link() const;
            /* The main and the auxiliary water circuit's and the oil's temperature [C] - what the
             * original's driver warms the engine up by (PrepareHeating(), Driver.cpp:5040-5045) */
            double get_main_circuit_water_temperature() const;
            double get_auxiliary_circuit_water_temperature() const;
            double get_oil_temperature() const;

            /* Original engine: Mover.cpp:11371 NominalCoolingPower's default - su45's, the engine
             * the heat model was written for */
            static constexpr double NOMINAL_COOLING_POWER = 1235.0;

            /* R_Place= : retarder location within the mechanical transmission */
            enum RetarderPlacement {
                RETARDER_PLACEMENT_AFTER_GEARBOX,
                RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC,
                RETARDER_PLACEMENT_BETWEEN_TC_AND_ENGINE,
            };

        private:
            static void _bind_methods();
            int turbo_position = 0;

        public:
            /* TurboPos: - the master controller position from which the turbocharger is heard; 0
             * without it (LoadFIZ_TurboPos, Mover.cpp:10714; DynObj.cpp:8267) */
            void set_turbo_position(int p_value);
            int get_turbo_position() const;

        private:
            MAKE_MEMBER_GS(float, oil_pump_pressure_minimum, 0.0);
            MAKE_MEMBER_GS(float, oil_pump_pressure_maximum, 0.65);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, fuel_pump_start_mode, RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, oil_pump_start_mode, RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, water_pump_start_mode, RailVehicleController::START_MODE_MANUAL);

            /* Engine: (Kont.), przekladnia mechaniczna */
            MAKE_MEMBER_GS(double, mechanical_min_rpm, 0.0);
            MAKE_MEMBER_GS(double, mechanical_max_rpm, 0.0);
            MAKE_MEMBER_GS(double, mechanical_fuel_cutoff_rpm, 0.0);
            MAKE_MEMBER_GS(double, mechanical_inertia, 1.0);
            MAKE_MEMBER_GS(double, mechanical_clutch_engage_speed, 0.5);
            MAKE_MEMBER_GS(double, mechanical_clutch_disengage_speed, 0.9);
            /* nmin_hdrive [1/s]: the idle speed while driving on the torque converter */
            MAKE_MEMBER_GS(double, mechanical_min_rpm_hydro_drive, 0.0);
            /* nmin_hdrive_factor [1/s]: its rise with the power asked for */
            MAKE_MEMBER_GS(double, mechanical_min_rpm_hydro_drive_factor, 0.0);
            /* nmin_retarder [1/s]: the speed while the retarder brakes */
            MAKE_MEMBER_GS(double, mechanical_min_rpm_retarder, 0.0);
            /* Engine: nmax [1/s]: the engine's top speed (not DList's) */
            MAKE_MEMBER_GS(double, mechanical_nominal_max_rpm, 0.0);
            /* nreg_acc [1/s2]: how fast the governor raises the speed */
            MAKE_MEMBER_GS(double, mechanical_regulator_acceleration, 999.0);
            /* RPMDecRate */
            MAKE_MEMBER_GS(double, mechanical_rpm_decrease_rate, 2.0);
            /* ShuntMode: the extra gear of a shunting mode (2Ls150), 0 without one */
            MAKE_MEMBER_GS(double, mechanical_shunt_mode_ratio, 0.0);
            /* minVelfullengage [km/h]: the clutch holds without slip above it */
            MAKE_MEMBER_GS(double, clutch_min_velocity_full_engage, 0.0);
            /* engageDia [m] */
            MAKE_MEMBER_GS(double, clutch_diameter, 0.5);
            /* engageMaxForce [N] */
            MAKE_MEMBER_GS(double, clutch_max_force, 6000.0);
            /* engagefriction */
            MAKE_MEMBER_GS(double, clutch_friction, 0.5);
            /* MaxVelANS [km/h]: the converter disengages below it */
            MAKE_MEMBER_GS(double, torque_converter_unlock_velocity, 3.0);
            /* R_EngageVel [km/h] */
            MAKE_MEMBER_GS(double, retarder_engage_velocity, 1.0);
            /* R_IsClutch: the retarder has its own clutch */
            MAKE_MEMBER_GS(bool, retarder_clutch, false);
            /* R_ClutchSpeed */
            MAKE_MEMBER_GS(double, retarder_clutch_speed, 10.0);
            /* R_WithIndividual */
            MAKE_MEMBER_GS(bool, retarder_with_individual, false);
            MAKE_MEMBER_GS(bool, torque_converter_present, false);
            MAKE_MEMBER_GS(double, torque_converter_max_torque_ratio, 2.0);
            MAKE_MEMBER_GS(double, torque_converter_coupling_point, 0.85);
            MAKE_MEMBER_GS(double, torque_converter_lockup_torque, 3000.0);
            MAKE_MEMBER_GS(double, torque_converter_lockup_rate, 1.0);
            MAKE_MEMBER_GS(double, torque_converter_unlock_rate, 1.0);
            MAKE_MEMBER_GS(double, torque_converter_fill_rate_increase, 1.0);
            MAKE_MEMBER_GS(double, torque_converter_fill_rate_decrease, 1.0);
            MAKE_MEMBER_GS(double, torque_converter_torque_in_in, 4.5);
            MAKE_MEMBER_GS(double, torque_converter_torque_in_out, 0.0);
            MAKE_MEMBER_GS(double, torque_converter_torque_out_out, 0.0);
            MAKE_MEMBER_GS(double, torque_converter_lockup_speed, 1.0);
            MAKE_MEMBER_GS(double, torque_converter_unlock_speed, 1.0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<VehicleCurvePointItem>, torque_converter_table)
            /* V2NList: predkosc -> maksymalne obroty silnika (dizel_vel2nmax_Table) */
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<VehicleCurvePointItem>, vel2nmax_table)
            MAKE_MEMBER_GS(bool, retarder_present, false);
            MAKE_MEMBER_GS_NR(RetarderPlacement, retarder_placement, RETARDER_PLACEMENT_AFTER_GEARBOX);
            MAKE_MEMBER_GS(double, retarder_torque_in_in, 1.0);
            MAKE_MEMBER_GS(double, retarder_max_torque, 1.0);
            MAKE_MEMBER_GS(double, retarder_max_power, 1.0);
            MAKE_MEMBER_GS(double, retarder_fill_rate_increase, 1.0);
            MAKE_MEMBER_GS(double, retarder_fill_rate_decrease, 1.0);
            MAKE_MEMBER_GS(double, retarder_min_velocity, 1.0);

            /* DList: tabela przepustnicy */
            MAKE_MEMBER_GS(double, throttle_table_max_torque, 1.0);
            MAKE_MEMBER_GS(double, throttle_table_max_torque_rpm, 1.0);
            MAKE_MEMBER_GS(double, throttle_table_max_rpm_torque, 2.0);
            MAKE_MEMBER_GS(double, throttle_table_nominal_fuel_dose, 0.0);
            MAKE_MEMBER_GS(double, throttle_table_resistance_torque, 0.0);
            MAKE_MEMBER_GS(double, throttle_table_nominal_fuel_consumption_rate, 250.0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleThrottlePositionItem>, throttle_table_positions)

            /* DMList: charakterystyka momentu obrotowego */
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<VehicleCurvePointItem>, torque_table)

            /* Engine: cooling (dizel_heat, LoadFIZ_Engine, Mover.cpp:11340-11373 of the original);
             * a temperature of -1 [deg C] is not set */
            /* HeatKW, HeatKV, HeatKFE, HeatKFS, HeatKFO, HeatKFO2: heat exchange factors */
            MAKE_MEMBER_GS(double, cooling_heat_kw, 0.35);
            MAKE_MEMBER_GS(double, cooling_heat_kv, 0.6);
            MAKE_MEMBER_GS(double, cooling_heat_kfe, 1.0);
            MAKE_MEMBER_GS(double, cooling_heat_kfs, 80.0);
            MAKE_MEMBER_GS(double, cooling_heat_kfo, 25.0);
            MAKE_MEMBER_GS(double, cooling_heat_kfo2, 25.0);
            /* WaterMin/Max/Flow/CoolingTemperature [deg C], WaterShutters */
            MAKE_MEMBER_GS(double, cooling_water_min_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_water_max_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_water_flow_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_water_cooling_temperature, -1.0);
            MAKE_MEMBER_GS(bool, cooling_water_shutters, false);
            /* WaterAuxCircuit: the cooling has a second water circuit, WaterAux*: its settings */
            MAKE_MEMBER_GS(bool, cooling_water_aux_circuit, false);
            MAKE_MEMBER_GS(double, cooling_water_aux_min_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_water_aux_max_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_water_aux_cooling_temperature, -1.0);
            MAKE_MEMBER_GS(bool, cooling_water_aux_shutters, false);
            /* OilMin/MaxTemperature [deg C] */
            MAKE_MEMBER_GS(double, cooling_oil_min_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_oil_max_temperature, -1.0);
            /* WaterCoolingFanSpeed: a share of the engine's revolutions, or absolute when negative */
            MAKE_MEMBER_GS(double, cooling_fan_speed, 0.075);
            /* HeaterMin/MaxTemperature [deg C]: the water heater */
            MAKE_MEMBER_GS(double, cooling_heater_min_temperature, -1.0);
            MAKE_MEMBER_GS(double, cooling_heater_max_temperature, -1.0);
            /* NominalCoolingPower: the engine's heat is scaled by su45's 1235 over it */
            MAKE_MEMBER_GS(double, cooling_nominal_power, NOMINAL_COOLING_POWER);

        private:
        protected:
            EngineType get_type() const override;
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            void oil_pump(bool p_enabled);
            void fuel_pump(bool p_enabled);
            /* The "off" side of a two-state pump switch (FuelPumpSwitchOff/OilPumpSwitchOff,
             * Train.cpp:3937, 3963) - an impulse switch has none */
            void oil_pump_switch_off(bool p_enabled);
            void fuel_pump_switch_off(bool p_enabled);
            /* WaterPumpSwitch(), WaterPumpBreakerSwitch(), WaterHeaterSwitch(),
             * WaterHeaterBreakerSwitch(), WaterCircuitsLinkSwitch() (Mover.cpp) */
            void water_pump(bool p_enabled);
            /* The "off" side of a two-state water pump switch (WaterPumpSwitchOff, Train.cpp:4312) */
            void water_pump_switch_off(bool p_enabled);
            void water_pump_breaker(bool p_enabled);
            void water_heater(bool p_enabled);
            void water_heater_breaker(bool p_enabled);
            void water_circuits_link(bool p_enabled);
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleDieselEngine::RetarderPlacement)
