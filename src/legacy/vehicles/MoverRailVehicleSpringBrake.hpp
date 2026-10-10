#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleSpringBrake.hpp"

namespace godot {
    /* RailVehicleSpringBrake on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleSpringBrake : public RailVehicleSpringBrake, public MoverComponent {
            GDCLASS(MoverRailVehicleSpringBrake, RailVehicleSpringBrake);

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
            void set_spring_brake_active(bool p_active) override;
            void set_spring_brake_enabled(bool p_enabled) override;
            void spring_brake_release() override;
    };
} // namespace godot
