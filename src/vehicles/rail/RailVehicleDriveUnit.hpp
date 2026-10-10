#pragma once
#include "vehicles/rail/RailVehicleController.hpp"
#include <godot_cpp/variant/dictionary.hpp>

namespace godot {
    class RailVehicleEngine;

    /* The drive of an engine ("typ napedu", MOVER.h:1569): main switch, torque and force, power,
     * revolutions, the overload relay and the EIM controller - a unit every engine kind has.
     *
     * RailVehicleEngine is the component and carries the vehicle's authored configuration; this
     * interface is the unit it is composed of, with an implementation per simulation. Not a Godot
     * class - Godot sees only the engine. */
    class RailVehicleDriveUnit {
        public:
            virtual ~RailVehicleDriveUnit() = default;

            virtual bool get_main_switch_enabled() const = 0;
            virtual bool get_main_switch_closable() const = 0;
            virtual double get_motor_torque() const = 0;
            virtual double get_wheel_torque() const = 0;
            virtual double get_wheel_force() const = 0;
            virtual double get_tractive_force() const = 0;
            virtual double get_power() const = 0;
            virtual double get_rpm_count() const = 0;
            /* The engine's turn [rad, 0..2pi]: the revolutions summed (eAngle, Mover.cpp:5554) */
            virtual double get_angle() const = 0;
            virtual double get_rpm_ratio() const = 0;
            virtual double get_circuit_nmax_rpm() const = 0;
            virtual int get_damage() const = 0;
            virtual double get_main_switch_time() const = 0;
            virtual bool get_main_no_power_pos() const = 0;
            virtual bool get_motor_overload_relay_high_threshold() const = 0;
            /* The power the EIM controller actually asks for, 0..1, braking below (eimic_real) */
            virtual double get_eimic_real() const = 0;
            /* The main circuit's no-voltage, overvoltage and ground relays (NoVoltRelay,
             * OvervoltageRelay, GroundRelay) - what the main switch waits on (Mover.cpp:3361) */
            virtual bool get_relay_novolt() const = 0;
            virtual bool get_relay_overvoltage() const = 0;
            virtual bool get_relay_ground() const = 0;
            /* The rows of the starting resistor list (RlistSize, RList:/DList:/ffList:) */
            virtual int get_circuit_rlist_size() const = 0;
            /* What an ammeter of the engine shows [A]: 0 the total, 1 and 2 a motor branch
             * (ShowCurrent(), Mover.cpp:2345) */
            virtual double get_current(int p_ammeter) const = 0;
            virtual void apply_configuration(const RailVehicleEngine *p_engine) const = 0;
            /* The traction motors' blowers at an end: switched on, switched off, working
             * (MotorBlowers[].is_enabled, is_disabled, is_active) */
            virtual bool get_motor_blowers_enabled(RailVehicleController::CouplerEnd p_end) const = 0;
            virtual bool get_motor_blowers_disabled(RailVehicleController::CouplerEnd p_end) const = 0;
            virtual bool get_motor_blowers_active(RailVehicleController::CouplerEnd p_end) const = 0;
            virtual bool main_switch(bool p_enabled) const = 0;
            /* MotorBlowersSwitch(), MotorBlowersSwitchOff() (Mover.cpp) */
            virtual void motor_blowers(bool p_enabled, RailVehicleController::CouplerEnd p_end) const = 0;
            virtual void motor_blowers_switch_off(bool p_enabled, RailVehicleController::CouplerEnd p_end) const = 0;
            /* The motor overload relay's high threshold, or the shunting mode of an engine that has
             * one (CurrentSwitch(), Mover.cpp:805) */
            virtual bool motor_overload_relay_threshold(bool p_high) const = 0;
            /* One simulation step of the engine, for the work the simulation leaves to its owner. */
            virtual void process(const RailVehicleEngine *p_engine, double p_delta) const = 0;
            virtual void fill_config(Dictionary &p_config) const = 0;
    };
} // namespace godot
