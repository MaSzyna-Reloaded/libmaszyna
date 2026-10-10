#include "RailVehicleDieselElectricEngine.hpp"

namespace godot {
    void RailVehicleDieselElectricEngine::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleDieselEngine::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["motor_current"] = get_motor_current();
        p_state["engine_voltage"] = get_engine_voltage();
        p_state["circuit_imax"] = get_circuit_imax();
        p_state["dynamic_brake_active"] = get_dynamic_brake_active();
        p_state["fuse_active"] = get_fuse_active();
        p_state["motor_connectors_open"] = get_motor_connectors_open();
        p_state["line_contactor_closed"] = is_line_contactor_closed();
        p_state["pressure_switch_tripped"] = is_pressure_switch_tripped();
    }

    void RailVehicleDieselElectricEngine::_bind_methods() {
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleDieselElectricEngine, Variant::ARRAY, wwlist, PROPERTY_HINT_TYPE_STRING,
                "RailVehicleWWListItem");
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::BOOL, generator_voltage_flat);
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::FLOAT, hyperbolic_speed);
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::FLOAT, additional_speed);
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::FLOAT, power_correction_ratio);
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::INT, shunt_relay_type);
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::BOOL, shunt_mode_allowed);
        BIND_PROPERTY(RailVehicleDieselElectricEngine, Variant::FLOAT, heating_rpm);

        ClassDB::bind_method(D_METHOD("get_motor_current"), &RailVehicleDieselElectricEngine::get_motor_current);
        ClassDB::bind_method(D_METHOD("get_engine_voltage"), &RailVehicleDieselElectricEngine::get_engine_voltage);
        ClassDB::bind_method(D_METHOD("get_circuit_imax"), &RailVehicleDieselElectricEngine::get_circuit_imax);
        ClassDB::bind_method(
                D_METHOD("get_dynamic_brake_active"), &RailVehicleDieselElectricEngine::get_dynamic_brake_active);
        ClassDB::bind_method(D_METHOD("get_fuse_active"), &RailVehicleDieselElectricEngine::get_fuse_active);
        ClassDB::bind_method(
                D_METHOD("get_motor_connectors_open"), &RailVehicleDieselElectricEngine::get_motor_connectors_open);
        ClassDB::bind_method(
                D_METHOD("is_line_contactor_closed"), &RailVehicleDieselElectricEngine::is_line_contactor_closed);
        ClassDB::bind_method(
                D_METHOD("is_pressure_switch_tripped"), &RailVehicleDieselElectricEngine::is_pressure_switch_tripped);
        ClassDB::bind_method(D_METHOD("fuse_reset"), &RailVehicleDieselElectricEngine::fuse_reset);
        ClassDB::bind_method(
                D_METHOD("set_motor_connectors_open", "open"),
                &RailVehicleDieselElectricEngine::set_motor_connectors_open);
    }

    double RailVehicleDieselElectricEngine::get_motor_current() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_motor_current() : 0.0;
    }

    double RailVehicleDieselElectricEngine::get_engine_voltage() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_engine_voltage() : 0.0;
    }

    double RailVehicleDieselElectricEngine::get_circuit_imax() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_circuit_imax() : 0.0;
    }

    bool RailVehicleDieselElectricEngine::get_dynamic_brake_active() const {
        return traction_motors_unit != nullptr && traction_motors_unit->get_dynamic_brake_active();
    }

    bool RailVehicleDieselElectricEngine::get_fuse_active() const {
        return traction_motors_unit != nullptr && traction_motors_unit->get_fuse_active();
    }

    bool RailVehicleDieselElectricEngine::get_motor_connectors_open() const {
        return traction_motors_unit != nullptr && traction_motors_unit->get_motor_connectors_open();
    }

    bool RailVehicleDieselElectricEngine::is_line_contactor_closed() const {
        return traction_motors_unit != nullptr && traction_motors_unit->is_line_contactor_closed();
    }

    bool RailVehicleDieselElectricEngine::is_pressure_switch_tripped() const {
        return traction_motors_unit != nullptr && traction_motors_unit->is_pressure_switch_tripped();
    }

    void RailVehicleDieselElectricEngine::fuse_reset() {
        if (traction_motors_unit != nullptr) {
            traction_motors_unit->fuse_reset();
        }
    }

    void RailVehicleDieselElectricEngine::set_motor_connectors_open(const bool p_open) {
        if (traction_motors_unit != nullptr) {
            traction_motors_unit->set_motor_connectors_open(p_open);
        }
    }

    void RailVehicleDieselElectricEngine::_register_commands() {
        RailVehicleDieselEngine::_register_commands();
        register_command("fuse_reset", Callable(this, "fuse_reset"));
        register_command("motor_connectors_open", Callable(this, "set_motor_connectors_open"));
    }

    void RailVehicleDieselElectricEngine::_unregister_commands() {
        RailVehicleDieselEngine::_unregister_commands();
        unregister_command("fuse_reset");
        unregister_command("motor_connectors_open");
    }

    RailVehicleEngine::EngineType RailVehicleDieselElectricEngine::get_type() const {
        return RailVehicleEngine::EngineType::DIESEL_ELECTRIC;
    }
} // namespace godot
