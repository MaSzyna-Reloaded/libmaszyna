#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
namespace godot {
    class VehicleController;
    class RailVehicleSpringBrake : public RailVehicleComponent {
            GDCLASS(RailVehicleSpringBrake, RailVehicleComponent);


        public:
            /// The original's default - every coupling passes the command on
            static constexpr int ALL_COUPLER_CONNECTIONS = 127;

            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_SPRING_BRAKE;
            }

        private:
            static void _bind_methods();

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_ready() const = 0;
            virtual bool get_shut_off() const = 0;
            virtual bool get_active() const = 0;
            /* SpringBrake.IsActive - the spring actually braking, from the cylinder pressure */
            virtual bool get_braking() const = 0;
            virtual double get_cylinder_pressure() const = 0;
            MAKE_MEMBER_GS(float, spring_actuator_chamber_volume, 1.0f);
            MAKE_MEMBER_GS(float, spring_actuator_max_filling_force, 0.0f);
            MAKE_MEMBER_GS(float, pressure_force_coefficient, 0.0f);
            MAKE_MEMBER_GS(float, spring_actuator_preload_pressure, 0.0f);
            MAKE_MEMBER_GS(float, spring_full_balance_pressure, 0.0f);
            MAKE_MEMBER_GS(float, brake_signal_released_state_pressure, 0.0f);
            MAKE_MEMBER_GS(float, brake_signal_braked_state_pressure, 0.0f);
            MAKE_MEMBER_GS(float, valve_cross_section_actuator_discharge, 0.0f);
            MAKE_MEMBER_GS(float, valve_cross_section_actuator_charge, 0.0f);
            MAKE_MEMBER_GS(float, valve_cross_section_pneumatic_brake, 0.0f);
            MAKE_MEMBER_GS(int, required_coupler_connection_method, ALL_COUPLER_CONNECTIONS);
            virtual void set_spring_brake_active(bool p_active) = 0;
            virtual void set_spring_brake_enabled(bool p_enabled) = 0;
            virtual void spring_brake_release() = 0;
    };
} // namespace godot
