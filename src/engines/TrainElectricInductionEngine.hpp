#pragma once
#include "../maszyna/McZapkie/MOVER.h"
#include "TrainElectricEngine.hpp"
#include "macros.hpp"
#include "resources/engines/CurvePointItem.hpp"

namespace godot {
    class TrainController;

    class TrainElectricInductionEngine : public TrainElectricEngine {
            GDCLASS(TrainElectricInductionEngine, TrainElectricEngine)
        public:
            static void _bind_methods();

        protected:
            EngineType get_engine_type() override;
            void _do_update_internal_mover(TMoverParameters *p_mover) override;

        public:
            MAKE_MEMBER_GS(double, slip_current_ratio, 0.0);
            MAKE_MEMBER_GS(double, max_slip, 0.0);
            MAKE_MEMBER_GS(double, pole_pairs, 0.0);
            MAKE_MEMBER_GS(double, nominal_uf_ratio, 0.0);
            MAKE_MEMBER_GS(double, current_torque_ratio, 0.0);
            MAKE_MEMBER_GS(double, current_three_phase_ratio, 0.0);
            MAKE_MEMBER_GS(double, max_supply_voltage, 0.0);
            MAKE_MEMBER_GS(double, max_supply_voltage_braking, 0.0);
            MAKE_MEMBER_GS(double, inverter_voltage_drop, 0.0);
            MAKE_MEMBER_GS(double, no_load_current, 0.0);
            MAKE_MEMBER_GS(double, inverter_uf_setpoint, 0.0);
            MAKE_MEMBER_GS(double, inverter_uf_setpoint_braking, 0.0);
            MAKE_MEMBER_GS(double, initial_force, 0.0);
            MAKE_MEMBER_GS(double, force_drop_rate, 0.0);
            MAKE_MEMBER_GS(double, max_power, 0.0);
            MAKE_MEMBER_GS(double, max_braking_force, 0.0);
            MAKE_MEMBER_GS(double, max_braking_power, 0.0);
            MAKE_MEMBER_GS(double, braking_decay_velocity, 0.0);
            MAKE_MEMBER_GS(double, braking_decay_start_velocity, 0.0);
            MAKE_MEMBER_GS(double, motor_max_current, 0.0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<CurvePointItem>, max_power_table)
    };
} // namespace godot
