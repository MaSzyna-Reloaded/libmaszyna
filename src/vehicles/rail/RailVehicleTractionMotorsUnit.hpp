#pragma once

namespace godot {
    /* The traction motors of an engine ("silniki trakcyjne"): their current, the overload relay
     * ("bezpiecznik nadmiarowy"), the line contactors ("styczniki liniowe"), dynamic braking and
     * the control pressure switch (MOVER.h:2104-2137).
     *
     * A unit two engine kinds are composed of - a catenary-fed locomotive and a diesel-electric -
     * with an implementation per simulation. Not a Godot class - Godot sees only the engine. */
    class RailVehicleTractionMotorsUnit {
        public:
            virtual ~RailVehicleTractionMotorsUnit() = default;

            /* Motor current (Im) */
            virtual double get_motor_current() const = 0;
            /* The voltage on the motors [V] (EngineVoltage) */
            virtual double get_engine_voltage() const = 0;
            /* The current the engine draws from the high voltage line [A] (Itot) */
            virtual double get_total_current() const = 0;
            /* Current limit of the traction circuit (Imax) */
            virtual double get_circuit_imax() const = 0;
            /* Rheostatic / regenerative braking is engaged */
            virtual bool get_dynamic_brake_active() const = 0;
            /* The motor overload relay has tripped */
            virtual bool get_fuse_active() const = 0;
            /* The line contactors are held open */
            virtual bool get_motor_connectors_open() const = 0;
            /* The line contactors are closed (StLinFlag) */
            virtual bool is_line_contactor_closed() const = 0;
            /* The control pressure switch tripped - the brake cylinder or pipe pressure is out of its
             * working range (ControlPressureSwitch, Mover.cpp:7177) */
            virtual bool is_pressure_switch_tripped() const = 0;

            /* "zbij nadmiarowy" - clears the overload relay */
            virtual void fuse_reset() const = 0;
            virtual void set_motor_connectors_open(bool p_open) const = 0;
    };
} // namespace godot
