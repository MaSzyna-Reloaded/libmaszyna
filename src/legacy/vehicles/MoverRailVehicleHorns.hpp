#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleHorns.hpp"

namespace godot {
    /* RailVehicleHorns on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleHorns : public RailVehicleHorns, public MoverComponent {
            GDCLASS(MoverRailVehicleHorns, RailVehicleHorns);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            /* Below this the vehicle counts as standing, and the alarm chain does not sound
             * the emergency signal (DynObj.cpp:4888, the same 0.5 m/s the original compares against) */
            static constexpr double HORN_EMERGENCY_MIN_SPEED = 0.5;

            static void _bind_methods();
            /* DynObj.cpp's per-frame horn combination: while moving with the alarm chain
             * pulled, the emergency signal overrides the manually commanded one - in the Mover's
             * WarningSignal bits */
            int _get_combined_signal() const;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            bool get_low_pressed() const override;
            bool get_high_pressed() const override;
            bool get_whistle_pressed() const override;
            bool get_low_active() const override;
            bool get_high_active() const override;
            bool get_whistle_active() const override;
            int get_horn() const override;
            void set_horn_low(bool p_state) override;
            void set_horn_high(bool p_state) override;
            void set_whistle(bool p_state) override;
    };
} // namespace godot
