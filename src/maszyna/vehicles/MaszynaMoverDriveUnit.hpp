#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverEngineConfiguration.hpp"
#include "vehicles/rail/RailVehicleDriveUnit.hpp"

namespace godot {
    /* The drive on the vendored Mover. */
    class MaszynaMoverDriveUnit : public RailVehicleDriveUnit {
        private:
            /* The MaszynaMover* component that owns this unit - it reaches the Mover through it. */
            const MaszynaMoverComponent &owner;
            /* What the owner's Mover model is configured with - the owner's own property */
            const Ref<MaszynaMoverEngineConfiguration> &configuration;

        public:
            MaszynaMoverDriveUnit(
                    const MaszynaMoverComponent &p_owner, const Ref<MaszynaMoverEngineConfiguration> &p_configuration) :
                owner(p_owner), configuration(p_configuration) {}

            bool get_main_switch_enabled() const override;
            bool get_main_switch_closable() const override;
            double get_motor_torque() const override;
            double get_wheel_torque() const override;
            double get_wheel_force() const override;
            double get_tractive_force() const override;
            double get_power() const override;
            double get_rpm_count() const override;
            double get_angle() const override;
            double get_rpm_ratio() const override;
            int get_damage() const override;
            double get_main_switch_time() const override;
            bool get_main_no_power_pos() const override;
            bool get_motor_overload_relay_high_threshold() const override;
            bool get_relay_novolt() const override;
            bool get_relay_overvoltage() const override;
            bool get_relay_ground() const override;
            int get_circuit_rlist_size() const override;
            double get_current(int p_ammeter) const override;
            void apply_configuration(const RailVehicleEngine *p_engine) const override;
            bool get_motor_blowers_enabled(RailVehicleController::CouplerEnd p_end) const override;
            bool get_motor_blowers_disabled(RailVehicleController::CouplerEnd p_end) const override;
            bool get_motor_blowers_active(RailVehicleController::CouplerEnd p_end) const override;
            bool main_switch(bool p_enabled) const override;
            void motor_blowers(bool p_enabled, RailVehicleController::CouplerEnd p_end) const override;
            void motor_blowers_switch_off(bool p_enabled, RailVehicleController::CouplerEnd p_end) const override;
            bool motor_overload_relay_threshold(bool p_high) const override;
            void process(const RailVehicleEngine *p_engine, double p_delta) const override;
            /* The engine's highest speed [rpm] (nmax) */
            double get_circuit_nmax_rpm() const;
            /* The power the EIM controller actually asks for, 0..1, braking below (eimic_real) */
            double get_eimic_real() const;
            void fill_config(Dictionary &p_config) const override;
            void fill_state(Dictionary &p_state) const override;
    };
} // namespace godot
