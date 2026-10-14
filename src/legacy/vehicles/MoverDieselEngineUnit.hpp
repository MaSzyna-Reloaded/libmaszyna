#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleDieselEngine.hpp"
#include "vehicles/rail/RailVehicleDieselEngineUnit.hpp"

namespace godot {
    /* The diesel engine on the vendored Mover. */
    class MoverDieselEngineUnit : public RailVehicleDieselEngineUnit {
        private:
            /* The Mover* component that owns this unit - it reaches the Mover through it. */
            const MoverComponent &owner;

        public:
            explicit MoverDieselEngineUnit(const MoverComponent &p_owner) : owner(p_owner) {}

            double get_rpm() const override;
            bool get_oil_pump_active() const override;
            bool get_oil_pump_disabled() const override;
            double get_oil_pump_pressure() const override;
            bool get_fuel_pump_active() const override;
            bool get_fuel_pump_disabled() const override;
            bool get_heat_malfunction() const override;
            bool get_fuel_pump_enabled() const override;
            bool get_oil_pump_enabled() const override;
            bool get_startup() const override;
            bool get_ignition() const override;
            bool get_spinup() const override;
            double get_output_power() const override;
            double get_torque() const override;
            double get_fill() const override;
            double get_fill_desired() const override;
            double get_clutch_desired() const override;
            double get_clutch_engagement() const override;
            double get_water_temperature() const override;
            double get_engine_temperature() const override;
            double get_retarder_fill() const override;
            bool get_water_pump_enabled() const override;
            bool get_water_pump_active() const override;
            bool get_water_pump_breaker() const override;
            bool get_water_heater_enabled() const override;
            bool get_water_heater_active() const override;
            bool get_water_heater_breaker() const override;
            bool get_water_circuits_link() const override;
            double get_main_circuit_water_temperature() const override;
            double get_auxiliary_circuit_water_temperature() const override;
            double get_oil_temperature() const override;
            double get_max_rpm() const override;
            double get_idle_rpm_count() const override;
            void apply_configuration(const RailVehicleDieselEngine *p_engine) const override;
            void oil_pump(bool p_enabled) const override;
            void fuel_pump(bool p_enabled) const override;
            void oil_pump_switch_off(bool p_enabled) const override;
            void fuel_pump_switch_off(bool p_enabled) const override;
            void water_pump(bool p_enabled) const override;
            void water_pump_switch_off(bool p_enabled) const override;
            void water_pump_breaker(bool p_enabled) const override;
            void water_heater(bool p_enabled) const override;
            void water_heater_breaker(bool p_enabled) const override;
            void water_circuits_link(bool p_enabled) const override;
            void fill_config(Dictionary &p_config) const override;
    };
} // namespace godot
