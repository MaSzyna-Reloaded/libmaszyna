#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleHeating.hpp"

namespace godot {
    /* RailVehicleHeating on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleHeating : public RailVehicleHeating, public MoverComponent {
            GDCLASS(MoverRailVehicleHeating, RailVehicleHeating);

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
            bool get_active() const override;
            bool get_allowed() const override;
            double get_power() const override;
            void heating(bool p_enabled) override;
            void _fill_state_dictionary(Dictionary &p_state) const override;
    };
} // namespace godot
