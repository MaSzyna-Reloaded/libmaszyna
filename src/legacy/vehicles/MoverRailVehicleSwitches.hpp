#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleSwitches.hpp"

namespace godot {
    /* RailVehicleSwitches on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleSwitches : public RailVehicleSwitches, public MoverComponent {
            GDCLASS(MoverRailVehicleSwitches, RailVehicleSwitches);

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
            bool get_sand_active() const override;
            int get_pantograph_preset_position(RailVehicleController::CouplerEnd p_end) const override;
            PantographPreset get_pantograph_preset(RailVehicleController::CouplerEnd p_end) const override;
            void sand(bool p_active) override;
            void universal_relay_reset(RelayResetButton p_button) override;
            void next_pantograph_preset(RailVehicleController::CouplerEnd p_end) override;
            void previous_pantograph_preset(RailVehicleController::CouplerEnd p_end) override;
    };
} // namespace godot
