#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class VehicleController;
    class RailVehicleSpeedControl : public RailVehicleComponent {
            GDCLASS(RailVehicleSpeedControl, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_SPEED_CONTROL;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_active() const = 0;
            virtual double get_desired_velocity() const = 0;
            virtual double get_desired_power() const = 0;
            virtual double get_selected_velocity() const = 0;
            /* The speed set by an impulse lever, in tens (NewSpeed) */
            virtual double get_set_velocity() const = 0;
            /* The speed control is on but waiting, holding no power (SpeedCtrlUnit.Standby) */
            virtual bool get_standby() const = 0;
            /* The player's buttons (OnCommand_speedcontrol*, Train.cpp:6887-6974) */
            virtual void speed_control_increase() = 0;
            virtual void speed_control_decrease() = 0;
            virtual void speed_control_power_increase() = 0;
            virtual void speed_control_power_decrease() = 0;
            virtual void speed_control_button(int p_button) = 0;
            /* The speed set outright [km/h], as a driver's order (RunCommand("SpeedCntrl"), Mover.cpp:12695) */
            virtual void speed_control_set(double p_velocity) = 0;
            void _register_commands() override;
            void _unregister_commands() override;
            MAKE_MEMBER_GS(bool, speed_control_enabled, false);
            /* LoadFIZ_SpeedControl's defaults (MOVER.h:1257-1286, 1933-1934) */
            MAKE_MEMBER_GS(double, delay, 2.0);
            MAKE_MEMBER_GS(bool, impulse_lever, false);
            MAKE_MEMBER_GS(int, disables_on, 0);
            MAKE_MEMBER_GS(
                    PackedFloat64Array, preset_speeds,
                    PackedFloat64Array({30.0, 40.0, 50.0, 60.0, 70.0, 80.0, 90.0, 100.0, 110.0, 120.0}));
            MAKE_MEMBER_GS(bool, override_manual_power, true);
            MAKE_MEMBER_GS(double, initial_power, 1.0);
            MAKE_MEMBER_GS(double, full_power_velocity, -1.0);
            MAKE_MEMBER_GS(double, start_velocity, -1.0);
            MAKE_MEMBER_GS(double, velocity_step, 5.0);
            MAKE_MEMBER_GS(double, power_step, 0.1);
            MAKE_MEMBER_GS(double, min_power, 0.0);
            MAKE_MEMBER_GS(double, max_power, 1.0);
            MAKE_MEMBER_GS(double, min_velocity, 0.0);
            MAKE_MEMBER_GS(double, max_velocity, 120.0);
            MAKE_MEMBER_GS(double, offset, -0.5);
            MAKE_MEMBER_GS(double, proportional_gain_positive, 0.5);
            MAKE_MEMBER_GS(double, proportional_gain_negative, 0.5);
            MAKE_MEMBER_GS(double, integral_gain_positive, 0.0);
            MAKE_MEMBER_GS(double, integral_gain_negative, 0.0);
            MAKE_MEMBER_GS(bool, brake_intervention, false);
            MAKE_MEMBER_GS(double, brake_intervention_max_velocity, 30.0);
            MAKE_MEMBER_GS(double, power_up_speed, 1000.0);
            MAKE_MEMBER_GS(double, power_down_speed, 1000.0);
    };
} // namespace godot
