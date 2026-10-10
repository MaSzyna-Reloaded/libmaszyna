#include "RailVehicleElectricInductionEngine.hpp"

namespace godot {
    void RailVehicleElectricInductionEngine::_bind_methods() {
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, slip_current_ratio);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_slip);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, pole_pairs);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, nominal_uf_ratio);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, current_torque_ratio);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, current_three_phase_ratio);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_supply_voltage);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_supply_voltage_braking);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, inverter_voltage_drop);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, no_load_current);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, inverter_uf_setpoint);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, inverter_uf_setpoint_braking);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, initial_force);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, force_drop_rate);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_power);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_braking_force);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, max_braking_power);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, braking_decay_velocity);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, braking_decay_start_velocity);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, motor_max_current);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, nominal_voltage);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, electrodynamic_brake_cylinder_ratio);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::FLOAT, electrodynamic_ep_ratio);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::BOOL, logarithmic_force_control);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::INT, inverter_control_coupler_flag);
        BIND_PROPERTY(RailVehicleElectricInductionEngine, Variant::BOOL, flat_force_characteristic);
        ClassDB::bind_method(D_METHOD("get_force_max"), &RailVehicleElectricInductionEngine::get_force_max);
        ClassDB::bind_method(D_METHOD("get_force_full"), &RailVehicleElectricInductionEngine::get_force_full);
        ClassDB::bind_method(D_METHOD("get_field_current"), &RailVehicleElectricInductionEngine::get_field_current);
        ClassDB::bind_method(D_METHOD("get_motor_voltage"), &RailVehicleElectricInductionEngine::get_motor_voltage);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleElectricInductionEngine, Variant::ARRAY, max_power_table, PROPERTY_HINT_TYPE_STRING,
                "VehicleCurvePointItem");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleElectricInductionEngine, Variant::ARRAY, wwlist, PROPERTY_HINT_TYPE_STRING,
                "RailVehicleWWListItem");
    }

    RailVehicleEngine::EngineType RailVehicleElectricInductionEngine::get_type() const {
        return RailVehicleEngine::EngineType::ELECTRIC_INDUCTION_MOTOR;
    }

    void RailVehicleElectricInductionEngine::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleElectricEngine::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["inverters"] = get_inverters();
        p_state["force_max"] = get_force_max();
        p_state["force_full"] = get_force_full();
        p_state["field_current"] = get_field_current();
        p_state["motor_voltage"] = get_motor_voltage();
    }

    void RailVehicleElectricInductionEngine::set_nominal_voltage(const double p_value) {
        nominal_voltage = p_value;
    }
    double RailVehicleElectricInductionEngine::get_nominal_voltage() const {
        return nominal_voltage;
    }
    void RailVehicleElectricInductionEngine::set_electrodynamic_brake_cylinder_ratio(const double p_value) {
        electrodynamic_brake_cylinder_ratio = p_value;
    }
    double RailVehicleElectricInductionEngine::get_electrodynamic_brake_cylinder_ratio() const {
        return electrodynamic_brake_cylinder_ratio;
    }
    void RailVehicleElectricInductionEngine::set_electrodynamic_ep_ratio(const double p_value) {
        electrodynamic_ep_ratio = p_value;
    }
    double RailVehicleElectricInductionEngine::get_electrodynamic_ep_ratio() const {
        return electrodynamic_ep_ratio;
    }
    void RailVehicleElectricInductionEngine::set_logarithmic_force_control(const bool p_value) {
        logarithmic_force_control = p_value;
    }
    bool RailVehicleElectricInductionEngine::get_logarithmic_force_control() const {
        return logarithmic_force_control;
    }
    void RailVehicleElectricInductionEngine::set_inverter_control_coupler_flag(const int p_value) {
        inverter_control_coupler_flag = p_value;
    }
    int RailVehicleElectricInductionEngine::get_inverter_control_coupler_flag() const {
        return inverter_control_coupler_flag;
    }
    void RailVehicleElectricInductionEngine::set_flat_force_characteristic(const bool p_value) {
        flat_force_characteristic = p_value;
    }
    bool RailVehicleElectricInductionEngine::get_flat_force_characteristic() const {
        return flat_force_characteristic;
    }
} // namespace godot
