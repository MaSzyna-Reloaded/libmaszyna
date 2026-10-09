#pragma once
#include "RailVehicleElectricEngine.hpp"
#include "RailVehicleInverter.hpp"
#include "macros.hpp"
#include "vehicles/base/VehicleCurvePointItem.hpp"
#include "vehicles/rail/RailVehicleWWListItem.hpp"

namespace godot {
    class VehicleController;

    class RailVehicleElectricInductionEngine : public RailVehicleElectricEngine {
            GDCLASS(RailVehicleElectricInductionEngine, RailVehicleElectricEngine)
        public:
            /// FIZ edep when absent, the original's default
            static constexpr double DEFAULT_ELECTRODYNAMIC_EP_RATIO = 1.5;
            /// FIZ InvCtrCplFlag when absent, the original's default
            static constexpr int DEFAULT_INVERTER_CONTROL_COUPLER_FLAG = 4;

            static void _bind_methods();

        private:
            TypedArray<RailVehicleWWListItem> wwlist;

        protected:
            EngineType get_type() const override;
            void _fill_state_dictionary(Dictionary &p_state) const override;

        public:
            /// The state of each inverter (TMoverParameters::Inverters)
            virtual TypedArray<RailVehicleInverter> get_inverters() const = 0;
            /* The force the motors are set to [kN], negative when braking (eimv[eimv_Fmax]) */
            virtual double get_force_max() const = 0;
            /* The full force at the current speed [kN] (eimv[eimv_Fful]) */
            virtual double get_force_full() const = 0;
            /* The motors' field current [A] (eimv[eimv_If]) */
            virtual double get_field_current() const = 0;
            /* The voltage the inverters feed the motors with [V] (eimv[eimv_U]) */
            virtual double get_motor_voltage() const = 0;

            TypedArray<RailVehicleWWListItem> get_wwlist() {
                return wwlist;
            }

            void set_wwlist(const TypedArray<RailVehicleWWListItem> &p_wwlist) {
                wwlist.clear();
                wwlist.append_array(p_wwlist);
            }

            MAKE_MEMBER_GS(double, slip_current_ratio, 0.0);
            MAKE_MEMBER_GS(double, max_slip, 0.0);
            MAKE_MEMBER_GS(double, pole_pairs, 0.0);
            MAKE_MEMBER_GS(double, nominal_uf_ratio, 0.0);
            MAKE_MEMBER_GS(double, current_torque_ratio, 0.0);
            MAKE_MEMBER_GS(double, current_three_phase_ratio, 0.0);
            MAKE_MEMBER_GS(double, max_supply_voltage, 0.0);
            MAKE_MEMBER_GS(double, max_supply_voltage_braking, 0.0);
            MAKE_MEMBER_GS(double, inverter_voltage_drop, 0.0);
            MAKE_MEMBER_GS(double, no_load_current, 0.0);
            MAKE_MEMBER_GS(double, inverter_uf_setpoint, 0.0);
            MAKE_MEMBER_GS(double, inverter_uf_setpoint_braking, 0.0);
            MAKE_MEMBER_GS(double, initial_force, 0.0);
            MAKE_MEMBER_GS(double, force_drop_rate, 0.0);
            MAKE_MEMBER_GS(double, max_power, 0.0);
            MAKE_MEMBER_GS(double, max_braking_force, 0.0);
            MAKE_MEMBER_GS(double, max_braking_power, 0.0);
            MAKE_MEMBER_GS(double, braking_decay_velocity, 0.0);
            MAKE_MEMBER_GS(double, braking_decay_start_velocity, 0.0);
            MAKE_MEMBER_GS(double, motor_max_current, 0.0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<VehicleCurvePointItem>, max_power_table)

        private:
            /* The rest of the FIZ Engine: line of an induction motor. */
            double nominal_voltage = 0.0;                                              // Volt
            double electrodynamic_brake_cylinder_ratio = 0.0;                          // abed
            double electrodynamic_ep_ratio = DEFAULT_ELECTRODYNAMIC_EP_RATIO;          // edep
            bool logarithmic_force_control = false;                                    // eimclf
            int inverter_control_coupler_flag = DEFAULT_INVERTER_CONTROL_COUPLER_FLAG; // InvCtrCplFlag
            bool flat_force_characteristic = false;                                    // Flat

        public:
            void set_nominal_voltage(double p_value);
            double get_nominal_voltage() const;
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
    };
} // namespace godot
