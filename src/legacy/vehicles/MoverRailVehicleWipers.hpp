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

            struct Wiper {
                    double position = 0.0;  // dWiperPos: 0 parked, 1 fully out
                    bool returning = false; // wiperDirection
                    double out_timer = 0.0;
                    double park_timer = 0.0;
                    int working_switch_position = 0;
            };
            std::vector<Wiper> wipers;
            int switch_position = 0;
            bool switch_initialized = false;
            void _set_switch_position(int p_position);

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _do_process_component(double p_delta) override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            int get_switch_position() const override;
            PackedFloat64Array get_sweep_positions() const override;
            void switch_increase() override;
            void switch_decrease() override;
    };
} // namespace godot
