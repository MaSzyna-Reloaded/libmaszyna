#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleSpringBrake.hpp"

namespace godot {
    /* RailVehicleSpringBrake on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleSpringBrake : public RailVehicleSpringBrake, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleSpringBrake, RailVehicleSpringBrake);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override {};

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            bool get_ready() const override;
            bool get_shut_off() const override;
            bool get_active() const override;
            bool get_braking() const override;
            double get_cylinder_pressure() const override;
            void spring_brake_activate(bool p_active) override;
            void spring_brake_shut_off(bool p_shut_off) override;
            void spring_brake_release() override;

        private:
            float spring_actuator_chamber_volume = 1.0f;
            float spring_actuator_max_filling_force = 0.0f;
            float pressure_force_coefficient = 0.0f;
            float spring_actuator_preload_pressure = 0.0f;
            float spring_full_balance_pressure = 0.0f;
            float brake_signal_released_state_pressure = 0.0f;
            float brake_signal_braked_state_pressure = 0.0f;
            float valve_cross_section_actuator_discharge = 0.0f;
            float valve_cross_section_actuator_charge = 0.0f;
            float valve_cross_section_pneumatic_brake = 0.0f;
            int required_coupler_connection_method = ALL_COUPLER_CONNECTIONS;

        public:
            MAKE_GS(float, spring_actuator_chamber_volume);
            MAKE_GS(float, spring_actuator_max_filling_force);
            MAKE_GS(float, pressure_force_coefficient);
            MAKE_GS(float, spring_actuator_preload_pressure);
            MAKE_GS(float, spring_full_balance_pressure);
            MAKE_GS(float, brake_signal_released_state_pressure);
            MAKE_GS(float, brake_signal_braked_state_pressure);
            MAKE_GS(float, valve_cross_section_actuator_discharge);
            MAKE_GS(float, valve_cross_section_actuator_charge);
            MAKE_GS(float, valve_cross_section_pneumatic_brake);
            MAKE_GS(int, required_coupler_connection_method);
    };
} // namespace godot
