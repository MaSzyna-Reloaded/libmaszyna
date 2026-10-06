#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"

namespace godot {
    /* RailVehicleBrake on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleBrake : public RailVehicleBrake, public MoverComponent {
            GDCLASS(MoverRailVehicleBrake, RailVehicleBrake);

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
            double get_pipe_brake_pressure() const override;
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
            double get_emergency_valve_flow() const override;
            double get_main_valve_flow() const override;
            double get_local_valve_flow() const override;
            double get_handle_braking_flow() const override;
            double get_handle_release_flow() const override;
            double get_handle_emergency_flow() const override;
            double get_handle_control_chamber_flow() const override;
            double get_handle_timing_reservoir_flow() const override;
            double get_control_pressure() const override;
            double get_handle_control_pressure() const override;
            double get_local_aeim_position() const override;
            double get_edb_cylinder_pressure() const override;
            bool get_releaser_active() const override;
            bool get_main_pipe_locked() const override;
            double get_force() const override;
            double get_force_at(double p_ratio, double p_velocity) const override;
            bool is_braking() const override;
            bool is_holding() const override;
            bool is_cut_off() const override;
            int get_delay_setting() const override;
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
            const std::unordered_map<RailVehicleBrakePressureTableItem::BrakeType, Maszyna::TBrakeSystem>
                    brake_pressure_table_type_map = {
                            {RailVehicleBrakePressureTableItem::BRAKE_TYPE_PNEUMATIC, Maszyna::TBrakeSystem::Pneumatic},
                            {RailVehicleBrakePressureTableItem::BRAKE_TYPE_ELECTRO_PNEUMATIC,
                             Maszyna::TBrakeSystem::ElectroPneumatic},
                            {RailVehicleBrakePressureTableItem::BRAKE_TYPE_INDIVIDUAL,
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
            int get_operation_mode() const override;
            void brake_level_charging(bool p_active) override;
            void alarm_chain(bool p_pulled) override;
            void universal_brake_button(int p_button, bool p_pressed) override;
    };
} // namespace godot
