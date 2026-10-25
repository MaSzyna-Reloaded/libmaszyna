#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleWheels.hpp"

namespace godot {
    /* RailVehicleWheels on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleWheels : public RailVehicleWheels, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleWheels, RailVehicleWheels);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

            /* The wheel turns - the vehicle's, not the simulation's (TDynamicObject::Update,
             * DynObj.cpp:3776-3792) [deg] */
            double wheel_angle_front_deg = 0.0;
            double wheel_angle_powered_deg = 0.0;
            double wheel_angle_rear_deg = 0.0;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            double get_angle_front_deg() const override;
            double get_angle_powered_deg() const override;
            double get_angle_rear_deg() const override;
            double get_rotation_speed_rps() const override;
            double get_rotation_acceleration_rps2() const override;
            bool get_slipping() const override;
            double get_flat() const override;

        protected:
            /* The wheels turned on by a step, from the vehicle's velocity and the powered axles'
             * revolutions */
            void _do_process_component(double p_delta) override;
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
    };
} // namespace godot
