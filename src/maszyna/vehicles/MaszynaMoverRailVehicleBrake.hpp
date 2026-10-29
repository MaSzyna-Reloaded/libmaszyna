#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverBrakePressureTableItem.hpp"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "maszyna/vehicles/MaszynaMoverCompressorListItem.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include <string>
#include <unordered_map>

namespace godot {
    /* RailVehicleBrake on the vendored Mover - the only class here that knows TMoverParameters. */
    class MaszynaMoverRailVehicleBrake : public RailVehicleBrake, public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleBrake, RailVehicleBrake);

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
                /* no BM= (BrakeMethod 0, Mover.cpp:10469) */
                BRAKE_METHOD_NONE,
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

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            bool get_alarm_chain_pulled() const override;
            double get_air_pressure() const override;
            double get_loco_pressure() const override;
            double get_pipe_pressure() const override;
            double get_feed_pipe_pressure() const override;
            double get_tank_volume() const override;
            double get_compressor_pressure() const override;
            bool get_compressor_enabled() const override;
            bool get_compressor_allowed() const override;
            double get_controller_position() const override;
            double get_controller_position_normalized() const override;
            double get_local_position_normalized() const override;
            int get_manual_position() const override;
            double get_unit_force() const override;
            double get_force_ratio() const override;
            double get_emergency_valve_flow() const;
            double get_main_valve_flow() const;
            double get_local_valve_flow() const override;
            /* The flows of the driver's brake valve its handle reports for its hiss
             * (Handle->GetSound(), hamulce.h:77-81): braking, release, emergency, the control
             * chamber's wave outflow and the timing reservoir's outflow */
            double get_handle_braking_flow() const;
            double get_handle_release_flow() const;
            double get_handle_emergency_flow() const;
            double get_handle_control_chamber_flow() const;
            double get_handle_timing_reservoir_flow() const;
            double get_control_pressure() const override;
            double get_handle_control_pressure() const override;
            double get_local_aeim_position() const;
            double get_edb_cylinder_pressure() const;
            bool get_releaser_active() const override;
            bool get_main_pipe_locked() const override;
            double get_force() const override;
            double get_force_at(double p_ratio, double p_velocity) const override;
            bool is_braking() const override;
            bool is_holding() const override;
            bool is_cut_off() const override;
            /* The delay setting in use, a BrakeDelaySetting (BrakeDelayFlag) */
            int get_delay_setting() const;
            double get_control_reservoir_pressure() const override;
            double get_handle_position(HandlePosition p_position) const override;
            bool get_handle_time_controlled() const override;
            bool get_handle_ep_time_controlled() const override;

        private:
            const std::unordered_map<BrakeMethod, int> brake_method_map = {
                    {BrakeMethod::BRAKE_METHOD_P10_BGU, 1},  {BrakeMethod::BRAKE_METHOD_P10_BG, 2},
                    {BrakeMethod::BRAKE_METHOD_D1, 9},       {BrakeMethod::BRAKE_METHOD_D2, 10},
                    {BrakeMethod::BRAKE_METHOD_FR513, 11},   {BrakeMethod::BRAKE_METHOD_COSID, 12},
                    {BrakeMethod::BRAKE_METHOD_P10Y_BG, 14}, {BrakeMethod::BRAKE_METHOD_P10Y_BGU, 16},
                    {BrakeMethod::BRAKE_METHOD_FR510, 17},   {BrakeMethod::BRAKE_METHOD_D1MG, 137},
                    {BrakeMethod::BRAKE_METHOD_NONE, 0},
            };
            /* ASBType: 1 manual, 2 automatic, 128 "yes" (Mover.cpp:10794-10815) */
            const std::unordered_map<AntiSkidBrakeType, int> anti_skid_brake_type_map = {
                    {ANTI_SKID_BRAKE_NONE, 0},
                    {ANTI_SKID_BRAKE_MANUAL, 1},
                    {ANTI_SKID_BRAKE_AUTOMATIC, 2},
                    {ANTI_SKID_BRAKE_YES, 128},
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
            const std::unordered_map<MaszynaMoverBrakePressureTableItem::BrakeType, Maszyna::TBrakeSystem>
                    brake_pressure_table_type_map = {
                            {MaszynaMoverBrakePressureTableItem::BRAKE_TYPE_PNEUMATIC,
                             Maszyna::TBrakeSystem::Pneumatic},
                            {MaszynaMoverBrakePressureTableItem::BRAKE_TYPE_ELECTRO_PNEUMATIC,
                             Maszyna::TBrakeSystem::ElectroPneumatic},
                            {MaszynaMoverBrakePressureTableItem::BRAKE_TYPE_INDIVIDUAL,
                             Maszyna::TBrakeSystem::Individual},
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
            bool main_pipe_emergency_cuts_off_handle = false;

        public:
            bool get_main_pipe_emergency_cuts_off_handle() const override {
                return main_pipe_emergency_cuts_off_handle;
            }
            void set_main_pipe_emergency_cuts_off_handle(const bool p_value) override {
                main_pipe_emergency_cuts_off_handle = p_value;
            }

        protected:
            void _apply_configuration() override;
            void _do_process_component(double p_delta) override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

        public:
            void brake_releaser(bool p_pressed) override;
            void consist_releaser(bool p_active) override;
            void compressor(bool p_enabled) override;
            void brake_level_set(double p_level) override;
            void brake_level_set_position(const String &p_position) override;
            void brake_level_increase() override;
            void brake_level_decrease() override;
            void local_brake_set(double p_level) override;
            void local_brake_increase() override;
            void local_brake_decrease() override;
            void manual_brake_increase() override;
            void manual_brake_decrease() override;
            void auto_rewident(int p_brake_delay) override;
            void brake_operation_mode_increase() override;
            void brake_operation_mode_decrease() override;
            bool ep_brake(bool p_applied) override;
            int get_operation_mode() const;
            void brake_level_charging(bool p_active) override;
            void alarm_chain(bool p_pulled) override;
            void universal_brake_button(int p_button, bool p_pressed) override;

        private:
            TrainBrakeValve valve_type = BRAKE_VALVE_NO_VALVE;
            /* BrakeValve= as the data writes it (BrakeValveParams): an ESt3's relays are read off it -
             * "PZZ" takes the independent brake's pressure, "AL2", "-s216", "-ED" (TNESt3::SetSize(),
             * Oerlikon_ESt.cpp) */
            String valve_parameters;
            int est_valve_size = 0;
            double cylinder_spring_force = 0.0;
            double piston_stroke_adjuster_resistance = 0.0;
            double rig_effectiveness = 0.0;
            double rapid_transfer = 1.0;
            double air_leak_multiplier = 1.0;
            bool compressor_tank_valve_active = false;
            double main_pipe_minimum_unblocking_handle_position = MAIN_PIPE_UNLOCK_HANDLE_POSITION_NONE;
            TypedArray<MaszynaMoverBrakePressureTableItem> brake_pressure_table;
            TypedArray<MaszynaMoverCompressorListItem> compressor_list;
            double compressor_emergency_valve_area = 0.0;
            BrakeHandleType cntrl_brake_handle_type = BRAKE_HANDLE_TYPE_NO_HANDLE;
            BrakeHandleType cntrl_local_brake_handle_type = BRAKE_HANDLE_TYPE_NO_HANDLE;
            bool cntrl_local_brake_traxx = false;
            BrakeMethod brake_method = BRAKE_METHOD_NONE;

        public:
            MAKE_GS_NR(TrainBrakeValve, valve_type);
            MAKE_GS(int, est_valve_size);
            MAKE_GS(double, cylinder_spring_force);
            MAKE_GS(double, piston_stroke_adjuster_resistance);
            MAKE_GS(double, rig_effectiveness);
            MAKE_GS(double, rapid_transfer);
            MAKE_GS(double, air_leak_multiplier);
            MAKE_GS(bool, compressor_tank_valve_active);
            MAKE_GS(double, main_pipe_minimum_unblocking_handle_position);
            MAKE_GS_NR(TypedArray<MaszynaMoverBrakePressureTableItem>, brake_pressure_table);
            MAKE_GS_NR(TypedArray<MaszynaMoverCompressorListItem>, compressor_list);
            MAKE_GS(double, compressor_emergency_valve_area);
            MAKE_GS_NR(BrakeHandleType, cntrl_brake_handle_type);
            MAKE_GS_NR(BrakeHandleType, cntrl_local_brake_handle_type);
            MAKE_GS(bool, cntrl_local_brake_traxx);
            MAKE_GS_NR(BrakeMethod, brake_method);
            void set_valve_parameters(const String &p_valve_parameters);
            String get_valve_parameters() const;
    };
} // namespace godot

VARIANT_ENUM_CAST(MaszynaMoverRailVehicleBrake::TrainBrakeValve)
VARIANT_ENUM_CAST(MaszynaMoverRailVehicleBrake::BrakeMethod)
VARIANT_ENUM_CAST(MaszynaMoverRailVehicleBrake::BrakeHandleType)
