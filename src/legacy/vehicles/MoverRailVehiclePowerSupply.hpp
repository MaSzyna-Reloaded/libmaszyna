#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehiclePowerSupply.hpp"

namespace godot {
    /* RailVehiclePowerSupply on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehiclePowerSupply : public RailVehiclePowerSupply, public MoverComponent {
            GDCLASS(MoverRailVehiclePowerSupply, RailVehiclePowerSupply);

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

        public:
            double get_live_battery_voltage() const override;
            bool get_battery_enabled() const override;
            bool get_converter_enabled() const override;
            bool get_converter_allowed() const override;
            double get_converter_time_to_start() const override;
            double get_power24_voltage() const override;
            bool get_power24_available() const override;
            bool get_power110_available() const override;
            void battery(bool p_enabled) override;
            void converter(bool p_enabled) override;
    };
} // namespace godot
