#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleSpeedControl.hpp"

namespace godot {
    /* RailVehicleSpeedControl on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleSpeedControl : public RailVehicleSpeedControl, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleSpeedControl, RailVehicleSpeedControl);

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

        public:
            void apply_vehicle_config() override;
            void _fill_state_dictionary(Dictionary &p_state) const override;
            bool get_active() const override;
            double get_desired_velocity() const override;
            double get_desired_power() const override;
            double get_selected_velocity() const override;
            double get_set_velocity() const override;
            bool get_standby() const override;
            void speed_control_increase() override;
            void speed_control_decrease() override;
            void speed_control_power_increase() override;
            void speed_control_power_decrease() override;
            void speed_control_button(int p_button) override;
            void speed_control_set(double p_velocity) override;

        private:
            double initial_power = 1.0;
            double full_power_velocity = -1.0;
            double start_velocity = -1.0;
            double offset = -0.5;
            double proportional_gain_positive = 0.5;
            double proportional_gain_negative = 0.5;
            double integral_gain_positive = 0.0;
            double integral_gain_negative = 0.0;
            double brake_intervention_max_velocity = 30.0;
            double power_up_speed = 1000.0;
            double power_down_speed = 1000.0;
            bool override_manual_power = true;

        public:
            MAKE_GS(double, initial_power);
            MAKE_GS(double, full_power_velocity);
            MAKE_GS(double, start_velocity);
            MAKE_GS(double, offset);
            MAKE_GS(double, proportional_gain_positive);
            MAKE_GS(double, proportional_gain_negative);
            MAKE_GS(double, integral_gain_positive);
            MAKE_GS(double, integral_gain_negative);
            MAKE_GS(double, brake_intervention_max_velocity);
            MAKE_GS(double, power_up_speed);
            MAKE_GS(double, power_down_speed);
            MAKE_GS(bool, override_manual_power);
    };
} // namespace godot
