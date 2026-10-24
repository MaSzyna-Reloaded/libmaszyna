#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleRadio.hpp"

namespace godot {
    /* RailVehicleRadio on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleRadio : public RailVehicleRadio, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleRadio, RailVehicleRadio);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            /* What radio_toggled last announced - compared in the tick, since power comes and
             * goes without a command */
            bool previous_powered = false;

        protected:
            static void _bind_methods();
            void _do_process_component(double p_delta) override;
            bool _radio_stop_receive(bool p_driven) override;

        public:
            bool get_enabled() const override;
            bool get_powered() const override;
            bool get_radio_stop_active() const override;
            void radio(bool p_enabled) override;
    };
} // namespace godot
