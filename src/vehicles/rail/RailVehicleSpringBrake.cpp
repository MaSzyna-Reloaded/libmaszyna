#include "RailVehicleSpringBrake.hpp"

namespace godot {
    void RailVehicleSpringBrake::_bind_methods() {
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, spring_actuator_chamber_volume, "spring/actuator")
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, spring_actuator_max_filling_force, "spring/actuator")
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, pressure_force_coefficient)
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, spring_actuator_preload_pressure, "spring/actuator")
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, spring_full_balance_pressure, "spring")
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, brake_signal_released_state_pressure, "brake_signal")
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::FLOAT, brake_signal_braked_state_pressure, "brake_signal")
        BIND_PROPERTY(
                RailVehicleSpringBrake, Variant::FLOAT, valve_cross_section_actuator_discharge, "valve_cross_section")
        BIND_PROPERTY(
                RailVehicleSpringBrake, Variant::FLOAT, valve_cross_section_actuator_charge, "valve_cross_section")
        BIND_PROPERTY(
                RailVehicleSpringBrake, Variant::FLOAT, valve_cross_section_pneumatic_brake, "valve_cross_section")
        BIND_PROPERTY(RailVehicleSpringBrake, Variant::INT, required_coupler_connection_method)

        ClassDB::bind_method(
                D_METHOD("spring_brake_activate", "active"), &RailVehicleSpringBrake::spring_brake_activate);
        ClassDB::bind_method(
                D_METHOD("spring_brake_shut_off", "shut_off"), &RailVehicleSpringBrake::spring_brake_shut_off);
        ClassDB::bind_method(D_METHOD("spring_brake_release"), &RailVehicleSpringBrake::spring_brake_release);

        ClassDB::bind_method(D_METHOD("get_ready"), &RailVehicleSpringBrake::get_ready);
        ClassDB::bind_method(D_METHOD("get_shut_off"), &RailVehicleSpringBrake::get_shut_off);
        ClassDB::bind_method(D_METHOD("get_active"), &RailVehicleSpringBrake::get_active);
        ClassDB::bind_method(D_METHOD("get_braking"), &RailVehicleSpringBrake::get_braking);
        ClassDB::bind_method(D_METHOD("get_cylinder_pressure"), &RailVehicleSpringBrake::get_cylinder_pressure);
    }

    void RailVehicleSpringBrake::_register_commands() {
        register_command("spring_brake_activate", callable_mp(this, &RailVehicleSpringBrake::spring_brake_activate));
        register_command("spring_brake_shut_off", callable_mp(this, &RailVehicleSpringBrake::spring_brake_shut_off));
        register_command("spring_brake_release", callable_mp(this, &RailVehicleSpringBrake::spring_brake_release));
    }

    void RailVehicleSpringBrake::_unregister_commands() {
        unregister_command("spring_brake_activate");
        unregister_command("spring_brake_shut_off");
        unregister_command("spring_brake_release");
    }
} // namespace godot
