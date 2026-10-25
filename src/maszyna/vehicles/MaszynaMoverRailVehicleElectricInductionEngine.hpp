#pragma once
#include "MaszynaMoverCircuitUnit.hpp"
#include "MaszynaMoverDriveUnit.hpp"
#include "MaszynaMoverTractionMotorsUnit.hpp"
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverElectricEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverEngineConfiguration.hpp"
#include "maszyna/vehicles/MaszynaMoverWWListItem.hpp"
#include "vehicles/rail/RailVehicleElectricInductionEngine.hpp"

namespace godot {
    /* RailVehicleElectricInductionEngine on the vendored Mover. It owns the units the engine is
     * composed of, and writes the induction motor's own
     * configuration into the Mover, which none of the units covers. */
    class MaszynaMoverRailVehicleElectricInductionEngine : public RailVehicleElectricInductionEngine,
                                                           public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleElectricInductionEngine, RailVehicleElectricInductionEngine);

        public:
            /// FIZ edep when absent, the original's default
            static constexpr double DEFAULT_ELECTRODYNAMIC_EP_RATIO = 1.5;
            /// FIZ InvCtrCplFlag when absent, the original's default
            static constexpr int DEFAULT_INVERTER_CONTROL_COUPLER_FLAG = 4;

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();
            /* What the Mover's model of the engine is configured with beyond its facts. The units read it */
            Ref<MaszynaMoverEngineConfiguration> engine_configuration;
            /* What the Mover's model of the electric engine's circuit is configured with. The units read it */
            Ref<MaszynaMoverElectricEngineConfiguration> electric_engine_configuration;
            MaszynaMoverDriveUnit drive_unit_impl{*this, engine_configuration};
            MaszynaMoverCircuitUnit circuit_unit_impl{*this, electric_engine_configuration};
            MaszynaMoverTractionMotorsUnit traction_motors_unit_impl{*this};


        public:
            MaszynaMoverRailVehicleElectricInductionEngine() {
                engine_configuration.instantiate();
                electric_engine_configuration.instantiate();
                traction_motors_unit = &traction_motors_unit_impl;
                drive_unit = &drive_unit_impl;
                circuit_unit = &circuit_unit_impl;
            }

            TypedArray<RailVehicleInverter> get_inverters() const override;
            double get_force_max() const override;
            double get_force_full() const override;
            double get_field_current() const override;
            double get_motor_voltage() const override;

        protected:
            void _apply_configuration() override;

        private:
            double slip_current_ratio = 0.0;
            double max_slip = 0.0;
            double nominal_uf_ratio = 0.0;
            double current_torque_ratio = 0.0;
            double current_three_phase_ratio = 0.0;
            double max_supply_voltage = 0.0;
            double max_supply_voltage_braking = 0.0;
            double inverter_voltage_drop = 0.0;
            double no_load_current = 0.0;
            double inverter_uf_setpoint = 0.0;
            double inverter_uf_setpoint_braking = 0.0;
            double force_drop_rate = 0.0;
            double braking_decay_velocity = 0.0;
            double braking_decay_start_velocity = 0.0;
            double electrodynamic_brake_cylinder_ratio = 0.0;                          // abed
            double electrodynamic_ep_ratio = DEFAULT_ELECTRODYNAMIC_EP_RATIO;          // edep
            bool logarithmic_force_control = false;                                    // eimclf
            int inverter_control_coupler_flag = DEFAULT_INVERTER_CONTROL_COUPLER_FLAG; // InvCtrCplFlag
            bool flat_force_characteristic = false;                                    // Flat
            TypedArray<VehicleCurvePointItem> max_power_table;
            TypedArray<MaszynaMoverWWListItem> wwlist;

        public:
            MAKE_GS(double, slip_current_ratio);
            MAKE_GS(double, max_slip);
            MAKE_GS(double, nominal_uf_ratio);
            MAKE_GS(double, current_torque_ratio);
            MAKE_GS(double, current_three_phase_ratio);
            MAKE_GS(double, max_supply_voltage);
            MAKE_GS(double, max_supply_voltage_braking);
            MAKE_GS(double, inverter_voltage_drop);
            MAKE_GS(double, no_load_current);
            MAKE_GS(double, inverter_uf_setpoint);
            MAKE_GS(double, inverter_uf_setpoint_braking);
            MAKE_GS(double, force_drop_rate);
            MAKE_GS(double, braking_decay_velocity);
            MAKE_GS(double, braking_decay_start_velocity);
            MAKE_GS_NR(TypedArray<VehicleCurvePointItem>, max_power_table);

            TypedArray<MaszynaMoverWWListItem> get_wwlist() {
                return wwlist;
            }

            void set_wwlist(const TypedArray<MaszynaMoverWWListItem> &p_wwlist) {
                wwlist.clear();
                wwlist.append_array(p_wwlist);
            }

            void set_electrodynamic_brake_cylinder_ratio(double p_value);
            double get_electrodynamic_brake_cylinder_ratio() const;
            void set_electrodynamic_ep_ratio(double p_value);
            double get_electrodynamic_ep_ratio() const;
            void set_logarithmic_force_control(bool p_value);
            bool get_logarithmic_force_control() const;
            void set_inverter_control_coupler_flag(int p_value);
            int get_inverter_control_coupler_flag() const;
            void set_flat_force_characteristic(bool p_value);
            bool get_flat_force_characteristic() const;
            Ref<MaszynaMoverEngineConfiguration> get_engine_configuration() const;
            void set_engine_configuration(const Ref<MaszynaMoverEngineConfiguration> &p_value);
            /* The Mover's model of the engine, read by the sounds and the AI (#316) */
            double get_circuit_nmax_rpm() const;
            double get_eimic_real() const;
            Ref<MaszynaMoverElectricEngineConfiguration> get_electric_engine_configuration() const;
            void set_electric_engine_configuration(const Ref<MaszynaMoverElectricEngineConfiguration> &p_value);
    };
} // namespace godot
