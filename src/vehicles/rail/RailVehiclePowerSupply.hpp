#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

namespace godot {
    /* The vehicle's low voltage: the battery, the converter and the 24 V / 110 V supply they feed
     * (Mover.cpp:1559-1565, PowerCouplersCheck) - the public contract, with no backend in it. A
     * carriage has one as well as a locomotive (LMaxVoltage of its Light:, and the converter the
     * original lets carriages run, Mover.cpp:1896). */
    class RailVehiclePowerSupply : public RailVehicleComponent {
            GDCLASS(RailVehiclePowerSupply, RailVehicleComponent);

        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_POWER_SUPPLY;
            }

            /// The low voltage came or went
            static const char *power_changed_signal;

        private:
            /* What power_changed last announced - compared in the tick, since the low voltage
             * comes and goes without a command */
            bool previous_powered = false;

        protected:
            static void _bind_methods();
            void _register_commands() override;
            void _unregister_commands() override;
            void _do_process_component(double p_delta) override;

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            /* The battery as it actually is, which drains and recharges. The authored
             * `battery_voltage` property is the nominal one - the two are only equal on a full
             * battery. */
            virtual double get_live_battery_voltage() const = 0;
            virtual bool get_battery_enabled() const = 0;
            /* ConverterFlag: the converter runs */
            virtual bool get_converter_enabled() const = 0;
            /* ConverterAllow: the converter's switch is on */
            virtual bool get_converter_allowed() const = 0;
            /* ConverterStartDelayTimer [s] */
            virtual double get_converter_time_to_start() const = 0;
            virtual double get_power24_voltage() const = 0;
            virtual bool get_power24_available() const = 0;
            virtual bool get_power110_available() const = 0;

            virtual void battery(bool p_enabled) = 0;
            /* The converter switched (ConverterSwitch(), Mover.cpp:3702): the cab's own, sent along
             * the control line to the vehicles that carry one */
            virtual void converter(bool p_enabled) = 0;

            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Light: LMaxVoltage - the nominal battery voltage (LoadFIZ_Light) */
            MAKE_MEMBER_GS(double, battery_voltage, 0.0);
            /* Cntrl. BatteryStart=, ConverterStart=, ConverterStartDelay= (LoadFIZ_Cntrl) */
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_battery_start_mode,
                    RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_converter_start_mode,
                    RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS(double, cntrl_converter_start_delay, 0.0);
    };
} // namespace godot
