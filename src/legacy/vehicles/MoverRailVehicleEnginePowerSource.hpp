#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleEnginePowerSource.hpp"

namespace godot {
    /* RailVehicleEnginePowerSource on the vendored Mover - EnginePowerSource and the pantographs of
     * TMoverParameters; the only class here that knows them. */
    class MoverRailVehicleEnginePowerSource : public RailVehicleEnginePowerSource, public MoverComponent {
            GDCLASS(MoverRailVehicleEnginePowerSource, RailVehicleEnginePowerSource);

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
            void _do_process_component(double p_delta) override;

        public:
            double get_collector_max_voltage() const override;
            double get_collector_max_current() const override;
            double get_collector_max_lifting() const override;
            double get_collector_min_lifting() const override;
            double get_collector_sliding_width() const override;
            double get_collector_min_main_switch_voltage() const override;
            double get_collector_min_pantograph_tank_pressure() const override;
            double get_collector_max_pantograph_tank_pressure() const override;
            double get_collector_pantograph_tank_pressure() const override;
            bool get_collector_pantograph_pressure_switch_armed() const override;
            bool get_collector_pantograph_pressure_lock_active() const override;
            bool get_collector_pantograph_compressor_valve() const override;
            bool get_collector_pantograph_compressor_enabled() const override;
            bool get_collector_overvoltage_relay() const override;
            double get_collector_required_main_switch_voltage() const override;
            bool get_collector_valve_active() const override;
            bool get_collector_valve_enabled() const override;
            bool get_collector_pantographs_dropped() const override;
            bool get_collector_pantograph_first_active() const override;
            bool get_collector_pantograph_second_active() const override;
            bool get_collector_pantograph_first_valve_enabled() const override;
            bool get_collector_pantograph_second_valve_enabled() const override;
            bool get_collector_pantograph_first_valve_active() const override;
            bool get_collector_pantograph_second_valve_active() const override;
            double get_collector_pantograph_first_voltage() const override;
            double get_collector_pantograph_second_voltage() const override;
            double get_collector_voltage() const override;
            double get_collector_trainset_high_voltage() const override;
            double get_energy_drawn() const override;
            double get_energy_returned() const override;

            void pantographs_valve(bool p_enabled) override;
            void pantographs_valve_operate(ValveOperation p_operation) override;
            void pantographs_drop_all(bool p_enabled) override;
            void pantograph_compressor(bool p_enabled) override;
            void pantograph_compressor_valve(bool p_to_compressor) override;
            void pantograph(PantographSelector p_selector, bool p_enabled) override;
            void pantograph_valve_operate(PantographSelector p_selector, ValveOperation p_operation) override;
            void set_pantograph_wire_voltage(PantographSelector p_selector, float p_voltage) override;
            void set_collector_voltage(float p_voltage) override;
    };
} // namespace godot
