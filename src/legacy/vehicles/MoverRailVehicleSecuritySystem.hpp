#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleSecuritySystem.hpp"

namespace godot {
    /* RailVehicleSecuritySystem on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleSecuritySystem : public RailVehicleSecuritySystem, public MoverComponent {
            GDCLASS(MoverRailVehicleSecuritySystem, RailVehicleSecuritySystem);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

            bool previous_blinking = false;
            bool previous_beeping = false;

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            bool get_beeping() const override;
            bool get_blinking() const override;
            bool get_radiostop_available() const override;
            bool get_vigilance_blinking() const override;
            bool get_cabsignal_blinking() const override;
            bool get_cabsignal_beeping() const override;
            bool get_braking() const override;
            bool get_engine_blocked() const override;
            bool get_separate_acknowledge() const override;
            void security_acknowledge(bool p_enabled) override;
            void security_cabsignal_acknowledge() override;
            void security_cabsignal_trigger() override;
            void security_radiostop(bool p_enabled) override;
    };
} // namespace godot
