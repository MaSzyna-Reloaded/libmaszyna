#pragma once
#include "RailVehicleDriveUnit.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleMotorParameter.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class VehicleController;
    class RailVehicleEngine : public RailVehicleComponent {
            GDCLASS(RailVehicleEngine, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_ENGINE;
            }

        protected:
            /* The drive unit, installed by the implementation that owns it */
            const RailVehicleDriveUnit *drive_unit = nullptr;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Live state, read straight from the engine's units - nothing is stored. */
            bool get_main_switch_enabled() const;
            bool get_main_switch_closable() const;
            double get_motor_torque() const;
            double get_wheel_torque() const;
            double get_wheel_force() const;
            double get_tractive_force() const;
            double get_power() const;
            double get_rpm_count() const;
            /* The engine's turn [rad] - what a pendulum swings by (DynObj.cpp:1121-1125) */
            double get_angle() const;
            double get_rpm_ratio() const;
            double get_circuit_nmax_rpm() const;
            int get_damage() const;
            double get_main_switch_time() const;
            bool get_main_no_power_pos() const;
            bool get_motor_overload_relay_high_threshold() const;
            double get_eimic_real() const;
            bool get_relay_novolt() const;
            bool get_relay_overvoltage() const;
            bool get_relay_ground() const;
            int get_circuit_rlist_size() const;
            /* The engine's ammeters [A]: the total and the two motor branches. A vehicle without
             * an engine has none - its cab reads the powered vehicle's (TTrain::mvControlled,
             * Train.cpp:8638). */
            double get_current0() const;
            double get_current1() const;
            double get_current2() const;

            enum EngineType {
                NONE,
                DUMB,
                WHEELS_DRIVEN,
                ELECTRIC_SERIES_MOTOR,
                ELECTRIC_INDUCTION_MOTOR,
                DIESEL,
                STEAM,
                DIESEL_ELECTRIC,
                MAIN
            };

            /* EIMCtrlType= : traction lever variant, for vehicles with an EIM-style controller */
            enum EimControlType {
                EIM_CONTROL_TYPE_0,
                EIM_CONTROL_TYPE_1,
                EIM_CONTROL_TYPE_2,
                EIM_CONTROL_TYPE_3,
            };

            /* AutoRelay= : automatic starting relay presence */
            enum AutoRelayMode {
                AUTO_RELAY_NO,
                AUTO_RELAY_YES,
                AUTO_RELAY_OPTIONAL,
            };


            TypedArray<RailVehicleMotorParameter> get_motor_param_table() const {
                return motor_param_table;
            }

            void set_motor_param_table(const TypedArray<RailVehicleMotorParameter> &p_motor_param_table) {
                motor_param_table.clear();
                motor_param_table.append_array(p_motor_param_table);
            }

            bool main_switch(bool p_enabled);
            bool motor_overload_relay_threshold(bool p_high);
            /* The traction motors' blowers at an end switched on, or their "off" switch
             * (MotorBlowersSwitch(), MotorBlowersSwitchOff()) */
            void motor_blowers(bool p_enabled, RailVehicleController::CouplerEnd p_end);
            void motor_blowers_switch_off(bool p_enabled, RailVehicleController::CouplerEnd p_end);
            /* The traction motors' blowers at an end: switched on, switched off, working */
            bool get_motor_blowers_enabled(RailVehicleController::CouplerEnd p_end) const;
            bool get_motor_blowers_disabled(RailVehicleController::CouplerEnd p_end) const;
            bool get_motor_blowers_active(RailVehicleController::CouplerEnd p_end) const;
            static void _bind_methods();
            TypedArray<RailVehicleMotorParameter> motor_param_table;

            /* Engine: (wspolne pola dla wszystkich typow napedu) */
            MAKE_MEMBER_GS(int, transmission_gear_teeth_motor, 0);
            MAKE_MEMBER_GS(int, transmission_gear_teeth_wheel, 0);
            MAKE_MEMBER_GS(double, transmission_efficiency, 1.0);
            /* Wheel teeth over motor teeth, 1.0 without a gear (LoadFIZ_Engine, Mover.cpp) */
            double get_transmission_ratio() const;
            MAKE_MEMBER_GS(double, maximum_traction_force, 0.0);
            MAKE_MEMBER_GS(double, motor_blowers_speed, 0.0);
            MAKE_MEMBER_GS(double, motor_blowers_sustain_time, 0.0);
            MAKE_MEMBER_GS(double, motor_blowers_start_velocity, -1.0);
            MAKE_MEMBER_GS(bool, pressure_switch_present, false);
            MAKE_MEMBER_GS(int, inverters_count, 0);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, motor_blowers_start_mode,
                    RailVehicleController::START_MODE_MANUAL);

            /* Cntrl. (wspolne pola sterowania nastawnikiem i rozrusznikiem) */
            MAKE_MEMBER_GS(bool, cntrl_eim_control_additional_zeros, false);
            MAKE_MEMBER_GS(bool, cntrl_eim_control_emergency, false);
            MAKE_MEMBER_GS_NR(EimControlType, cntrl_eim_control_type, EIM_CONTROL_TYPE_0);
            MAKE_MEMBER_GS_NR(AutoRelayMode, cntrl_auto_relay_mode, AUTO_RELAY_NO);
            MAKE_MEMBER_GS(bool, cntrl_has_camshaft, false);
            MAKE_MEMBER_GS(bool, cntrl_series_shunt_on_series_position, false);
            MAKE_MEMBER_GS(bool, cntrl_fast_series_circuit, false);

        private:
            /* Cntrl. MainInitTime: how long the main circuit takes to get ready once it has power
             * [s] (MainsInitTime, Mover.cpp:10910) */
            double main_init_time = 0.0;

        public:
            void set_main_init_time(double p_value);
            double get_main_init_time() const;

        protected:
            /// Change detection for engine_start/engine_stop, compared in _do_process_component().
            /// Starts false so a vehicle coming up with the main switch already closed reports
            /// engine_start on its first tick, as it did when this was compared against the
            /// not-yet-filled state dictionary.
            bool previous_main_switch = false;


        public:
            /* Which kind of engine this is - part of the contract, and what the simulation keys
             * its own configuration off */
            virtual EngineType get_type() const = 0;

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleEngine::EngineType);
VARIANT_ENUM_CAST(RailVehicleEngine::EimControlType);
VARIANT_ENUM_CAST(RailVehicleEngine::AutoRelayMode);
