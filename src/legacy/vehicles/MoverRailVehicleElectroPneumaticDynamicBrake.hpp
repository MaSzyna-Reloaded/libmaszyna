#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleElectroPneumaticDynamicBrake.hpp"

namespace godot {
    /* RailVehicleElectroPneumaticDynamicBrake on the vendored Mover - the only class here that knows TMoverParameters.
     */
    class MoverRailVehicleElectroPneumaticDynamicBrake : public RailVehicleElectroPneumaticDynamicBrake,
                                                         public MoverComponent {
            GDCLASS(MoverRailVehicleElectroPneumaticDynamicBrake, RailVehicleElectroPneumaticDynamicBrake);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            double get_ed_braking_ep_delay() const override;
            double get_ep_max_brake_engagement_speed() const override;
            double get_ep_min_regenerative_braking() const override;
            double get_ep_force() const override;
            bool get_ep_fuse() const override;
            void set_ep_brake_force(int p_value) override;
            void switch_ep_fuse(bool p_value) override;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override {};
    };
} // namespace godot
