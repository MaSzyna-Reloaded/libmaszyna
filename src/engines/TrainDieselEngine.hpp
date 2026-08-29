#pragma once
#include "../maszyna/McZapkie/MOVER.h"
#include "TrainEngine.hpp"
#include "macros.hpp"
#include "resources/engines/CurvePointItem.hpp"
#include "resources/engines/ThrottlePositionItem.hpp"

namespace godot {
    class TrainController;

    class TrainDieselEngine : public TrainEngine {
            GDCLASS(TrainDieselEngine, TrainEngine)
        public:
            /* R_Place= : retarder location within the mechanical transmission */
            enum RetarderPlacement {
                RETARDER_PLACEMENT_AFTER_GEARBOX,
                RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC,
                RETARDER_PLACEMENT_BETWEEN_TC_AND_ENGINE,
            };

        private:
            static void _bind_methods();
            MAKE_MEMBER_GS(float, oil_min_pressure, 0.0);
            MAKE_MEMBER_GS(float, oil_max_pressure, 0.65);
            MAKE_MEMBER_GS_NR(StartMode, fuel_pump_start_mode, START_MODE_MANUAL);
            MAKE_MEMBER_GS_NR(StartMode, oil_pump_start_mode, START_MODE_MANUAL);
            MAKE_MEMBER_GS_NR(StartMode, water_pump_start_mode, START_MODE_MANUAL);

            /* Engine: (Kont.), przekladnia mechaniczna */
            MAKE_MEMBER_GS(double, min_rpm, 0.0);
            MAKE_MEMBER_GS(double, max_rpm, 0.0);
            MAKE_MEMBER_GS(double, fuel_cutoff_rpm, 0.0);
            MAKE_MEMBER_GS(double, inertia, 1.0);
            MAKE_MEMBER_GS(double, clutch_engage_speed, 0.5);
            MAKE_MEMBER_GS(double, clutch_disengage_speed, 0.9);
            MAKE_MEMBER_GS(bool, has_torque_converter, false);
            MAKE_MEMBER_GS(double, tc_max_torque_ratio, 2.0);
            MAKE_MEMBER_GS(double, tc_coupling_point, 0.85);
            MAKE_MEMBER_GS(double, tc_lockup_torque, 3000.0);
            MAKE_MEMBER_GS(double, tc_lockup_rate, 1.0);
            MAKE_MEMBER_GS(double, tc_unlock_rate, 1.0);
            MAKE_MEMBER_GS(double, tc_fill_rate_increase, 1.0);
            MAKE_MEMBER_GS(double, tc_fill_rate_decrease, 1.0);
            MAKE_MEMBER_GS(double, tc_torque_in_in, 4.5);
            MAKE_MEMBER_GS(double, tc_torque_in_out, 0.0);
            MAKE_MEMBER_GS(double, tc_torque_out_out, 0.0);
            MAKE_MEMBER_GS(double, tc_lockup_speed, 1.0);
            MAKE_MEMBER_GS(double, tc_unlock_speed, 1.0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<CurvePointItem>, torque_converter_table)
            MAKE_MEMBER_GS(bool, has_retarder, false);
            MAKE_MEMBER_GS_NR(RetarderPlacement, retarder_placement, RETARDER_PLACEMENT_AFTER_GEARBOX);
            MAKE_MEMBER_GS(double, retarder_torque_in_in, 1.0);
            MAKE_MEMBER_GS(double, retarder_max_torque, 1.0);
            MAKE_MEMBER_GS(double, retarder_max_power, 1.0);
            MAKE_MEMBER_GS(double, retarder_fill_rate_increase, 1.0);
            MAKE_MEMBER_GS(double, retarder_fill_rate_decrease, 1.0);
            MAKE_MEMBER_GS(double, retarder_min_velocity, 1.0);

            /* DList: tabela przepustnicy */
            MAKE_MEMBER_GS(double, max_torque, 1.0);
            MAKE_MEMBER_GS(double, max_torque_rpm, 1.0);
            MAKE_MEMBER_GS(double, max_rpm_torque, 2.0);
            MAKE_MEMBER_GS(double, nominal_fuel_dose, 0.0);
            MAKE_MEMBER_GS(double, resistance_torque, 0.0);
            MAKE_MEMBER_GS(double, nominal_fuel_consumption_rate, 250.0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<ThrottlePositionItem>, throttle_table)

            /* DMList: charakterystyka momentu obrotowego */
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<CurvePointItem>, torque_table)

        protected:
            EngineType get_engine_type() override;
            void _do_update_internal_mover(TMoverParameters *p_mover) override;
            void _do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            void oil_pump(bool p_enabled);
            void fuel_pump(bool p_enabled);
    };
} // namespace godot
VARIANT_ENUM_CAST(TrainDieselEngine::RetarderPlacement)
