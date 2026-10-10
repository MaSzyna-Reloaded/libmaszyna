#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleController.hpp"

namespace godot {
    /* What a vehicle's traction is fed from (FIZ "Power:", EnginePowerSource, LoadFIZ_Power): the
     * kind of source and, for one fed from the catenary, the current collector - the pantographs,
     * their valves, tank and compressor, the voltage they bring and the energy meter. A car of a
     * multiple unit has one without an engine of its own (31WE/ED78 B and C: CollectorsNo=1, no
     * Engine:) and feeds the unit's motors over the high voltage line. The public contract, with no
     * backend in it. */
    class RailVehicleEnginePowerSource : public RailVehicleComponent {
            GDCLASS(RailVehicleEnginePowerSource, RailVehicleComponent);

        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_ENGINE_POWER_SOURCE;
            }

            /* Which pantograph an individual command applies to - a vehicle has at most two
             * (the front and the rear one); named FIRST/SECOND here rather than
             * FRONT/REAR since which end is physically "front" depends on the active cab. */
            enum PantographSelector {
                PANTOGRAPH_FIRST,
                PANTOGRAPH_SECOND,
            };
            /* Power: PantType= - the pantograph type whose arms' dimensions the vehicle has (TPantType;
             * Mover.cpp:11620-11629 of the original, the drawing's TAnimPant, DynObj.cpp:181-194);
             * NONE without the key - the arms are then measured from the model */
            enum PantographType {
                PANTOGRAPH_TYPE_NONE,
                PANTOGRAPH_TYPE_AKP_4E,
                PANTOGRAPH_TYPE_DSAX,
                PANTOGRAPH_TYPE_EC160_200,
                PANTOGRAPH_TYPE_WBL85,
            };
            /* What an operation does to a valve - Maszyna's operation_t (MOVER.h:177):
             * ENABLE/DISABLE set it for a two-state switch, the ..._ON/..._OFF pairs press and
             * release one side of an impulse switch, NONE lets both sides go. */
            enum ValveOperation {
                VALVE_OPERATION_NONE,
                VALVE_OPERATION_ENABLE,
                VALVE_OPERATION_DISABLE,
                VALVE_OPERATION_ENABLE_ON,
                VALVE_OPERATION_ENABLE_OFF,
                VALVE_OPERATION_DISABLE_ON,
                VALVE_OPERATION_DISABLE_OFF,
            };

            static const char *pantograph_up_signal;
            static const char *pantograph_down_signal;

        private:
            /* Change detection for pantograph_up/pantograph_down, compared in
             * _do_process_component() against this component's own members */
            bool previous_pantograph_live[2] = {false, false};
            bool previous_pantograph_active[2] = {false, false};

        protected:
            static void _bind_methods();
            void _register_commands() override;
            void _unregister_commands() override;
            void _do_process_component(double p_delta) override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;

            /* Not every vehicle is fed the same way; these say whether the matching values mean
             * anything at all, so the dump can leave their keys out */
            bool has_accumulator() const;
            bool has_power_cable() const;

            /* Live state, read straight from the backend - nothing is stored. */
            virtual double get_collector_max_voltage() const = 0;
            virtual double get_collector_max_current() const = 0;
            virtual double get_collector_max_lifting() const = 0;
            virtual double get_collector_min_lifting() const = 0;
            virtual double get_collector_sliding_width() const = 0;
            virtual double get_collector_min_main_switch_voltage() const = 0;
            virtual double get_collector_min_pantograph_tank_pressure() const = 0;
            virtual double get_collector_max_pantograph_tank_pressure() const = 0;
            virtual double get_collector_pantograph_tank_pressure() const = 0;
            virtual bool get_collector_pantograph_pressure_switch_armed() const = 0;
            /* An EMU's converter and heating held off since its pressure switch tripped, until it
             * is primed again (PantPressLockActive, Mover.cpp:875) */
            virtual bool get_collector_pantograph_pressure_lock_active() const = 0;
            virtual bool get_collector_pantograph_compressor_valve() const = 0;
            /* The small compressor filling the pantograph tank is running (PantCompFlag) */
            virtual bool get_collector_pantograph_compressor_enabled() const = 0;
            virtual bool get_collector_overvoltage_relay() const = 0;
            virtual double get_collector_required_main_switch_voltage() const = 0;
            virtual bool get_collector_valve_active() const = 0;
            virtual bool get_collector_valve_enabled() const = 0;
            virtual bool get_collector_pantographs_dropped() const = 0;
            virtual bool get_collector_pantograph_first_active() const = 0;
            virtual bool get_collector_pantograph_second_active() const = 0;
            /* A pantograph's own valve (Pantographs[].valve.is_enabled) - what pantfront_sw and
             * pantrear_sw flip, together with is_active (Train.cpp:3161) */
            virtual bool get_collector_pantograph_first_valve_enabled() const = 0;
            virtual bool get_collector_pantograph_second_valve_enabled() const = 0;
            /* A pantograph's own valve working (Pantographs[].valve.is_active, Mover.cpp:2312-2315),
             * whether the vehicle has that pantograph or not - what the original's driver takes as
             * a pantograph raised or lowered (driverhints.cpp:269-310) */
            virtual bool get_collector_pantograph_first_valve_active() const = 0;
            virtual bool get_collector_pantograph_second_valve_active() const = 0;
            virtual double get_collector_pantograph_first_voltage() const = 0;
            virtual double get_collector_pantograph_second_voltage() const = 0;
            virtual double get_collector_voltage() const = 0;
            /* The highest voltage on the trainset's high voltage and heating lines [V] */
            virtual double get_collector_trainset_high_voltage() const = 0;
            /* Energy drawn from the wire and returned to it [kWh], the returned one negative */
            virtual double get_energy_drawn() const = 0;
            virtual double get_energy_returned() const = 0;

            virtual void pantographs_valve(bool p_enabled) = 0;
            virtual void pantographs_valve_operate(ValveOperation p_operation) = 0;
            virtual void pantographs_drop_all(bool p_enabled) = 0;
            virtual void pantograph_compressor(bool p_enabled) = 0;
            virtual void pantograph_compressor_valve(bool p_to_compressor) = 0;
            virtual void pantograph(PantographSelector p_selector, bool p_enabled) = 0;
            /* One pantograph's own valve, as the cab operates it (OnCommand_pantographraisefront/
             * lowerfront, Train.cpp:3218-3300) - unlike pantograph(), it leaves the master valve alone */
            virtual void pantograph_valve_operate(PantographSelector p_selector, ValveOperation p_operation) = 0;
            /* Voltage of the overhead wire the pantograph is touching, fed in from outside
             * (RailVehicleServer::vehicle_collect_current()); 0.0 means "not touching a wire" */
            virtual void set_pantograph_wire_voltage(PantographSelector p_selector, float p_voltage) = 0;
            /* The voltage the vehicle is fed with from its pantographs - the wire's, held through a
             * short loss (RailVehicleServer::vehicle_collect_current(), DynObj.cpp:3128-3141) */
            virtual void set_collector_voltage(float p_voltage) = 0;

            /* Power: EnginePower= - the kind of source (LoadFIZ_Power, Mover.cpp:11071) */
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::TrainPowerSource, source_type,
                    RailVehicleController::POWER_SOURCE_NOT_DEFINED);
            /* Power: of a current collector (LoadFIZ_PowerParamsDecode) */
            MAKE_MEMBER_GS(int, current_collector_number_of_collectors, 0);
            MAKE_MEMBER_GS(double, current_collector_max_voltage, 0.0);
            MAKE_MEMBER_GS(double, current_collector_max_current, 0.0);
            MAKE_MEMBER_GS(double, current_collector_min_collector_lifting, 0.0);
            MAKE_MEMBER_GS(double, current_collector_max_collector_lifting, 0.0);
            MAKE_MEMBER_GS(double, current_collector_sliding_width, 0.0);

        private:
            PantographType current_collector_pantograph_type = PANTOGRAPH_TYPE_NONE;

        public:
            void set_current_collector_pantograph_type(PantographType p_value);
            PantographType get_current_collector_pantograph_type() const;
            MAKE_MEMBER_GS(double, current_collector_min_main_switch_voltage, 0.0);
            MAKE_MEMBER_GS(double, current_collector_min_pantograph_tank_pressure, 0.0);
            MAKE_MEMBER_GS(double, current_collector_max_pantograph_tank_pressure, 0.0);
            MAKE_MEMBER_GS(bool, current_collector_overvoltage_relay, false);
            MAKE_MEMBER_GS(double, current_collector_required_main_switch_voltage, 0.0);
            MAKE_MEMBER_GS(bool, current_collector_fake_power, false);
            MAKE_MEMBER_GS(int, current_collector_physical_layout, 0);
            MAKE_MEMBER_GS(double, transducer_input_voltage, 0.0);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::TrainPowerSource, accumulator_recharge_source,
                    RailVehicleController::POWER_SOURCE_NOT_DEFINED);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::TrainPowerType, power_cable_source, RailVehicleController::POWER_TYPE_NONE);
            MAKE_MEMBER_GS(double, power_cable_steam_pressure, 0.0);

            /* Cntrl. of the pantographs (LoadFIZ_Cntrl, Mover.cpp:10909-10946) */
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_pantograph_compressor_start_mode,
                    RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS(bool, cntrl_pantograph_auto_valve, false);
            /* The master valve of the pantographs opens by itself unless the FIZ says otherwise -
             * "there was no pantographs valve" in older vehicles - while each pantograph's own
             * valve is worked by hand */
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_pantographs_valve_start_mode,
                    RailVehicleController::START_MODE_AUTOMATIC);
            MAKE_MEMBER_GS(bool, cntrl_pantographs_valve_spring, true);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::StartMode, cntrl_pantograph_valve_start_mode,
                    RailVehicleController::START_MODE_MANUAL);
            MAKE_MEMBER_GS(bool, cntrl_pantograph_valve_spring, true);
            MAKE_MEMBER_GS(bool, cntrl_pantograph_valve_solenoid, true);
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleEnginePowerSource::PantographSelector);
VARIANT_ENUM_CAST(RailVehicleEnginePowerSource::ValveOperation);
VARIANT_ENUM_CAST(RailVehicleEnginePowerSource::PantographType);
