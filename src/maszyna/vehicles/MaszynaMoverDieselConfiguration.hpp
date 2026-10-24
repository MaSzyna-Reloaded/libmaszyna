#pragma once
#include "macros.hpp"
#include "maszyna/vehicles/MaszynaMoverThrottlePositionItem.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"

#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /* What the Mover's model of a diesel engine is configured with: its governor, clutch, torque converter, retarder,
     * cooling and throttle tables (LoadFIZ_Engine for DieselEngine). Shared by the diesel and the diesel-electric
     * adapters and read by MaszynaMoverDieselEngineUnit. */
    class MaszynaMoverDieselConfiguration : public Resource {
            GDCLASS(MaszynaMoverDieselConfiguration, Resource);

        public:
            /* Original engine: Mover.cpp:11371 NominalCoolingPower's default - su45's, the engine
             * the heat model was written for */
            static constexpr double NOMINAL_COOLING_POWER = 1235.0;
            /* R_Place= : retarder location within the mechanical transmission */
            enum RetarderPlacement {
                RETARDER_PLACEMENT_AFTER_GEARBOX,
                RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC,
                RETARDER_PLACEMENT_BETWEEN_TC_AND_ENGINE,
            };

        protected:
            static void _bind_methods();

        private:
            double mechanical_clutch_engage_speed = 0.5;
            double mechanical_clutch_disengage_speed = 0.9;
            /* nmin_hdrive_factor [1/s]: its rise with the power asked for */
            double mechanical_min_rpm_hydro_drive_factor = 0.0;
            /* nreg_acc [1/s2]: how fast the governor raises the speed */
            double mechanical_regulator_acceleration = 999.0;
            /* RPMDecRate */
            double mechanical_rpm_decrease_rate = 2.0;
            /* minVelfullengage [km/h]: the clutch holds without slip above it */
            double clutch_min_velocity_full_engage = 0.0;
            /* engageDia [m] */
            double clutch_diameter = 0.5;
            /* engageMaxForce [N] */
            double clutch_max_force = 6000.0;
            /* engagefriction */
            double clutch_friction = 0.5;
            /* MaxVelANS [km/h]: the converter disengages below it */
            double torque_converter_unlock_velocity = 3.0;
            /* R_EngageVel [km/h] */
            double retarder_engage_velocity = 1.0;
            /* R_ClutchSpeed */
            double retarder_clutch_speed = 10.0;
            double torque_converter_max_torque_ratio = 2.0;
            double torque_converter_coupling_point = 0.85;
            double torque_converter_lockup_torque = 3000.0;
            double torque_converter_lockup_rate = 1.0;
            double torque_converter_unlock_rate = 1.0;
            double torque_converter_fill_rate_increase = 1.0;
            double torque_converter_fill_rate_decrease = 1.0;
            double torque_converter_torque_in_in = 4.5;
            double torque_converter_torque_in_out = 0.0;
            double torque_converter_torque_out_out = 0.0;
            double torque_converter_lockup_speed = 1.0;
            double torque_converter_unlock_speed = 1.0;
            /* V2NList: predkosc -> maksymalne obroty silnika (dizel_vel2nmax_Table) */
            TypedArray<VehicleCurvePointItem> vel2nmax_table;
            double retarder_torque_in_in = 1.0;
            double retarder_fill_rate_increase = 1.0;
            double retarder_fill_rate_decrease = 1.0;
            double retarder_min_velocity = 1.0;
            double throttle_table_nominal_fuel_dose = 0.0;
            TypedArray<MaszynaMoverThrottlePositionItem> throttle_table_positions;
            /* Engine: cooling (dizel_heat, LoadFIZ_Engine, Mover.cpp:11340-11373 of the original);
             * a temperature of -1 [deg C] is not set */
            /* HeatKW, HeatKV, HeatKFE, HeatKFS, HeatKFO, HeatKFO2: heat exchange factors */
            double cooling_heat_kw = 0.35;
            double cooling_heat_kv = 0.6;
            double cooling_heat_kfe = 1.0;
            double cooling_heat_kfs = 80.0;
            double cooling_heat_kfo = 25.0;
            double cooling_heat_kfo2 = 25.0;
            /* WaterCoolingFanSpeed: a share of the engine's revolutions, or absolute when negative */
            double cooling_fan_speed = 0.075;
            /* NominalCoolingPower: the engine's heat is scaled by su45's 1235 over it */
            double cooling_nominal_power = NOMINAL_COOLING_POWER;
            double mechanical_inertia = 1.0;
            /* nmin_hdrive [1/s]: the idle speed while driving on the torque converter */
            double mechanical_min_rpm_hydro_drive = 0.0;
            /* nmin_retarder [1/s]: the speed while the retarder brakes */
            double mechanical_min_rpm_retarder = 0.0;
            TypedArray<VehicleCurvePointItem> torque_converter_table;
            RetarderPlacement retarder_placement = RETARDER_PLACEMENT_AFTER_GEARBOX;
            double throttle_table_resistance_torque = 0.0;

        public:
            MAKE_GS(double, mechanical_clutch_engage_speed);
            MAKE_GS(double, mechanical_clutch_disengage_speed);
            MAKE_GS(double, mechanical_min_rpm_hydro_drive_factor);
            MAKE_GS(double, mechanical_regulator_acceleration);
            MAKE_GS(double, mechanical_rpm_decrease_rate);
            MAKE_GS(double, clutch_min_velocity_full_engage);
            MAKE_GS(double, clutch_diameter);
            MAKE_GS(double, clutch_max_force);
            MAKE_GS(double, clutch_friction);
            MAKE_GS(double, torque_converter_unlock_velocity);
            MAKE_GS(double, retarder_engage_velocity);
            MAKE_GS(double, retarder_clutch_speed);
            MAKE_GS(double, torque_converter_max_torque_ratio);
            MAKE_GS(double, torque_converter_coupling_point);
            MAKE_GS(double, torque_converter_lockup_torque);
            MAKE_GS(double, torque_converter_lockup_rate);
            MAKE_GS(double, torque_converter_unlock_rate);
            MAKE_GS(double, torque_converter_fill_rate_increase);
            MAKE_GS(double, torque_converter_fill_rate_decrease);
            MAKE_GS(double, torque_converter_torque_in_in);
            MAKE_GS(double, torque_converter_torque_in_out);
            MAKE_GS(double, torque_converter_torque_out_out);
            MAKE_GS(double, torque_converter_lockup_speed);
            MAKE_GS(double, torque_converter_unlock_speed);
            MAKE_GS_NR(TypedArray<VehicleCurvePointItem>, vel2nmax_table);
            MAKE_GS(double, retarder_torque_in_in);
            MAKE_GS(double, retarder_fill_rate_increase);
            MAKE_GS(double, retarder_fill_rate_decrease);
            MAKE_GS(double, retarder_min_velocity);
            MAKE_GS(double, throttle_table_nominal_fuel_dose);
            MAKE_GS_NR(TypedArray<MaszynaMoverThrottlePositionItem>, throttle_table_positions);

            MAKE_GS(double, cooling_heat_kw);
            MAKE_GS(double, cooling_heat_kv);
            MAKE_GS(double, cooling_heat_kfe);
            MAKE_GS(double, cooling_heat_kfs);
            MAKE_GS(double, cooling_heat_kfo);
            MAKE_GS(double, cooling_heat_kfo2);
            MAKE_GS(double, cooling_fan_speed);
            MAKE_GS(double, cooling_nominal_power);
            MAKE_GS(double, mechanical_inertia);
            MAKE_GS(double, mechanical_min_rpm_hydro_drive);
            MAKE_GS(double, mechanical_min_rpm_retarder);
            MAKE_GS_NR(TypedArray<VehicleCurvePointItem>, torque_converter_table);
            MAKE_GS_NR(RetarderPlacement, retarder_placement);
            MAKE_GS(double, throttle_table_resistance_torque);
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaMoverDieselConfiguration::RetarderPlacement)
