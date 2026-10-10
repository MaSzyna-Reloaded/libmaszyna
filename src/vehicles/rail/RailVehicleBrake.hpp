#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleBrakePressureTableItem.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleCompressorListItem.hpp"
#include <godot_cpp/classes/node.hpp>
#include <unordered_map>

namespace godot {
    class VehicleController;
    class RailVehicleBrake : public RailVehicleComponent {
            GDCLASS(RailVehicleBrake, RailVehicleComponent)


        public:
            /* The distributor's accelerator fired (sf_Acc, hamulce.cpp:555) - an event, once */
            static const char *accelerator_activated_signal;

            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_BRAKES;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_alarm_chain_pulled() const = 0;
            virtual double get_air_pressure() const = 0;
            virtual double get_loco_pressure() const = 0;
            virtual double get_pipe_brake_pressure() const = 0;
            virtual double get_pipe_pressure() const = 0;
            virtual double get_feed_pipe_pressure() const = 0;
            virtual double get_tank_volume() const = 0;
            virtual double get_compressor_pressure() const = 0;
            /* CompressorFlag: the compressor runs */
            virtual bool get_compressor_enabled() const = 0;
            /* CompressorAllow: the compressor's switch is on */
            virtual bool get_compressor_allowed() const = 0;
            virtual double get_controller_position() const = 0;
            virtual double get_controller_position_normalized() const = 0;
            virtual double get_local_position_normalized() const = 0;
            virtual int get_manual_position() const = 0;
            virtual double get_unit_force() const = 0;
            virtual double get_force_ratio() const = 0;
            virtual double get_emergency_valve_flow() const = 0;
            virtual double get_main_valve_flow() const = 0;
            virtual double get_local_valve_flow() const = 0;
            /* The flows of the driver's brake valve its handle reports for its hiss
             * (Handle->GetSound(), hamulce.h:77-81): braking, release, emergency, the control
             * chamber's wave outflow and the timing reservoir's outflow */
            virtual double get_handle_braking_flow() const = 0;
            virtual double get_handle_release_flow() const = 0;
            virtual double get_handle_emergency_flow() const = 0;
            virtual double get_handle_control_chamber_flow() const = 0;
            virtual double get_handle_timing_reservoir_flow() const = 0;
            virtual double get_control_pressure() const = 0;
            /* Control reservoir of the driver's brake valve (Handle->GetCP(), Train.cpp:8908) */
            virtual double get_handle_control_pressure() const = 0;
            virtual double get_local_aeim_position() const = 0;
            virtual double get_edb_cylinder_pressure() const = 0;
            virtual bool get_releaser_active() const = 0;
            /* The main pipe is cut off from the brake valve (LockPipe, the i-mainpipelock lamp,
             * Train.cpp:11758) */
            virtual bool get_main_pipe_locked() const = 0;
            /* Braking force of the vehicle [kN] (Fb) */
            virtual double get_force() const = 0;
            /* The force the vehicle's brake would give [N] at `p_ratio` of its full pressure and
             * `p_velocity` [km/h] (BrakeForceR(), Mover.cpp:4606) - what a driver works out its
             * braking from (TController::CheckVehicles(), Driver.cpp:2286-2291) */
            virtual double get_force_at(double p_ratio, double p_velocity) const = 0;
            /* The brake's status (GetBrakeStatus(), hamulce.h:55-64): braking - the cylinder
             * filling (b_on), holding its pressure (b_hld), cut off from the train brake (b_dmg) */
            virtual bool is_braking() const = 0;
            virtual bool is_holding() const = 0;
            virtual bool is_cut_off() const = 0;
            /* The delay setting in use, a BrakeDelaySetting (BrakeDelayFlag) */
            virtual int get_delay_setting() const = 0;
            /* The distributor's control reservoir [MPa] (GetCRP()) */
            virtual double get_control_reservoir_pressure() const = 0;
            /**
             * @enum BrakeMethod
             * Enumeration representing various brake methods used in train systems.
             */
            enum BrakeMethod {
                BRAKE_METHOD_P10_BGU,
                BRAKE_METHOD_P10_BG,
                BRAKE_METHOD_D1,
                BRAKE_METHOD_D2,
                BRAKE_METHOD_FR513,
                BRAKE_METHOD_COSID,
                BRAKE_METHOD_P10Y_BG,
                BRAKE_METHOD_P10Y_BGU,
                BRAKE_METHOD_FR510,
                BRAKE_METHOD_D1MG,
                /* no BM= (BrakeMethod 0, Mover.cpp:10469) */
                BRAKE_METHOD_NONE,
            };
            enum CompressorPower {
                COMPRESSOR_POWER_MAIN = 0,
                /* the original's default (Mover.cpp:10512): from the converter, switched by hand */
                COMPRESSOR_POWER_CONVERTER_MANUAL = 1,
                COMPRESSOR_POWER_CONVERTER = 2,
                COMPRESSOR_POWER_ENGINE,
                COMPRESSOR_POWER_COUPLER1,
                COMPRESSOR_POWER_COUPLER2
            };
            /* BrakeHandle= / LocBrakeHandle= : shared handle-type enum for both the main and local (independent)
             * brake handles */
            enum BrakeHandleType {
                BRAKE_HANDLE_TYPE_NO_HANDLE,
                BRAKE_HANDLE_TYPE_WESTINGHOUSE,
                BRAKE_HANDLE_TYPE_FV4A,
                BRAKE_HANDLE_TYPE_M394,
                BRAKE_HANDLE_TYPE_M254,
                BRAKE_HANDLE_TYPE_FVE408,
                BRAKE_HANDLE_TYPE_FVEL6,
                BRAKE_HANDLE_TYPE_D2,
                BRAKE_HANDLE_TYPE_KNORR,
                BRAKE_HANDLE_TYPE_FD1,
                BRAKE_HANDLE_TYPE_BS2,
                BRAKE_HANDLE_TYPE_TESTH,
                BRAKE_HANDLE_TYPE_ST113,
                BRAKE_HANDLE_TYPE_MHZ_P,
                BRAKE_HANDLE_TYPE_MHZ_T,
                BRAKE_HANDLE_TYPE_MHZ_EN57,
                BRAKE_HANDLE_TYPE_MHZ_K5P,
                BRAKE_HANDLE_TYPE_MHZ_K8P,
                BRAKE_HANDLE_TYPE_MHZ_6P,
            };
            /* LocalBrake= */
            enum LocalBrakeType {
                LOCAL_BRAKE_TYPE_NONE,
                LOCAL_BRAKE_TYPE_MANUAL,
                LOCAL_BRAKE_TYPE_PNEUMATIC,
                LOCAL_BRAKE_TYPE_HYDRAULIC,
            };
            /* ASB= : anti-skid brake control method */
            /* How the train brake handle answers a key: a position per press, or moving while the key
               is held (OnCommand_trainbrakeincrease(), Train.cpp:1960-1966) */
            enum BrakeHandleMovement {
                BRAKE_HANDLE_MOVEMENT_STEPPED,
                BRAKE_HANDLE_MOVEMENT_CONTINUOUS,
            };
            enum AntiSkidBrakeType {
                ANTI_SKID_BRAKE_NONE,
                ANTI_SKID_BRAKE_MANUAL,
                ANTI_SKID_BRAKE_AUTOMATIC,
                /* "yes": the original's ASBType 128, the only kind a vehicle without a train brake
                   handle takes (Mover.cpp:10794-10815) */
                ANTI_SKID_BRAKE_YES,
            };
            /* DynamicBrake= */
            enum DynamicBrakeType {
                DYNAMIC_BRAKE_NONE = 0,
                DYNAMIC_BRAKE_PASSIVE = 1,
                DYNAMIC_BRAKE_SWITCH = 2,
                DYNAMIC_BRAKE_REVERSAL = 4,
                DYNAMIC_BRAKE_AUTOMATIC = 8,
            };
            /* BrakeDelays= : possible brake delay settings, named per the FIZ wiki */
            enum BrakeDelaySetting {
                /* no BrakeDelays= (Mover.cpp:10751) */
                BRAKE_DELAY_NONE = 0,
                BRAKE_DELAY_G = 1,
                BRAKE_DELAY_P = 2,
                BRAKE_DELAY_R = 4,
                BRAKE_DELAY_GP = 3,
                BRAKE_DELAY_PR = 6,
                BRAKE_DELAY_GPR = 7,
                BRAKE_DELAY_PR_MG = 14,
                BRAKE_DELAY_GPR_MG = 15,
            };
            /* The train brake handle's named positions (bh_MIN..bh_EPB, hamulce.h:111-121) - where
             * each one stands depends on the handle's type (TDriverHandle::GetPos()) */
            enum HandlePosition {
                HANDLE_POSITION_MIN = 0,
                HANDLE_POSITION_MAX = 1,
                HANDLE_POSITION_FILLING = 2,
                HANDLE_POSITION_DRIVE = 3,
                HANDLE_POSITION_CUTOFF = 4,
                HANDLE_POSITION_FIRST_STEP = 5,
                HANDLE_POSITION_FULL = 6,
                HANDLE_POSITION_EMERGENCY = 7,
                HANDLE_POSITION_EP_RELEASE = 8,
                HANDLE_POSITION_EP_HOLD = 9,
                HANDLE_POSITION_EP_BRAKE = 10,
            };
            virtual double get_handle_position(HandlePosition p_position) const = 0;
            /* A handle that sets the pipe pressure by how long it is held, not by where it stands
             * (TDriverHandle::Time) */
            virtual bool get_handle_time_controlled() const = 0;
            /* The EP brake is applied by how long the handle is held (TDriverHandle::TimeEP) */
            virtual bool get_handle_ep_time_controlled() const = 0;
            /* BrakeOpModes= */
            /* BrakeOpModes= - the operating modes a brake can be set to (bom_PS/PN/EP/MED, MOVER.h:325-328);
             * none when the FIZ does not say, as the original loads it (Mover.cpp:10743-10746) */
            enum BrakeOperationMode {
                BRAKE_OP_MODE_NONE = 0,
                BRAKE_OP_MODE_PN = 3,
                BRAKE_OP_MODE_PNEP = 7,
                BRAKE_OP_MODE_PNEPMED = 15,
            };
            /* BrakeSystem= */
            enum BrakeSystemType {
                BRAKE_SYSTEM_INDIVIDUAL,
                BRAKE_SYSTEM_PNEUMATIC,
                BRAKE_SYSTEM_ELECTRO_PNEUMATIC,
            };
            enum TrainBrakeValve {
                BRAKE_VALVE_NO_VALVE,
                BRAKE_VALVE_W,
                BRAKE_VALVE_W_LU_VI,
                BRAKE_VALVE_W_LU_L,
                BRAKE_VALVE_W_LU_XR,
                BRAKE_VALVE_K,
                BRAKE_VALVE_KG,
                BRAKE_VALVE_KP,
                BRAKE_VALVE_KSS,
                BRAKE_VALVE_KKG,
                BRAKE_VALVE_KKP,
                BRAKE_VALVE_KKS,
                BRAKE_VALVE_HIKG1,
                BRAKE_VALVE_HIKSS,
                BRAKE_VALVE_HIKP1,
                BRAKE_VALVE_KE,
                BRAKE_VALVE_SW,
                BRAKE_VALVE_ESTED,
                BRAKE_VALVE_NEST3,
                BRAKE_VALVE_EST3,
                BRAKE_VALVE_LST,
                BRAKE_VALVE_EST4,
                BRAKE_VALVE_EST3AL2,
                BRAKE_VALVE_EP1,
                BRAKE_VALVE_EP2,
                BRAKE_VALVE_M483,
                BRAKE_VALVE_CV1_L_TR,
                BRAKE_VALVE_CV1,
                BRAKE_VALVE_CV1_R,
                BRAKE_VALVE_OTHER
            };

        private:
            MAKE_MEMBER_GS_NR(TrainBrakeValve, valve_type, BRAKE_VALVE_NO_VALVE);
            /* BrakeValve= as the data writes it (BrakeValveParams): an ESt3's relays are read off it -
             * "PZZ" takes the independent brake's pressure, "AL2", "-s216", "-ED" (TNESt3::SetSize(),
             * Oerlikon_ESt.cpp) */
            String valve_parameters;

        public:
            void set_valve_parameters(const String &p_valve_parameters);
            String get_valve_parameters() const;

        private:
            MAKE_MEMBER_GS(int, est_valve_size, 0);
            MAKE_MEMBER_GS(int, friction_elements_per_axle, 1);
            MAKE_MEMBER_GS(double, brake_force_max, 1.0);
            MAKE_MEMBER_GS(double, brake_force_traction, 0.0);
            MAKE_MEMBER_GS(double, max_cylinder_pressure, 0.0);
            MAKE_MEMBER_GS(double, max_aux_pressure, 0.0);
            MAKE_MEMBER_GS(double, max_antislip_pressure, 0.0);
            MAKE_MEMBER_GS(double, max_tare_pressure, 0.0);
            MAKE_MEMBER_GS(double, max_medium_pressure, 0.0);
            MAKE_MEMBER_GS(int, cylinder_count, 0);
            MAKE_MEMBER_GS(double, cylinder_radius, 0.0);
            MAKE_MEMBER_GS(double, cylinder_distance, 0.0);
            MAKE_MEMBER_GS(double, cylinder_spring_force, 0.0);
            MAKE_MEMBER_GS(double, piston_stroke_adjuster_resistance, 0.0);
            MAKE_MEMBER_GS(double, cylinder_gear_ratio, 0.0);
            MAKE_MEMBER_GS(double, cylinder_gear_ratio_low, 0.0);
            MAKE_MEMBER_GS(double, cylinder_gear_ratio_high, 0.0);
            /* HiPP [bar]; 0 when the FIZ gives none - the control pipe then starts near 5 bar */
            MAKE_MEMBER_GS(double, pipe_pressure_max, 0.0);
            MAKE_MEMBER_GS(double, pipe_pressure_min, 3.5);
            MAKE_MEMBER_GS(double, tank_volume_main, 0.0);
            MAKE_MEMBER_GS(double, tank_volume_aux, 0.0);
            MAKE_MEMBER_GS(double, compressor_cab_a_min_pressure, 0.0);
            MAKE_MEMBER_GS(double, compressor_cab_a_max_pressure, 0.0);
            MAKE_MEMBER_GS(double, compressor_cab_b_min_pressure, 0.0);
            MAKE_MEMBER_GS(double, compressor_cab_b_max_pressure, 0.0);
            MAKE_MEMBER_GS(double, compressor_speed, 0.0);
            MAKE_MEMBER_GS(double, rapid_transfer, 1.0);
            MAKE_MEMBER_GS(double, rapid_switching_speed, 55.0);
            MAKE_MEMBER_GS_NR(CompressorPower, compressor_power, COMPRESSOR_POWER_CONVERTER_MANUAL);
            MAKE_MEMBER_GS_NR(BrakeMethod, brake_method, BRAKE_METHOD_NONE);
            MAKE_MEMBER_GS(double, rig_effectiveness, 0.0);
            MAKE_MEMBER_GS(double, air_leak_multiplier, 1.0);
            MAKE_MEMBER_GS(bool, compressor_tank_valve_active, false);
            MAKE_MEMBER_GS(double, compressor_lower_emergency_closing_pressure, -1.0);
            MAKE_MEMBER_GS(double, compressor_higher_emergency_closing_pressure, -1.0);
            /* LPOn=/LPOff= absent: the main pipe is never locked, no pressure is below it
               (LockPipeOn/Off -1, Mover.cpp:4533, 10505-10506) */
            static constexpr double MAIN_PIPE_LOCK_NONE = -1.0;
            /* HandlePipeUnlockPos= absent (HandleUnlock -3, Mover.cpp:10507) */
            static constexpr double MAIN_PIPE_UNLOCK_HANDLE_POSITION_NONE = -3.0;
            MAKE_MEMBER_GS(double, main_pipe_blocking_pressure, MAIN_PIPE_LOCK_NONE);
            MAKE_MEMBER_GS(double, main_pipe_unblocking_pressure, MAIN_PIPE_LOCK_NONE);
            MAKE_MEMBER_GS(double, main_pipe_minimum_unblocking_handle_position, MAIN_PIPE_UNLOCK_HANDLE_POSITION_NONE);

        public:
            virtual bool get_main_pipe_emergency_cuts_off_handle() const = 0;
            virtual void set_main_pipe_emergency_cuts_off_handle(bool p_value) = 0;
            MAKE_MEMBER_GS(bool, releaser_enabled_only_at_no_power_pos, false)
            MAKE_MEMBER_GS(double, compressor_emergency_valve_area, 0.0);
            MAKE_MEMBER_GS(int, universal_brake_button_1, 0);
            MAKE_MEMBER_GS(int, universal_brake_button_2, 0);
            MAKE_MEMBER_GS(int, universal_brake_button_3, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleBrakePressureTableItem>, brake_pressure_table)
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleCompressorListItem>, compressor_list)
            /* Cntrl. (czesc dotyczaca hamulca) */
            /* BrakeSystem= absent: Individual (Mover.cpp:10726) */
            MAKE_MEMBER_GS_NR(BrakeSystemType, cntrl_brake_system, BRAKE_SYSTEM_INDIVIDUAL);
            /* LoadFIZ_Cntrl's defaults (MOVER.h:1612-1630, 1696, 2045): no handle, no delays - a
               delay of 0 is taken from CheckLocomotiveParameters' table (Mover.cpp:12066-12073) */
            MAKE_MEMBER_GS(int, cntrl_brake_ctrl_position_count, 0);
            MAKE_MEMBER_GS_NR(BrakeDelaySetting, cntrl_brake_delays, BRAKE_DELAY_NONE);
            MAKE_MEMBER_GS(double, cntrl_brake_delay_1, 0.0);
            MAKE_MEMBER_GS(double, cntrl_brake_delay_2, 0.0);
            MAKE_MEMBER_GS(double, cntrl_brake_delay_3, 0.0);
            MAKE_MEMBER_GS(double, cntrl_brake_delay_4, 0.0);

        private:
            /* MaxBPMass= [t]: the mass at which the cylinders reach MaxBP, an empty car TareMaxBP and a
               load between them in proportion (MBPM, Mover.cpp:10771, TEStEP2::PLC(), hamulce.cpp:1264);
               0 when absent - the cylinders then always reach MaxBP */
            double cntrl_max_brake_pressure_mass = 0.0;

        public:
            double get_cntrl_max_brake_pressure_mass() const {
                return cntrl_max_brake_pressure_mass;
            }
            void set_cntrl_max_brake_pressure_mass(const double p_value) {
                cntrl_max_brake_pressure_mass = p_value;
            }
            MAKE_MEMBER_GS_NR(BrakeOperationMode, cntrl_brake_op_modes, BRAKE_OP_MODE_NONE);
            MAKE_MEMBER_GS_NR(BrakeHandleType, cntrl_brake_handle_type, BRAKE_HANDLE_TYPE_NO_HANDLE);

        private:
            /* How the handle answers a key; the positions brake_level_increase()/_decrease() move it
               (Global.fBrakeStep, Globals.h:194) */
            BrakeHandleMovement handle_movement = BRAKE_HANDLE_MOVEMENT_STEPPED;
            double handle_step = 1.0;

        public:
            BrakeHandleMovement get_handle_movement() const {
                return handle_movement;
            }
            void set_handle_movement(const BrakeHandleMovement p_value) {
                handle_movement = p_value;
            }
            double get_handle_step() const {
                return handle_step;
            }
            void set_handle_step(const double p_value) {
                handle_step = p_value;
            }
            MAKE_MEMBER_GS_NR(AntiSkidBrakeType, cntrl_anti_skid_brake_type, ANTI_SKID_BRAKE_NONE);
            MAKE_MEMBER_GS_NR(LocalBrakeType, cntrl_local_brake_type, LOCAL_BRAKE_TYPE_NONE);
            MAKE_MEMBER_GS_NR(BrakeHandleType, cntrl_local_brake_handle_type, BRAKE_HANDLE_TYPE_NO_HANDLE);
            MAKE_MEMBER_GS(bool, cntrl_manual_brake_present, false);
            MAKE_MEMBER_GS_NR(DynamicBrakeType, cntrl_dynamic_brake_type, DYNAMIC_BRAKE_NONE);
            MAKE_MEMBER_GS(bool, cntrl_local_brake_traxx, false);
            MAKE_MEMBER_GS(bool, cntrl_release_parking_by_spring_brake, false);
            MAKE_MEMBER_GS(bool, cntrl_release_parking_by_spring_brake_when_door_open, false);
            MAKE_MEMBER_GS(bool, cntrl_spring_brake_cuts_off_drive, true);
            MAKE_MEMBER_GS(double, cntrl_spring_brake_drive_emergency_velocity, -1.0);

        private:
            /* How much of the maximum force one block is making, 0..1. */
        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            virtual void brake_releaser(bool p_pressed) = 0;
            /* The releaser of this vehicle's valve switched directly, whatever the cab's conditions
             * - what the original's consistreleaser does to each vehicle of a trainset
             * (simulation.cpp:184). Switched on, it stays on until the brakes stop braking. */
            virtual void consist_releaser(bool p_active) = 0;
            /* The compressor switched (CompressorSwitch(), Mover.cpp:3724): the cab's own, sent along
             * the control line to the vehicles that carry one */
            virtual void compressor(bool p_enabled) = 0;
            virtual void brake_level_set(double p_level) = 0;
            /* A named handle position - "min", "max", "drive", "full", "emergency" */
            virtual void brake_level_set_position(const String &p_position) = 0;
            virtual void brake_level_increase() = 0;
            virtual void brake_level_decrease() = 0;
            virtual void local_brake_set(double p_level) = 0;
            virtual void local_brake_increase() = 0;
            virtual void local_brake_decrease() = 0;
            virtual void manual_brake_increase() = 0;
            virtual void manual_brake_decrease() = 0;
            virtual void auto_rewident(int p_brake_delay) = 0;
            /* The next or the previous operation mode the brake has (BrakeOpModeFlag, one bit of
             * BrakeOpModes; OnCommand_trainbrakeoperationmodeincrease/decrease, Train.cpp:2445-2478) */
            virtual void brake_operation_mode_increase() = 0;
            virtual void brake_operation_mode_decrease() = 0;
            /* The electro-pneumatic brake applied or released (SwitchEPBrake(), Mover.cpp:4153); true
             * when it changed */
            virtual bool ep_brake(bool p_applied) = 0;
            virtual int get_operation_mode() const = 0;
            virtual void brake_level_charging(bool p_active) = 0;
            virtual void alarm_chain(bool p_pulled) = 0;
            /* One of the vehicle's universal brake buttons (UBB1..3 in the FIZ), 0-based */
            virtual void universal_brake_button(int p_button, bool p_pressed) = 0;
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleBrake::CompressorPower)
VARIANT_ENUM_CAST(RailVehicleBrake::TrainBrakeValve)
VARIANT_ENUM_CAST(RailVehicleBrake::BrakeMethod)
VARIANT_ENUM_CAST(RailVehicleBrake::BrakeHandleType)
VARIANT_ENUM_CAST(RailVehicleBrake::LocalBrakeType)
VARIANT_ENUM_CAST(RailVehicleBrake::AntiSkidBrakeType)
VARIANT_ENUM_CAST(RailVehicleBrake::BrakeHandleMovement)
VARIANT_ENUM_CAST(RailVehicleBrake::DynamicBrakeType)
VARIANT_ENUM_CAST(RailVehicleBrake::BrakeDelaySetting)
VARIANT_ENUM_CAST(RailVehicleBrake::HandlePosition)
VARIANT_ENUM_CAST(RailVehicleBrake::BrakeOperationMode)
VARIANT_ENUM_CAST(RailVehicleBrake::BrakeSystemType)
