#include "RailVehicleCircuitUnit.hpp"
#include "RailVehicleElectricEngine.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>

#include <algorithm>
#include <cmath>

namespace godot {
    bool RailVehicleElectricEngine::get_contactors_active() const {
        return circuit_unit != nullptr ? circuit_unit->get_contactors_active() : false;
    }
    bool RailVehicleElectricEngine::get_diff_relay_active() const {
        return circuit_unit != nullptr ? circuit_unit->get_diff_relay_active() : false;
    }
    bool RailVehicleElectricEngine::get_resistors_active() const {
        return circuit_unit != nullptr ? circuit_unit->get_resistors_active() : false;
    }
    bool RailVehicleElectricEngine::get_vent_overload_active() const {
        return circuit_unit != nullptr ? circuit_unit->get_vent_overload_active() : false;
    }
    bool RailVehicleElectricEngine::get_highcurrent_active() const {
        return circuit_unit != nullptr ? circuit_unit->get_highcurrent_active() : false;
    }
    bool RailVehicleElectricEngine::get_mainbreaker_active() const {
        return circuit_unit != nullptr ? circuit_unit->get_mainbreaker_active() : false;
    }
    bool RailVehicleElectricEngine::get_camshaft_available() const {
        return circuit_unit != nullptr ? circuit_unit->get_camshaft_available() : false;
    }
    bool RailVehicleElectricEngine::get_converter_overload() const {
        return circuit_unit != nullptr ? circuit_unit->get_converter_overload() : false;
    }
    double RailVehicleElectricEngine::get_line_breaker_delay() const {
        return circuit_unit != nullptr ? circuit_unit->get_line_breaker_delay() : 0.0;
    }
    double RailVehicleElectricEngine::get_line_breaker_initial_delay() const {
        return circuit_unit != nullptr ? circuit_unit->get_line_breaker_initial_delay() : 0.0;
    }
    bool RailVehicleElectricEngine::get_line_breaker_closes_at_no_power() const {
        return circuit_unit != nullptr ? circuit_unit->get_line_breaker_closes_at_no_power() : false;
    }
    void RailVehicleElectricEngine::_apply_configuration() {
        RailVehicleEngine::_apply_configuration();
        if (circuit_unit != nullptr) {
            circuit_unit->apply_configuration(this);
        }
    }

    void RailVehicleElectricEngine::_bind_methods() {
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_resistance, "circuit");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::INT, circuit_imax_low, "circuit");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::INT, circuit_imax_high, "circuit");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::INT, circuit_imin_low, "circuit");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::INT, circuit_imin_high, "circuit");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_sum, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_diff, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_min_current, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_max_current, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::INT, circuit_tuhex_stages, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_sum_1, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_sum_2, "circuit/tuhex");
        BIND_PROPERTY(RailVehicleElectricEngine, Variant::FLOAT, circuit_tuhex_sum_3, "circuit/tuhex");
        BIND_PROPERTY_W_HINT(
                RailVehicleElectricEngine, Variant::INT, cntrl_converter_overload_relay_start_mode, "cntrl",
                PROPERTY_HINT_ENUM, "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                RailVehicleElectricEngine, Variant::BOOL, cntrl_converter_overload_relay_off_when_main_is_off, "cntrl");
        BIND_PROPERTY_W_HINT(
                RailVehicleElectricEngine, Variant::INT, cntrl_main_switch_start_mode, "cntrl", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        ClassDB::bind_method(D_METHOD("converter_fuse_reset"), &RailVehicleElectricEngine::converter_fuse_reset);
        ClassDB::bind_method(
                D_METHOD("is_line_contactor_closed"), &RailVehicleElectricEngine::is_line_contactor_closed);
        ClassDB::bind_method(
                D_METHOD("is_pressure_switch_tripped"), &RailVehicleElectricEngine::is_pressure_switch_tripped);
        ClassDB::bind_method(D_METHOD("fuse_reset"), &RailVehicleElectricEngine::fuse_reset);
        ClassDB::bind_method(
                D_METHOD("set_motor_connectors_open", "open"), &RailVehicleElectricEngine::set_motor_connectors_open);
        ClassDB::bind_method(D_METHOD("get_contactors_active"), &RailVehicleElectricEngine::get_contactors_active);
        ClassDB::bind_method(D_METHOD("get_diff_relay_active"), &RailVehicleElectricEngine::get_diff_relay_active);
        ClassDB::bind_method(D_METHOD("get_resistors_active"), &RailVehicleElectricEngine::get_resistors_active);
        ClassDB::bind_method(
                D_METHOD("get_vent_overload_active"), &RailVehicleElectricEngine::get_vent_overload_active);
        ClassDB::bind_method(D_METHOD("get_highcurrent_active"), &RailVehicleElectricEngine::get_highcurrent_active);
        ClassDB::bind_method(D_METHOD("get_mainbreaker_active"), &RailVehicleElectricEngine::get_mainbreaker_active);

        ClassDB::bind_method(D_METHOD("get_camshaft_available"), &RailVehicleElectricEngine::get_camshaft_available);
        ClassDB::bind_method(D_METHOD("get_converter_overload"), &RailVehicleElectricEngine::get_converter_overload);
        ClassDB::bind_method(D_METHOD("get_line_breaker_delay"), &RailVehicleElectricEngine::get_line_breaker_delay);
        ClassDB::bind_method(
                D_METHOD("get_line_breaker_initial_delay"), &RailVehicleElectricEngine::get_line_breaker_initial_delay);
        ClassDB::bind_method(
                D_METHOD("get_line_breaker_closes_at_no_power"),
                &RailVehicleElectricEngine::get_line_breaker_closes_at_no_power);
        ClassDB::bind_method(D_METHOD("get_motor_current"), &RailVehicleElectricEngine::get_motor_current);
        ClassDB::bind_method(D_METHOD("get_engine_voltage"), &RailVehicleElectricEngine::get_engine_voltage);
        ClassDB::bind_method(D_METHOD("get_total_current"), &RailVehicleElectricEngine::get_total_current);
        ClassDB::bind_method(D_METHOD("get_circuit_imax"), &RailVehicleElectricEngine::get_circuit_imax);
        ClassDB::bind_method(
                D_METHOD("get_dynamic_brake_active"), &RailVehicleElectricEngine::get_dynamic_brake_active);
        ClassDB::bind_method(D_METHOD("get_fuse_active"), &RailVehicleElectricEngine::get_fuse_active);
        ClassDB::bind_method(
                D_METHOD("get_motor_connectors_open"), &RailVehicleElectricEngine::get_motor_connectors_open);
    }


    void RailVehicleElectricEngine::_fill_state_dictionary(Dictionary &p_state) const {
        RailVehicleEngine::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["camshaft_available"] = get_camshaft_available();
        p_state["converter_overload"] = get_converter_overload();
        p_state["line_breaker_delay"] = get_line_breaker_delay();
        p_state["line_breaker_initial_delay"] = get_line_breaker_initial_delay();
        p_state["line_breaker_closes_at_no_power"] = get_line_breaker_closes_at_no_power();
        p_state["motor_current"] = get_motor_current();
        p_state["engine_voltage"] = get_engine_voltage();
        p_state["total_current"] = get_total_current();
        p_state["circuit_imax"] = get_circuit_imax();
        p_state["dynamic_brake_active"] = get_dynamic_brake_active();
        p_state["fuse_active"] = get_fuse_active();
        p_state["motor_connectors_open"] = get_motor_connectors_open();
        p_state["line_contactor_closed"] = is_line_contactor_closed();
        p_state["pressure_switch_tripped"] = is_pressure_switch_tripped();
        p_state["indicators/contactors_active"] = get_contactors_active();
        p_state["indicators/diff_relay_active"] = get_diff_relay_active();
        p_state["indicators/resistors_active"] = get_resistors_active();
        p_state["indicators/vent_overload_active"] = get_vent_overload_active();
        p_state["indicators/highcurrent_active"] = get_highcurrent_active();
        p_state["indicators/mainbreaker_active"] = get_mainbreaker_active();
    }


    double RailVehicleElectricEngine::get_motor_current() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_motor_current() : 0.0;
    }

    double RailVehicleElectricEngine::get_engine_voltage() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_engine_voltage() : 0.0;
    }

    double RailVehicleElectricEngine::get_total_current() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_total_current() : 0.0;
    }

    double RailVehicleElectricEngine::get_circuit_imax() const {
        return traction_motors_unit != nullptr ? traction_motors_unit->get_circuit_imax() : 0.0;
    }

    bool RailVehicleElectricEngine::get_dynamic_brake_active() const {
        return traction_motors_unit != nullptr && traction_motors_unit->get_dynamic_brake_active();
    }

    bool RailVehicleElectricEngine::get_fuse_active() const {
        return traction_motors_unit != nullptr && traction_motors_unit->get_fuse_active();
    }

    bool RailVehicleElectricEngine::get_motor_connectors_open() const {
        return traction_motors_unit != nullptr && traction_motors_unit->get_motor_connectors_open();
    }

    bool RailVehicleElectricEngine::is_line_contactor_closed() const {
        return traction_motors_unit != nullptr && traction_motors_unit->is_line_contactor_closed();
    }

    bool RailVehicleElectricEngine::is_pressure_switch_tripped() const {
        return traction_motors_unit != nullptr && traction_motors_unit->is_pressure_switch_tripped();
    }

    void RailVehicleElectricEngine::fuse_reset() {
        if (traction_motors_unit != nullptr) {
            traction_motors_unit->fuse_reset();
        }
    }

    void RailVehicleElectricEngine::set_motor_connectors_open(const bool p_open) {
        if (traction_motors_unit != nullptr) {
            traction_motors_unit->set_motor_connectors_open(p_open);
        }
    }

    void RailVehicleElectricEngine::converter_fuse_reset() {
        if (circuit_unit != nullptr) {
            circuit_unit->converter_fuse_reset();
        }
    }

    void RailVehicleElectricEngine::_register_commands() {
        RailVehicleEngine::_register_commands();
        register_command("converter_fuse_reset", Callable(this, "converter_fuse_reset"));
        register_command("fuse_reset", Callable(this, "fuse_reset"));
        register_command("motor_connectors_open", Callable(this, "set_motor_connectors_open"));
    }

    void RailVehicleElectricEngine::_unregister_commands() {
        RailVehicleEngine::_unregister_commands();
        unregister_command("converter_fuse_reset");
        unregister_command("fuse_reset");
        unregister_command("motor_connectors_open");
    }


} // namespace godot
