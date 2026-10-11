#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleMasterController.hpp"

namespace godot {
    /* RailVehicleMasterController on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleMasterController : public RailVehicleMasterController, public MoverComponent {
            GDCLASS(MoverRailVehicleMasterController, RailVehicleMasterController);

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
            double _get_powered_wheel_diameter() const override;
            double _get_powered_axle_revolutions() const override;
            double _get_max_velocity() const override;
            bool _get_low_voltage_available() const override;
            int _get_occupied_cab() const override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        public:
            int get_main_position() const override;
            int get_second_position() const override;
            int get_joint_position() const override;
            int get_main_actual_position() const override;
            int get_second_actual_position() const override;
            bool get_main_delayed() const override;
            int get_main_no_power_position() const override;
            int get_cabin() const override;
            bool get_cabin_controleable() const override;
    };
} // namespace godot
