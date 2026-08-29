#include "TrainElectricInductionEngine.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainElectricInductionEngine::_bind_methods() {
        BIND_PROPERTY(
                Variant::FLOAT, "slip_current_ratio", "slip_current_ratio",
                &TrainElectricInductionEngine::set_slip_current_ratio,
                &TrainElectricInductionEngine::get_slip_current_ratio, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "max_slip", "max_slip", &TrainElectricInductionEngine::set_max_slip,
                &TrainElectricInductionEngine::get_max_slip, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "pole_pairs", "pole_pairs", &TrainElectricInductionEngine::set_pole_pairs,
                &TrainElectricInductionEngine::get_pole_pairs, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "nominal_uf_ratio", "nominal_uf_ratio",
                &TrainElectricInductionEngine::set_nominal_uf_ratio,
                &TrainElectricInductionEngine::get_nominal_uf_ratio, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "current_torque_ratio", "current_torque_ratio",
                &TrainElectricInductionEngine::set_current_torque_ratio,
                &TrainElectricInductionEngine::get_current_torque_ratio, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "current_three_phase_ratio", "current_three_phase_ratio",
                &TrainElectricInductionEngine::set_current_three_phase_ratio,
                &TrainElectricInductionEngine::get_current_three_phase_ratio, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "max_supply_voltage", "max_supply_voltage",
                &TrainElectricInductionEngine::set_max_supply_voltage,
                &TrainElectricInductionEngine::get_max_supply_voltage, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "max_supply_voltage_braking", "max_supply_voltage_braking",
                &TrainElectricInductionEngine::set_max_supply_voltage_braking,
                &TrainElectricInductionEngine::get_max_supply_voltage_braking, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "inverter_voltage_drop", "inverter_voltage_drop",
                &TrainElectricInductionEngine::set_inverter_voltage_drop,
                &TrainElectricInductionEngine::get_inverter_voltage_drop, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "no_load_current", "no_load_current",
                &TrainElectricInductionEngine::set_no_load_current, &TrainElectricInductionEngine::get_no_load_current,
                "value");
        BIND_PROPERTY(
                Variant::FLOAT, "inverter_uf_setpoint", "inverter_uf_setpoint",
                &TrainElectricInductionEngine::set_inverter_uf_setpoint,
                &TrainElectricInductionEngine::get_inverter_uf_setpoint, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "inverter_uf_setpoint_braking", "inverter_uf_setpoint_braking",
                &TrainElectricInductionEngine::set_inverter_uf_setpoint_braking,
                &TrainElectricInductionEngine::get_inverter_uf_setpoint_braking, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "initial_force", "initial_force", &TrainElectricInductionEngine::set_initial_force,
                &TrainElectricInductionEngine::get_initial_force, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "force_drop_rate", "force_drop_rate", &TrainElectricInductionEngine::set_force_drop_rate,
                &TrainElectricInductionEngine::get_force_drop_rate, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "max_power", "max_power", &TrainElectricInductionEngine::set_max_power,
                &TrainElectricInductionEngine::get_max_power, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "max_braking_force", "max_braking_force",
                &TrainElectricInductionEngine::set_max_braking_force,
                &TrainElectricInductionEngine::get_max_braking_force, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "max_braking_power", "max_braking_power",
                &TrainElectricInductionEngine::set_max_braking_power,
                &TrainElectricInductionEngine::get_max_braking_power, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "braking_decay_velocity", "braking_decay_velocity",
                &TrainElectricInductionEngine::set_braking_decay_velocity,
                &TrainElectricInductionEngine::get_braking_decay_velocity, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "braking_decay_start_velocity", "braking_decay_start_velocity",
                &TrainElectricInductionEngine::set_braking_decay_start_velocity,
                &TrainElectricInductionEngine::get_braking_decay_start_velocity, "value");
        BIND_PROPERTY(
                Variant::FLOAT, "motor_max_current", "motor_max_current",
                &TrainElectricInductionEngine::set_motor_max_current,
                &TrainElectricInductionEngine::get_motor_max_current, "value");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "max_power_table", "max_power_table", &TrainElectricInductionEngine::set_max_power_table,
                &TrainElectricInductionEngine::get_max_power_table, "max_power_table", PROPERTY_HINT_TYPE_STRING,
                "CurvePointItem");
    }

    TrainEngine::EngineType TrainElectricInductionEngine::get_engine_type() {
        return TrainEngine::EngineType::ELECTRIC_INDUCTION_MOTOR;
    }

    void TrainElectricInductionEngine::_do_update_internal_mover(TMoverParameters *p_mover) {
        TrainElectricEngine::_do_update_internal_mover(p_mover);

        p_mover->eimc[Maszyna::eimc_s_dfic] = slip_current_ratio;
        p_mover->eimc[Maszyna::eimc_s_dfmax] = max_slip;
        p_mover->eimc[Maszyna::eimc_s_p] = pole_pairs;
        p_mover->eimc[Maszyna::eimc_s_cfu] = nominal_uf_ratio;
        p_mover->eimc[Maszyna::eimc_s_cim] = current_torque_ratio;
        p_mover->eimc[Maszyna::eimc_s_icif] = current_three_phase_ratio;
        p_mover->eimc[Maszyna::eimc_f_Uzmax] = max_supply_voltage;
        p_mover->eimc[Maszyna::eimc_f_Uzh] = max_supply_voltage_braking;
        p_mover->eimc[Maszyna::eimc_f_DU] = inverter_voltage_drop;
        p_mover->eimc[Maszyna::eimc_f_I0] = no_load_current;
        p_mover->eimc[Maszyna::eimc_f_cfu] = inverter_uf_setpoint;
        p_mover->eimc[Maszyna::eimc_f_cfuH] = inverter_uf_setpoint_braking;
        p_mover->eimc[Maszyna::eimc_p_F0] = initial_force;
        p_mover->eimc[Maszyna::eimc_p_a1] = force_drop_rate;
        p_mover->eimc[Maszyna::eimc_p_Pmax] = max_power;
        p_mover->eimc[Maszyna::eimc_p_Fh] = max_braking_force;
        p_mover->eimc[Maszyna::eimc_p_Ph] = max_braking_power;
        p_mover->eimc[Maszyna::eimc_p_Vh0] = braking_decay_velocity;
        p_mover->eimc[Maszyna::eimc_p_Vh1] = braking_decay_start_velocity;
        p_mover->eimc[Maszyna::eimc_p_Imax] = motor_max_current;

        /* Pmaxlist: tabela mocy maksymalnej od predkosci (niedokumentowana na wiki, patrz EIM_Pmax_Table w MOVER.h) */
        p_mover->EIM_Pmax_Table.clear();
        for (int i = 0; i < max_power_table.size(); i++) {
            const Ref<CurvePointItem> &row = max_power_table[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[TrainElectricInductionEngine]: max_power_table property is null at index " + String::num(i));
                continue;
            }
            p_mover->EIM_Pmax_Table.emplace(row->get_x(), row->get_y());
        }
    }
} // namespace godot
