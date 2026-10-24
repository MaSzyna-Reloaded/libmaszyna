#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverUniversalControllerListItem.hpp"
#include "vehicles/rail/RailVehicleUniversalController.hpp"

namespace godot {
    /* RailVehicleUniversalController on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleUniversalController : public RailVehicleUniversalController,
                                                       public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleUniversalController, RailVehicleUniversalController);

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
            void _fill_config_dictionary(Dictionary &p_config) const override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

        private:
            TypedArray<MaszynaMoverUniversalControllerListItem> positions;

        public:
            MAKE_GS_NR(TypedArray<MaszynaMoverUniversalControllerListItem>, positions);
    };
} // namespace godot
