#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleAIHints.hpp"

namespace godot {
    /* RailVehicleAIHints on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleAIHints : public RailVehicleAIHints, public MoverComponent {
            GDCLASS(MoverRailVehicleAIHints, RailVehicleAIHints);

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
    };
} // namespace godot
