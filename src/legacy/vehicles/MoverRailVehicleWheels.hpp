#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"

namespace godot {
    /* RailVehicleWheels on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleWheels : public RailVehicleWheels, public MoverComponent {
            GDCLASS(MoverRailVehicleWheels, RailVehicleWheels);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            double get_angle_front_deg() const override;
            double get_angle_powered_deg() const override;
            double get_angle_rear_deg() const override;
            double get_rotation_speed_rps() const override;
            double get_rotation_acceleration_rps2() const override;
            bool get_slipping() const override;
            double get_flat() const override;

        private:
            double wheel_angle_front_deg = 0.0;
            double wheel_angle_powered_deg = 0.0;
            double wheel_angle_rear_deg = 0.0;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _do_process_component(double p_delta) override;
    };
} // namespace godot
