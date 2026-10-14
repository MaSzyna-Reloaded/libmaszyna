#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"

namespace godot {
    /* RailVehicleWipers on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleWipers : public RailVehicleWipers, public MoverComponent {
            GDCLASS(MoverRailVehicleWipers, RailVehicleWipers);

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
            bool _get_battery_enabled() const override;
    };
} // namespace godot
