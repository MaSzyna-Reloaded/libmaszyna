#include "MaszynaMoverRailVehicleElectricInductionEngine.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MaszynaMoverRailVehicleElectricInductionEngine::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("get_circuit_nmax_rpm"),
                &MaszynaMoverRailVehicleElectricInductionEngine::get_circuit_nmax_rpm);
        ClassDB::bind_method(
                D_METHOD("get_eimic_real"), &MaszynaMoverRailVehicleElectricInductionEngine::get_eimic_real);
        ClassDB::bind_method(
                D_METHOD("get_engine_configuration"),
                &MaszynaMoverRailVehicleElectricInductionEngine::get_engine_configuration);
        ClassDB::bind_method(
                D_METHOD("set_engine_configuration", "value"),
                &MaszynaMoverRailVehicleElectricInductionEngine::set_engine_configuration);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "engine_configuration", PROPERTY_HINT_RESOURCE_TYPE,
                        "MaszynaMoverEngineConfiguration"),
                "set_engine_configuration", "get_engine_configuration");
        ClassDB::bind_method(
                D_METHOD("get_electric_engine_configuration"),
                &MaszynaMoverRailVehicleElectricInductionEngine::get_electric_engine_configuration);
        ClassDB::bind_method(
                D_METHOD("set_electric_engine_configuration", "value"),
                &MaszynaMoverRailVehicleElectricInductionEngine::set_electric_engine_configuration);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "electric_engine_configuration", PROPERTY_HINT_RESOURCE_TYPE,
                        "MaszynaMoverElectricEngineConfiguration"),
                "set_electric_engine_configuration", "get_electric_engine_configuration");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverRailVehicleElectricInductionEngine, Variant::ARRAY, wwlist, PROPERTY_HINT_TYPE_STRING,
                "MaszynaMoverWWListItem");
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, slip_current_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, max_slip);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, nominal_uf_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, current_torque_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, current_three_phase_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, max_supply_voltage);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, max_supply_voltage_braking);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, inverter_voltage_drop);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, no_load_current);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, inverter_uf_setpoint);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, inverter_uf_setpoint_braking);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, force_drop_rate);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, braking_decay_velocity);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, braking_decay_start_velocity);
        BIND_PROPERTY(
                MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, electrodynamic_brake_cylinder_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::FLOAT, electrodynamic_ep_ratio);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::BOOL, logarithmic_force_control);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::INT, inverter_control_coupler_flag);
        BIND_PROPERTY(MaszynaMoverRailVehicleElectricInductionEngine, Variant::BOOL, flat_force_characteristic);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverRailVehicleElectricInductionEngine, Variant::ARRAY, max_power_table,
                PROPERTY_HINT_TYPE_STRING, "VehicleCurvePointItem");
    }

    TypedArray<RailVehicleInverter> MaszynaMoverRailVehicleElectricInductionEngine::get_inverters() const {
        TypedArray<RailVehicleInverter> inverters;
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return inverters;
        }
        for (const Maszyna::inverter &inverter: mover->Inverters) {
            Ref<RailVehicleInverter> state;
            state.instantiate();
            state->set_active(inverter.IsActive);
            state->set_error(inverter.Error);
            state->set_allow(inverter.Activate);
            inverters.append(state);
        }
        return inverters;
    }

    double MaszynaMoverRailVehicleElectricInductionEngine::get_force_max() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->eimv[Maszyna::eimv_Fmax] : 0.0;
    }

    double MaszynaMoverRailVehicleElectricInductionEngine::get_force_full() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->eimv[Maszyna::eimv_Fful] : 0.0;
    }

    double MaszynaMoverRailVehicleElectricInductionEngine::get_field_current() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->eimv[Maszyna::eimv_If] : 0.0;
    }

    double MaszynaMoverRailVehicleElectricInductionEngine::get_motor_voltage() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->eimv[Maszyna::eimv_U] : 0.0;
    }

    void MaszynaMoverRailVehicleElectricInductionEngine::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        RailVehicleElectricInductionEngine::_apply_configuration();

        p_mover->eimc[Maszyna::eimc_s_dfic] = get_slip_current_ratio();
        p_mover->eimc[Maszyna::eimc_s_dfmax] = get_max_slip();
        p_mover->eimc[Maszyna::eimc_s_p] = get_pole_pairs();
        p_mover->eimc[Maszyna::eimc_s_cfu] = get_nominal_uf_ratio();
        p_mover->eimc[Maszyna::eimc_s_cim] = get_current_torque_ratio();
        p_mover->eimc[Maszyna::eimc_s_icif] = get_current_three_phase_ratio();
        p_mover->eimc[Maszyna::eimc_f_Uzmax] = get_max_supply_voltage();
        p_mover->eimc[Maszyna::eimc_f_Uzh] = get_max_supply_voltage_braking();
        p_mover->eimc[Maszyna::eimc_f_DU] = get_inverter_voltage_drop();
        p_mover->eimc[Maszyna::eimc_f_I0] = get_no_load_current();
        p_mover->eimc[Maszyna::eimc_f_cfu] = get_inverter_uf_setpoint();
        p_mover->eimc[Maszyna::eimc_f_cfuH] = get_inverter_uf_setpoint_braking();
        p_mover->eimc[Maszyna::eimc_p_F0] = get_initial_force();
        p_mover->eimc[Maszyna::eimc_p_a1] = get_force_drop_rate();
        p_mover->eimc[Maszyna::eimc_p_Pmax] = get_max_power();
        p_mover->eimc[Maszyna::eimc_p_Fh] = get_max_braking_force();
        p_mover->eimc[Maszyna::eimc_p_Ph] = get_max_braking_power();
        p_mover->eimc[Maszyna::eimc_p_Vh0] = get_braking_decay_velocity();
        p_mover->eimc[Maszyna::eimc_p_Vh1] = get_braking_decay_start_velocity();
        p_mover->eimc[Maszyna::eimc_p_Imax] = get_motor_max_current();
        // the rest of LoadFIZ_Engine's induction motor block (Mover.cpp:11276-11306); edep defaults
        // to eimc[eimc_p_eped] = 1.5 (Mover.cpp:497), InvCtrCplFlag to InverterControlCouplerFlag{4}
        p_mover->eimc[Maszyna::eimc_p_abed] = get_electrodynamic_brake_cylinder_ratio();
        p_mover->eimc[Maszyna::eimc_p_eped] = get_electrodynamic_ep_ratio();
        p_mover->NominalVoltage = get_nominal_voltage();
        p_mover->EIMCLogForce = get_logarithmic_force_control();
        p_mover->InverterControlCouplerFlag = get_inverter_control_coupler_flag();
        p_mover->Flat = get_flat_force_characteristic();
        // Mover.cpp:11302 - a powered EIM without InvNo has one inverter; with none the traction
        // step divides by InvertersNo (Mover.cpp:5627) and every force becomes NaN
        if (p_mover->eimc[Maszyna::eimc_p_Pmax] > 0 && p_mover->Power > 0 && p_mover->InvertersNo == 0) {
            p_mover->InvertersNo = 1;
        }
        p_mover->Inverters.resize(p_mover->InvertersNo);

        /* Pmaxlist: tabela mocy maksymalnej od predkosci (niedokumentowana na wiki, patrz EIM_Pmax_Table w MOVER.h) */
        p_mover->EIM_Pmax_Table.clear();
        for (int i = 0; i < get_max_power_table().size(); i++) {
            const Ref<VehicleCurvePointItem> &row = get_max_power_table()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[MaszynaMoverRailVehicleElectricInductionEngine]: max_power_table property is null at index " +
                        String::num(i));
                continue;
            }
            p_mover->EIM_Pmax_Table.emplace(row->get_x(), row->get_y());
        }

        /* ffList:/ffBrakeList: -> DElist/RlistSize, read by TractionForce()'s
         * ElectricInductionMotor branch to compute InverterFrequency (Mover.cpp:5895-5908).
         * RlistSize mirrors RailVehicleElectricSeriesEngine's own RList:-driven wiring exactly - it's
         * a single mover-wide field, so only one engine part may legitimately drive it. */
        const int max_delist = sizeof(p_mover->DElist) / sizeof(Maszyna::TDEScheme);
        const int wwlist_size = static_cast<int>(get_wwlist().size());
        p_mover->RlistSize = std::min(max_delist, wwlist_size);
        for (int i = 0; i < p_mover->RlistSize; i++) {
            const Ref<MaszynaMoverWWListItem> &row = get_wwlist()[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[MaszynaMoverRailVehicleElectricInductionEngine]: wwlist property is null at index " +
                        String::num(i));
                continue;
            }
            p_mover->DElist[i].RPM = row->get_rpm();
            p_mover->DElist[i].GenPower = row->get_max_power();
        }
    }

    void MaszynaMoverRailVehicleElectricInductionEngine::set_electrodynamic_brake_cylinder_ratio(const double p_value) {
        electrodynamic_brake_cylinder_ratio = p_value;
    }
    double MaszynaMoverRailVehicleElectricInductionEngine::get_electrodynamic_brake_cylinder_ratio() const {
        return electrodynamic_brake_cylinder_ratio;
    }
    void MaszynaMoverRailVehicleElectricInductionEngine::set_electrodynamic_ep_ratio(const double p_value) {
        electrodynamic_ep_ratio = p_value;
    }
    double MaszynaMoverRailVehicleElectricInductionEngine::get_electrodynamic_ep_ratio() const {
        return electrodynamic_ep_ratio;
    }
    void MaszynaMoverRailVehicleElectricInductionEngine::set_logarithmic_force_control(const bool p_value) {
        logarithmic_force_control = p_value;
    }
    bool MaszynaMoverRailVehicleElectricInductionEngine::get_logarithmic_force_control() const {
        return logarithmic_force_control;
    }
    void MaszynaMoverRailVehicleElectricInductionEngine::set_inverter_control_coupler_flag(const int p_value) {
        inverter_control_coupler_flag = p_value;
    }
    int MaszynaMoverRailVehicleElectricInductionEngine::get_inverter_control_coupler_flag() const {
        return inverter_control_coupler_flag;
    }
    void MaszynaMoverRailVehicleElectricInductionEngine::set_flat_force_characteristic(const bool p_value) {
        flat_force_characteristic = p_value;
    }
    bool MaszynaMoverRailVehicleElectricInductionEngine::get_flat_force_characteristic() const {
        return flat_force_characteristic;
    }

    Ref<MaszynaMoverEngineConfiguration>
    MaszynaMoverRailVehicleElectricInductionEngine::get_engine_configuration() const {
        return engine_configuration;
    }

    void MaszynaMoverRailVehicleElectricInductionEngine::set_engine_configuration(
            const Ref<MaszynaMoverEngineConfiguration> &p_value) {
        engine_configuration = p_value;
    }

    Ref<MaszynaMoverElectricEngineConfiguration>
    MaszynaMoverRailVehicleElectricInductionEngine::get_electric_engine_configuration() const {
        return electric_engine_configuration;
    }

    void MaszynaMoverRailVehicleElectricInductionEngine::set_electric_engine_configuration(
            const Ref<MaszynaMoverElectricEngineConfiguration> &p_value) {
        electric_engine_configuration = p_value;
    }

    double MaszynaMoverRailVehicleElectricInductionEngine::get_circuit_nmax_rpm() const {
        return drive_unit_impl.get_circuit_nmax_rpm();
    }

    double MaszynaMoverRailVehicleElectricInductionEngine::get_eimic_real() const {
        return drive_unit_impl.get_eimic_real();
    }
} // namespace godot
