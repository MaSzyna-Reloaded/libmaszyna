#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleWipers.hpp"

#include <vector>

namespace godot {
    /* RailVehicleWipers on the vendored Mover - the only class here that knows TMoverParameters.
     *
     * The Mover has no wiper state at all (the original keeps it in its vehicle layer, DynObj.h
     * dWiperPos), so the wipers move here: a port of TDynamicObject::update()
     * (DynObj.cpp:4048-4115), the switch the interface's (Train.cpp:2638-2661). */
    class MaszynaMoverRailVehicleWipers : public RailVehicleWipers, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleWipers, RailVehicleWipers);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

            /* The original's wiperOutTimer/wiperParkTimer run on every step (DynObj.cpp:4152-4153);
             * here a wiper keeps when they were last zeroed against one sweep_clock, so a parked
             * one costs nothing and still reads the same timers */
            struct Wiper {
                    double position = 0.0;  // dWiperPos: 0 parked, 1 fully out
                    bool returning = false; // wiperDirection
                    double out_since = 0.0;
                    double park_since = 0.0;
                    int working_switch_position = 0;
            };
            std::vector<Wiper> wipers;
            double sweep_clock = 0.0;
            /* Every wiper stood parked after the last pass */
            bool parked = true;

        protected:
            /* As many wipers as the vehicle model has (the original counts its animated submodels,
             * DynObj.cpp:5842), or the highest one the list switches on */
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;

        public:
            PackedFloat64Array get_sweep_positions() const override;
    };
} // namespace godot
