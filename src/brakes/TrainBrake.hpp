#pragma once
#include "../core/TrainPart.hpp"
#include "macros.hpp"
#include "resources/brakes/BrakePressureTableItem.hpp"
#include "resources/brakes/CompressorListItem.hpp"
#include <godot_cpp/classes/node.hpp>
#include <unordered_map>

#define ASSERT_MOVER_BRAKE(mover_ptr, ...)                                                                             \
    if ((mover_ptr) == nullptr || mover_ptr->Hamulec == nullptr) {                                                     \
        return __VA_ARGS__;                                                                                            \
    }

namespace godot {
    class TrainController;
    class TrainBrake : public TrainPart {
            GDCLASS(TrainBrake, TrainPart)
        public:
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
            };

            const std::unordered_map<BrakeMethod, int> brake_method_map = {
                    {BrakeMethod::BRAKE_METHOD_P10_BGU, 1},  {BrakeMethod::BRAKE_METHOD_P10_BG, 2},
                    {BrakeMethod::BRAKE_METHOD_D1, 9},       {BrakeMethod::BRAKE_METHOD_D2, 10},
                    {BrakeMethod::BRAKE_METHOD_FR513, 11},   {BrakeMethod::BRAKE_METHOD_COSID, 12},
                    {BrakeMethod::BRAKE_METHOD_P10Y_BG, 14}, {BrakeMethod::BRAKE_METHOD_P10Y_BGU, 16},
                    {BrakeMethod::BRAKE_METHOD_FR510, 17},   {BrakeMethod::BRAKE_METHOD_D1MG, 137},
            };

            enum BrakeHandlePosition {
                BRAKE_HANDLE_POSITION_MIN = 0,
                BRAKE_HANDLE_POSITION_MAX = 1,
                BRAKE_HANDLE_POSITION_DRIVE = 2,
                BRAKE_HANDLE_POSITION_FULL = 3,
                BRAKE_HANDLE_POSITION_EMERGENCY = 4,
            };

            enum CompressorPower {
                COMPRESSOR_POWER_MAIN = 0,
                COMPRESSOR_POWER_UNUSED = 1,
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
            enum AntiSkidBrakeType {
                ANTI_SKID_BRAKE_NONE,
                ANTI_SKID_BRAKE_MANUAL,
                ANTI_SKID_BRAKE_AUTOMATIC,
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
                BRAKE_DELAY_G = 1,
                BRAKE_DELAY_P = 2,
                BRAKE_DELAY_R = 4,
                BRAKE_DELAY_GP = 3,
                BRAKE_DELAY_PR = 6,
                BRAKE_DELAY_GPR = 7,
                BRAKE_DELAY_PR_MG = 14,
                BRAKE_DELAY_GPR_MG = 15,
            };

            /* BrakeOpModes= */
            enum BrakeOperationMode {
                BRAKE_OP_MODE_PN = 3,
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
            const std::unordered_map<BrakeHandlePosition, int> brake_handle_position_map = {
                    {BrakeHandlePosition::BRAKE_HANDLE_POSITION_MIN, Maszyna::bh_MIN},
                    {BrakeHandlePosition::BRAKE_HANDLE_POSITION_MAX, Maszyna::bh_MAX},
                    {BrakeHandlePosition::BRAKE_HANDLE_POSITION_DRIVE, Maszyna::bh_RP},
                    {BrakeHandlePosition::BRAKE_HANDLE_POSITION_FULL, Maszyna::bh_FB},
                    {BrakeHandlePosition::BRAKE_HANDLE_POSITION_EMERGENCY, Maszyna::bh_EB},
            };
            const std::unordered_map<std::string, int> brake_handle_position_string_map = {
                    {"min", Maszyna::bh_MIN}, {"max", Maszyna::bh_MAX},      {"drive", Maszyna::bh_RP},
                    {"full", Maszyna::bh_FB}, {"emergency", Maszyna::bh_EB},
            };
            const std::unordered_map<TBrakeValve, TBrakeSubSystem> brake_valve_to_subsystem_map = {
                    {TBrakeValve::W, TBrakeSubSystem::ss_W},       {TBrakeValve::W_Lu_L, TBrakeSubSystem::ss_W},
                    {TBrakeValve::W_Lu_VI, TBrakeSubSystem::ss_W}, {TBrakeValve::W_Lu_XR, TBrakeSubSystem::ss_W},
                    {TBrakeValve::ESt3, TBrakeSubSystem::ss_ESt},  {TBrakeValve::ESt3AL2, TBrakeSubSystem::ss_ESt},
                    {TBrakeValve::ESt4, TBrakeSubSystem::ss_ESt},  {TBrakeValve::EP2, TBrakeSubSystem::ss_ESt},
                    {TBrakeValve::EP1, TBrakeSubSystem::ss_ESt},   {TBrakeValve::KE, TBrakeSubSystem::ss_KE},
                    {TBrakeValve::CV1, TBrakeSubSystem::ss_Dako},  {TBrakeValve::CV1_L_TR, TBrakeSubSystem::ss_Dako},
                    {TBrakeValve::LSt, TBrakeSubSystem::ss_LSt},   {TBrakeValve::EStED, TBrakeSubSystem::ss_LSt}};

            const std::unordered_map<BrakePressureTableItem::BrakeType, Maszyna::TBrakeSystem>
                    brake_pressure_table_type_map = {
                            {BrakePressureTableItem::BRAKE_TYPE_PNEUMATIC, Maszyna::TBrakeSystem::Pneumatic},
                            {BrakePressureTableItem::BRAKE_TYPE_ELECTRO_PNEUMATIC,
                             Maszyna::TBrakeSystem::ElectroPneumatic},
                            {BrakePressureTableItem::BRAKE_TYPE_INDIVIDUAL, Maszyna::TBrakeSystem::Individual},
                    };

            const std::unordered_map<BrakeHandleType, Maszyna::TBrakeHandle> brake_handle_type_map = {
                    {BRAKE_HANDLE_TYPE_NO_HANDLE, Maszyna::TBrakeHandle::NoHandle},
                    {BRAKE_HANDLE_TYPE_WESTINGHOUSE, Maszyna::TBrakeHandle::West},
                    {BRAKE_HANDLE_TYPE_FV4A, Maszyna::TBrakeHandle::FV4a},
                    {BRAKE_HANDLE_TYPE_M394, Maszyna::TBrakeHandle::M394},
                    {BRAKE_HANDLE_TYPE_M254, Maszyna::TBrakeHandle::M254},
                    {BRAKE_HANDLE_TYPE_FVE408, Maszyna::TBrakeHandle::FVE408},
                    {BRAKE_HANDLE_TYPE_FVEL6, Maszyna::TBrakeHandle::FVel6},
                    {BRAKE_HANDLE_TYPE_D2, Maszyna::TBrakeHandle::D2},
                    {BRAKE_HANDLE_TYPE_KNORR, Maszyna::TBrakeHandle::Knorr},
                    {BRAKE_HANDLE_TYPE_FD1, Maszyna::TBrakeHandle::FD1},
                    {BRAKE_HANDLE_TYPE_BS2, Maszyna::TBrakeHandle::BS2},
                    {BRAKE_HANDLE_TYPE_TESTH, Maszyna::TBrakeHandle::testH},
                    {BRAKE_HANDLE_TYPE_ST113, Maszyna::TBrakeHandle::St113},
                    {BRAKE_HANDLE_TYPE_MHZ_P, Maszyna::TBrakeHandle::MHZ_P},
                    {BRAKE_HANDLE_TYPE_MHZ_T, Maszyna::TBrakeHandle::MHZ_T},
                    {BRAKE_HANDLE_TYPE_MHZ_EN57, Maszyna::TBrakeHandle::MHZ_EN57},
                    {BRAKE_HANDLE_TYPE_MHZ_K5P, Maszyna::TBrakeHandle::MHZ_K5P},
                    {BRAKE_HANDLE_TYPE_MHZ_K8P, Maszyna::TBrakeHandle::MHZ_K8P},
                    {BRAKE_HANDLE_TYPE_MHZ_6P, Maszyna::TBrakeHandle::MHZ_6P},
            };

            const std::unordered_map<BrakeSystemType, Maszyna::TBrakeSystem> brake_system_type_map = {
                    {BRAKE_SYSTEM_INDIVIDUAL, Maszyna::TBrakeSystem::Individual},
                    {BRAKE_SYSTEM_PNEUMATIC, Maszyna::TBrakeSystem::Pneumatic},
                    {BRAKE_SYSTEM_ELECTRO_PNEUMATIC, Maszyna::TBrakeSystem::ElectroPneumatic},
            };

            const std::unordered_map<LocalBrakeType, Maszyna::TLocalBrake> local_brake_type_map = {
                    {LOCAL_BRAKE_TYPE_NONE, Maszyna::TLocalBrake::NoBrake},
                    {LOCAL_BRAKE_TYPE_MANUAL, Maszyna::TLocalBrake::ManualBrake},
                    {LOCAL_BRAKE_TYPE_PNEUMATIC, Maszyna::TLocalBrake::PneumaticBrake},
                    {LOCAL_BRAKE_TYPE_HYDRAULIC, Maszyna::TLocalBrake::HydraulicBrake},
            };

            MAKE_MEMBER_GS_NR(
                    TrainBrakeValve, valve_type, static_cast<TrainBrakeValve>(static_cast<int>(TBrakeValve::NoValve)));
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
            MAKE_MEMBER_GS(double, pipe_pressure_max, 5.0);
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
            MAKE_MEMBER_GS_NR(CompressorPower, compressor_power, COMPRESSOR_POWER_MAIN);
            MAKE_MEMBER_GS_NR(BrakeMethod, brake_method, BRAKE_METHOD_P10_BGU);
            MAKE_MEMBER_GS(double, rig_effectiveness, 0.0);
            MAKE_MEMBER_GS(double, air_leak_multiplier, 1.0);

            MAKE_MEMBER_GS(bool, compressor_tank_valve_active, false);
            MAKE_MEMBER_GS(double, compressor_lower_emergency_closing_pressure, -1.0);
            MAKE_MEMBER_GS(double, compressor_higher_emergency_closing_pressure, -1.0);
            MAKE_MEMBER_GS(double, main_pipe_blocking_pressure, 0.0);
            MAKE_MEMBER_GS(double, main_pipe_unblocking_pressure, 0.0);
            MAKE_MEMBER_GS(int, main_pipe_minimum_unblocking_handle_position, -3.0);
            MAKE_MEMBER_GS(bool, releaser_enabled_only_at_no_power_pos, false)
            MAKE_MEMBER_GS(double, emergency_valve_area, 0.0);
            MAKE_MEMBER_GS(int, universal_brake_button_1, 0);
            MAKE_MEMBER_GS(int, universal_brake_button_2, 0);
            MAKE_MEMBER_GS(int, universal_brake_button_3, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<BrakePressureTableItem>, brake_pressure_table)
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<CompressorListItem>, compressor_list)

            /* Cntrl. (czesc dotyczaca hamulca) */
            MAKE_MEMBER_GS_NR(BrakeSystemType, brake_system, BRAKE_SYSTEM_PNEUMATIC);
            MAKE_MEMBER_GS(int, brake_ctrl_position_count, 6);
            MAKE_MEMBER_GS_NR(BrakeDelaySetting, brake_delays, BRAKE_DELAY_GP);
            MAKE_MEMBER_GS(double, brake_delay_1, 15.0);
            MAKE_MEMBER_GS(double, brake_delay_2, 3.0);
            MAKE_MEMBER_GS(double, brake_delay_3, 36.0);
            MAKE_MEMBER_GS(double, brake_delay_4, 22.0);
            MAKE_MEMBER_GS_NR(BrakeOperationMode, brake_op_modes, BRAKE_OP_MODE_PNEPMED);
            MAKE_MEMBER_GS_NR(BrakeHandleType, brake_handle_type, BRAKE_HANDLE_TYPE_FV4A);
            MAKE_MEMBER_GS_NR(AntiSkidBrakeType, anti_skid_brake_type, ANTI_SKID_BRAKE_MANUAL);
            MAKE_MEMBER_GS_NR(LocalBrakeType, local_brake_type, LOCAL_BRAKE_TYPE_PNEUMATIC);
            MAKE_MEMBER_GS_NR(BrakeHandleType, local_brake_handle_type, BRAKE_HANDLE_TYPE_FD1);
            MAKE_MEMBER_GS(bool, manual_brake_present, true);
            MAKE_MEMBER_GS_NR(DynamicBrakeType, dynamic_brake_type, DYNAMIC_BRAKE_NONE);
            MAKE_MEMBER_GS(bool, local_brake_traxx, false);
            MAKE_MEMBER_GS(bool, release_parking_by_spring_brake, false);
            MAKE_MEMBER_GS(bool, release_parking_by_spring_brake_when_door_open, false);
            MAKE_MEMBER_GS(bool, spring_brake_cuts_off_drive, true);
            MAKE_MEMBER_GS(double, spring_brake_drive_emergency_velocity, -1.0);

        protected:
            void _do_update_internal_mover(TMoverParameters *p_mover) override;
            void _do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) override;
            void _do_fetch_config_from_mover(TMoverParameters *p_mover, Dictionary &p_config) override;
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            static void _bind_methods();
            void brake_releaser(bool p_pressed);
            void brake_level_set(double p_level);
            void brake_level_set_position(BrakeHandlePosition p_position);
            void brake_level_set_position_str(const String &p_position);
            void brake_level_increase();
            void brake_level_decrease();
    };
} // namespace godot
VARIANT_ENUM_CAST(TrainBrake::CompressorPower)
VARIANT_ENUM_CAST(TrainBrake::TrainBrakeValve)
VARIANT_ENUM_CAST(TrainBrake::BrakeHandlePosition)
VARIANT_ENUM_CAST(TrainBrake::BrakeMethod)
VARIANT_ENUM_CAST(TrainBrake::BrakeHandleType)
VARIANT_ENUM_CAST(TrainBrake::LocalBrakeType)
VARIANT_ENUM_CAST(TrainBrake::AntiSkidBrakeType)
VARIANT_ENUM_CAST(TrainBrake::DynamicBrakeType)
VARIANT_ENUM_CAST(TrainBrake::BrakeDelaySetting)
VARIANT_ENUM_CAST(TrainBrake::BrakeOperationMode)
VARIANT_ENUM_CAST(TrainBrake::BrakeSystemType)
