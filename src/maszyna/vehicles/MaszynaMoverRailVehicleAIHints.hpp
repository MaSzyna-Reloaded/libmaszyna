#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleAIHints.hpp"

namespace godot {
    /* RailVehicleAIHints on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleAIHints : public RailVehicleAIHints, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleAIHints, RailVehicleAIHints);

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

        private:
            double local_brake_acceleration_factor = 1.05; // MOVER.h:2286

        public:
            MAKE_GS(double, local_brake_acceleration_factor);
    };
} // namespace godot
