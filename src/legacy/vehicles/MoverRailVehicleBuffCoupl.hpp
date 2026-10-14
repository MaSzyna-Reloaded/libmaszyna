#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleBuffCoupl.hpp"

namespace godot {
    /* RailVehicleBuffCoupl on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleBuffCoupl : public RailVehicleBuffCoupl, public MoverComponent {
            GDCLASS(MoverRailVehicleBuffCoupl, RailVehicleBuffCoupl);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();
            TCoupling *_get_coupling(TMoverParameters *p_mover) const;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        public:
            void apply_vehicle_config() override;
            bool is_coupled(RailVehicleController::CouplerEnd p_end) const override;
            bool is_brake_hose_connected(RailVehicleController::CouplerEnd p_end) const override;
            bool is_main_hose_connected(RailVehicleController::CouplerEnd p_end) const override;
            bool is_coupling_owner(RailVehicleController::CouplerEnd p_end) const override;
            RailVehicleController::CouplerEnd get_connected_end(RailVehicleController::CouplerEnd p_end) const override;
            double get_coupler_max_force(RailVehicleController::CouplerEnd p_end) const override;
    };
} // namespace godot
