#include "utils/utils.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include <algorithm>
#include <cmath>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *RailVehicleBrake::accelerator_activated_signal = "accelerator_activated";

    void RailVehicleBrake::set_valve_parameters(const String &p_valve_parameters) {
        valve_parameters = p_valve_parameters;
    }

    String RailVehicleBrake::get_valve_parameters() const {
        return valve_parameters;
    }

    void RailVehicleBrake::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, valve_type, "valve", PROPERTY_HINT_ENUM,
                "NoValve,W,W_Lu_VI,W_Lu_L,W_Lu_XR,K,Kg,Kp,Kss,Kkg,Kkp,Kks,Hikg1,Hikss,Hikp1,KE,SW,EStED,NESt3,ESt3,LSt,"
                "ESt4,ESt3AL2,EP1,EP2,M483,CV1_L_TR,CV1,CV1_R,Other")
        BIND_PROPERTY(RailVehicleBrake, Variant::STRING, valve_parameters, "valve");
        BIND_PROPERTY(RailVehicleBrake, Variant::INT, friction_elements_per_axle);
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, brake_force_max, "brake_force");
        BIND_PROPERTY(RailVehicleBrake, Variant::INT, est_valve_size, "est_valve");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, brake_force_traction, "brake_force");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, max_cylinder_pressure);
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, max_aux_pressure);
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, max_tare_pressure);
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, max_medium_pressure);
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, max_antislip_pressure);
        BIND_PROPERTY(RailVehicleBrake, Variant::INT, cylinder_count, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cylinder_radius, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cylinder_distance, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cylinder_spring_force, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, piston_stroke_adjuster_resistance, "piston_stroke");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cylinder_gear_ratio, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cylinder_gear_ratio_low, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cylinder_gear_ratio_high, "cylinder");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, pipe_pressure_min, "pipe");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, pipe_pressure_max, "pipe");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, tank_volume_main, "tank");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, tank_volume_aux, "tank");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_cab_a_min_pressure, "compressor/cab_a");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_cab_a_max_pressure, "compressor/cab_a");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_cab_b_min_pressure, "compressor/cab_b");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_cab_b_max_pressure, "compressor/cab_b");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_speed, "compressor");
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, compressor_power, "compressor", PROPERTY_HINT_ENUM,
                "Main,ConverterManual,Converter,Engine,Coupler1,Coupler2");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, rig_effectiveness);
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, brake_method, "brake", PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"P10-Bgu", BRAKE_METHOD_P10_BGU},
                         {"P10-Bg", BRAKE_METHOD_P10_BG},
                         {"Disk1", BRAKE_METHOD_D1},
                         {"Disk2", BRAKE_METHOD_D2},
                         {"FR513", BRAKE_METHOD_FR513},
                         {"Cosid", BRAKE_METHOD_COSID},
                         {"P10yBg", BRAKE_METHOD_P10Y_BG},
                         {"P10yBgu", BRAKE_METHOD_P10Y_BGU},
                         {"FR510", BRAKE_METHOD_FR510},
                         {"Disk1+Mg", BRAKE_METHOD_D1MG},
                         {"None", BRAKE_METHOD_NONE}}));
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, rapid_transfer, "rapid");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, rapid_switching_speed, "rapid");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, air_leak_multiplier)
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, compressor_tank_valve_active, "compressor")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_lower_emergency_closing_pressure, "compressor")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_higher_emergency_closing_pressure, "compressor")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, main_pipe_blocking_pressure, "main_pipe")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, main_pipe_unblocking_pressure, "main_pipe")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, main_pipe_minimum_unblocking_handle_position, "main_pipe")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, main_pipe_emergency_cuts_off_handle, "main_pipe")
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleBrake, Variant::ARRAY, brake_pressure_table, PROPERTY_HINT_TYPE_STRING,
                "RailVehicleBrakePressureTableItem");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleBrake, Variant::ARRAY, compressor_list, PROPERTY_HINT_TYPE_STRING,
                "RailVehicleCompressorListItem");
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, compressor_emergency_valve_area, "compressor")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, releaser_enabled_only_at_no_power_pos, "releaser")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, universal_brake_button_1, "universal_brake_button", PROPERTY_HINT_FLAGS,
                "Releaser,Bridge Emergency Valve,High Pressure Impulse,Assimilation,Anti-Skid Brake")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, universal_brake_button_2, "universal_brake_button", PROPERTY_HINT_FLAGS,
                "Releaser,Bridge Emergency Valve,High Pressure Impulse,Assimilation,Anti-Skid Brake")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, universal_brake_button_3, "universal_brake_button", PROPERTY_HINT_FLAGS,
                "Releaser,Bridge Emergency Valve,High Pressure Impulse,Assimilation,Anti-Skid Brake")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_brake_system, "cntrl", PROPERTY_HINT_ENUM,
                "Individual,Pneumatic,ElectroPneumatic")
        BIND_PROPERTY(RailVehicleBrake, Variant::INT, cntrl_brake_ctrl_position_count, "cntrl")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_brake_delays, "cntrl", PROPERTY_HINT_ENUM,
                "None:0,G:1,P:2,R:4,GP:3,PR:6,GPR:7,PR+Mg:14,GPR+Mg:15")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cntrl_brake_delay_1, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cntrl_brake_delay_2, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cntrl_brake_delay_3, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cntrl_brake_delay_4, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cntrl_max_brake_pressure_mass, "cntrl")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, handle_movement, "handle", PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"Stepped", BRAKE_HANDLE_MOVEMENT_STEPPED}, {"Continuous", BRAKE_HANDLE_MOVEMENT_CONTINUOUS}}))
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, handle_step, "handle")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_brake_op_modes, "cntrl", PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"None", BRAKE_OP_MODE_NONE},
                         {"PN", BRAKE_OP_MODE_PN},
                         {"PNEP", BRAKE_OP_MODE_PNEP},
                         {"PNEPMED", BRAKE_OP_MODE_PNEPMED}}))
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_brake_handle_type, "cntrl", PROPERTY_HINT_ENUM,
                "NoHandle,Westinghouse,FV4a,M394,M254,FVE408,FVel6,D2,Knorr,FD1,BS2,testH,St113,MHZ_P,MHZ_T,MHZ_EN57,"
                "MHZ_K5P,MHZ_K8P,MHZ_6P")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_anti_skid_brake_type, "cntrl", PROPERTY_HINT_ENUM,
                "None,Manual,Automatic,Yes")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_local_brake_type, "cntrl", PROPERTY_HINT_ENUM,
                "None,Manual,Pneumatic,Hydraulic")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_local_brake_handle_type, "cntrl", PROPERTY_HINT_ENUM,
                "NoHandle,Westinghouse,FV4a,M394,M254,FVE408,FVel6,D2,Knorr,FD1,BS2,testH,St113,MHZ_P,MHZ_T,MHZ_EN57,"
                "MHZ_K5P,MHZ_K8P,MHZ_6P")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, cntrl_manual_brake_present, "cntrl")
        BIND_PROPERTY_W_HINT(
                RailVehicleBrake, Variant::INT, cntrl_dynamic_brake_type, "cntrl", PROPERTY_HINT_ENUM,
                "None:0,Passive:1,Switch:2,Reversal:4,Automatic:8")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, cntrl_local_brake_traxx, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, cntrl_release_parking_by_spring_brake, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, cntrl_release_parking_by_spring_brake_when_door_open, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::BOOL, cntrl_spring_brake_cuts_off_drive, "cntrl")
        BIND_PROPERTY(RailVehicleBrake, Variant::FLOAT, cntrl_spring_brake_drive_emergency_velocity, "cntrl")

        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_NO_HANDLE);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_WESTINGHOUSE);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_FV4A);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_M394);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_M254);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_FVE408);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_FVEL6);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_D2);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_KNORR);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_FD1);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_BS2);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_TESTH);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_ST113);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_MHZ_P);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_MHZ_T);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_MHZ_EN57);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_MHZ_K5P);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_MHZ_K8P);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_TYPE_MHZ_6P);

        BIND_ENUM_CONSTANT(LOCAL_BRAKE_TYPE_NONE);
        BIND_ENUM_CONSTANT(LOCAL_BRAKE_TYPE_MANUAL);
        BIND_ENUM_CONSTANT(LOCAL_BRAKE_TYPE_PNEUMATIC);
        BIND_ENUM_CONSTANT(LOCAL_BRAKE_TYPE_HYDRAULIC);

        BIND_ENUM_CONSTANT(ANTI_SKID_BRAKE_NONE);
        BIND_ENUM_CONSTANT(ANTI_SKID_BRAKE_MANUAL);
        BIND_ENUM_CONSTANT(ANTI_SKID_BRAKE_AUTOMATIC);
        BIND_ENUM_CONSTANT(ANTI_SKID_BRAKE_YES);

        BIND_ENUM_CONSTANT(BRAKE_HANDLE_MOVEMENT_STEPPED);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_MOVEMENT_CONTINUOUS);

        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_NONE);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_PASSIVE);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_SWITCH);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_REVERSAL);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_AUTOMATIC);

        BIND_ENUM_CONSTANT(BRAKE_DELAY_NONE);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_G);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_P);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_R);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_GP);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_PR);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_GPR);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_PR_MG);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_GPR_MG);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_MIN);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_MAX);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_FILLING);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_DRIVE);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_CUTOFF);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_FIRST_STEP);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_FULL);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_EMERGENCY);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_EP_RELEASE);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_EP_HOLD);
        BIND_ENUM_CONSTANT(HANDLE_POSITION_EP_BRAKE);

        BIND_ENUM_CONSTANT(BRAKE_OP_MODE_NONE);
        BIND_ENUM_CONSTANT(BRAKE_OP_MODE_PN);
        BIND_ENUM_CONSTANT(BRAKE_OP_MODE_PNEP);
        BIND_ENUM_CONSTANT(BRAKE_OP_MODE_PNEPMED);

        BIND_ENUM_CONSTANT(BRAKE_SYSTEM_INDIVIDUAL);
        BIND_ENUM_CONSTANT(BRAKE_SYSTEM_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_SYSTEM_ELECTRO_PNEUMATIC);

        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_MAIN);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_CONVERTER_MANUAL);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_CONVERTER);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_ENGINE);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_COUPLER1);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_COUPLER2);


        BIND_ENUM_CONSTANT(BRAKE_VALVE_NO_VALVE);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_W);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_W_LU_VI);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_W_LU_L);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_W_LU_XR);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_K);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KG);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KP);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KSS);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KKG);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KKP);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KKS);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_HIKG1);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_HIKSS);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_HIKP1);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_KE);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_SW);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_ESTED);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_NEST3);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_EST3);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_LST);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_EST4);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_EST3AL2);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_EP1);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_EP2);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_M483);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_CV1_L_TR);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_CV1);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_CV1_R);
        BIND_ENUM_CONSTANT(BRAKE_VALVE_OTHER);

        BIND_ENUM_CONSTANT(BRAKE_METHOD_P10_BGU);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_P10_BG);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_D1);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_D2);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_FR513);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_COSID);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_P10Y_BG);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_P10Y_BGU);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_FR510);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_D1MG);
        BIND_ENUM_CONSTANT(BRAKE_METHOD_NONE);

        ClassDB::bind_method(D_METHOD("brake_releaser", "enabled"), &RailVehicleBrake::brake_releaser);
        ClassDB::bind_method(D_METHOD("consist_releaser", "active"), &RailVehicleBrake::consist_releaser);
        ClassDB::bind_method(D_METHOD("compressor", "enabled"), &RailVehicleBrake::compressor);
        ClassDB::bind_method(D_METHOD("brake_level_set", "level"), &RailVehicleBrake::brake_level_set);
        ClassDB::bind_method(
                D_METHOD("brake_level_set_position", "position"), &RailVehicleBrake::brake_level_set_position);
        ClassDB::bind_method(D_METHOD("brake_level_increase"), &RailVehicleBrake::brake_level_increase);
        ClassDB::bind_method(D_METHOD("brake_level_decrease"), &RailVehicleBrake::brake_level_decrease);
        ClassDB::bind_method(D_METHOD("local_brake_set", "level"), &RailVehicleBrake::local_brake_set);
        ClassDB::bind_method(D_METHOD("local_brake_increase"), &RailVehicleBrake::local_brake_increase);
        ClassDB::bind_method(D_METHOD("local_brake_decrease"), &RailVehicleBrake::local_brake_decrease);
        ClassDB::bind_method(D_METHOD("manual_brake_increase"), &RailVehicleBrake::manual_brake_increase);
        ClassDB::bind_method(D_METHOD("manual_brake_decrease"), &RailVehicleBrake::manual_brake_decrease);
        ClassDB::bind_method(D_METHOD("auto_rewident", "brake_delay"), &RailVehicleBrake::auto_rewident);
        ClassDB::bind_method(
                D_METHOD("brake_operation_mode_increase"), &RailVehicleBrake::brake_operation_mode_increase);
        ClassDB::bind_method(
                D_METHOD("brake_operation_mode_decrease"), &RailVehicleBrake::brake_operation_mode_decrease);
        ClassDB::bind_method(D_METHOD("ep_brake", "applied"), &RailVehicleBrake::ep_brake);
        ClassDB::bind_method(D_METHOD("get_operation_mode"), &RailVehicleBrake::get_operation_mode);
        ClassDB::bind_method(D_METHOD("brake_level_charging", "active"), &RailVehicleBrake::brake_level_charging);
        ClassDB::bind_method(D_METHOD("alarm_chain", "pulled"), &RailVehicleBrake::alarm_chain);
        ClassDB::bind_method(
                D_METHOD("universal_brake_button", "button", "pressed"), &RailVehicleBrake::universal_brake_button);

        ClassDB::bind_method(D_METHOD("get_alarm_chain_pulled"), &RailVehicleBrake::get_alarm_chain_pulled);
        ClassDB::bind_method(D_METHOD("get_air_pressure"), &RailVehicleBrake::get_air_pressure);
        ClassDB::bind_method(D_METHOD("get_loco_pressure"), &RailVehicleBrake::get_loco_pressure);
        ClassDB::bind_method(D_METHOD("get_pipe_brake_pressure"), &RailVehicleBrake::get_pipe_brake_pressure);
        ClassDB::bind_method(D_METHOD("get_pipe_pressure"), &RailVehicleBrake::get_pipe_pressure);
        ClassDB::bind_method(D_METHOD("get_feed_pipe_pressure"), &RailVehicleBrake::get_feed_pipe_pressure);
        ClassDB::bind_method(D_METHOD("get_tank_volume"), &RailVehicleBrake::get_tank_volume);
        ClassDB::bind_method(D_METHOD("get_compressor_pressure"), &RailVehicleBrake::get_compressor_pressure);
        ClassDB::bind_method(D_METHOD("get_compressor_enabled"), &RailVehicleBrake::get_compressor_enabled);
        ClassDB::bind_method(D_METHOD("get_compressor_allowed"), &RailVehicleBrake::get_compressor_allowed);
        ClassDB::bind_method(D_METHOD("get_controller_position"), &RailVehicleBrake::get_controller_position);
        ClassDB::bind_method(
                D_METHOD("get_controller_position_normalized"), &RailVehicleBrake::get_controller_position_normalized);
        ClassDB::bind_method(
                D_METHOD("get_local_position_normalized"), &RailVehicleBrake::get_local_position_normalized);
        ClassDB::bind_method(D_METHOD("get_manual_position"), &RailVehicleBrake::get_manual_position);
        ClassDB::bind_method(D_METHOD("get_unit_force"), &RailVehicleBrake::get_unit_force);
        ClassDB::bind_method(D_METHOD("get_force_ratio"), &RailVehicleBrake::get_force_ratio);
        ClassDB::bind_method(D_METHOD("get_emergency_valve_flow"), &RailVehicleBrake::get_emergency_valve_flow);
        ClassDB::bind_method(D_METHOD("get_main_valve_flow"), &RailVehicleBrake::get_main_valve_flow);
        ClassDB::bind_method(D_METHOD("get_local_valve_flow"), &RailVehicleBrake::get_local_valve_flow);
        ClassDB::bind_method(D_METHOD("get_handle_braking_flow"), &RailVehicleBrake::get_handle_braking_flow);
        ClassDB::bind_method(D_METHOD("get_handle_release_flow"), &RailVehicleBrake::get_handle_release_flow);
        ClassDB::bind_method(D_METHOD("get_handle_emergency_flow"), &RailVehicleBrake::get_handle_emergency_flow);
        ClassDB::bind_method(
                D_METHOD("get_handle_control_chamber_flow"), &RailVehicleBrake::get_handle_control_chamber_flow);
        ClassDB::bind_method(
                D_METHOD("get_handle_timing_reservoir_flow"), &RailVehicleBrake::get_handle_timing_reservoir_flow);
        ClassDB::bind_method(D_METHOD("get_control_pressure"), &RailVehicleBrake::get_control_pressure);
        ClassDB::bind_method(D_METHOD("get_handle_control_pressure"), &RailVehicleBrake::get_handle_control_pressure);
        ClassDB::bind_method(D_METHOD("get_local_aeim_position"), &RailVehicleBrake::get_local_aeim_position);
        ClassDB::bind_method(D_METHOD("get_edb_cylinder_pressure"), &RailVehicleBrake::get_edb_cylinder_pressure);
        ClassDB::bind_method(D_METHOD("get_releaser_active"), &RailVehicleBrake::get_releaser_active);
        ClassDB::bind_method(D_METHOD("get_main_pipe_locked"), &RailVehicleBrake::get_main_pipe_locked);
        ClassDB::bind_method(D_METHOD("get_force"), &RailVehicleBrake::get_force);
        ClassDB::bind_method(D_METHOD("get_delay_setting"), &RailVehicleBrake::get_delay_setting);
        ClassDB::bind_method(
                D_METHOD("get_control_reservoir_pressure"), &RailVehicleBrake::get_control_reservoir_pressure);
        ClassDB::bind_method(D_METHOD("get_handle_position", "position"), &RailVehicleBrake::get_handle_position);
        ClassDB::bind_method(D_METHOD("get_handle_time_controlled"), &RailVehicleBrake::get_handle_time_controlled);
        ClassDB::bind_method(
                D_METHOD("get_handle_ep_time_controlled"), &RailVehicleBrake::get_handle_ep_time_controlled);
        ClassDB::bind_method(D_METHOD("get_force_at", "ratio", "velocity"), &RailVehicleBrake::get_force_at);
        ClassDB::bind_method(D_METHOD("is_braking"), &RailVehicleBrake::is_braking);
        ClassDB::bind_method(D_METHOD("is_holding"), &RailVehicleBrake::is_holding);
        ClassDB::bind_method(D_METHOD("is_cut_off"), &RailVehicleBrake::is_cut_off);

        ADD_SIGNAL(MethodInfo(accelerator_activated_signal));
    }

    void RailVehicleBrake::_register_commands() {
        register_command("brake_releaser", Callable(this, "brake_releaser"));
        register_command("consist_releaser", Callable(this, "consist_releaser"));
        register_command("compressor", Callable(this, "compressor"));
        register_command("brake_level_set", Callable(this, "brake_level_set"));
        register_command("brake_level_set_position", Callable(this, "brake_level_set_position"));
        register_command("brake_level_increase", Callable(this, "brake_level_increase"));
        register_command("brake_level_decrease", Callable(this, "brake_level_decrease"));
        register_command("local_brake_set", Callable(this, "local_brake_set"));
        register_command("local_brake_increase", Callable(this, "local_brake_increase"));
        register_command("local_brake_decrease", Callable(this, "local_brake_decrease"));
        register_command("manual_brake_increase", Callable(this, "manual_brake_increase"));
        register_command("manual_brake_decrease", Callable(this, "manual_brake_decrease"));
        register_command("auto_rewident", Callable(this, "auto_rewident"));
        register_command("brake_operation_mode_increase", Callable(this, "brake_operation_mode_increase"));
        register_command("brake_operation_mode_decrease", Callable(this, "brake_operation_mode_decrease"));
        register_command("ep_brake", Callable(this, "ep_brake"));
        register_command("brake_level_charging", Callable(this, "brake_level_charging"));
        register_command("alarm_chain", Callable(this, "alarm_chain"));
        register_command("universal_brake_button", Callable(this, "universal_brake_button"));
    }

    void RailVehicleBrake::_unregister_commands() {
        unregister_command("brake_releaser");
        unregister_command("consist_releaser");
        unregister_command("compressor");
        unregister_command("brake_level_set");
        unregister_command("brake_level_set_position");
        unregister_command("brake_level_increase");
        unregister_command("brake_level_decrease");
        unregister_command("local_brake_set");
        unregister_command("local_brake_increase");
        unregister_command("local_brake_decrease");
        unregister_command("manual_brake_increase");
        unregister_command("manual_brake_decrease");
        unregister_command("auto_rewident");
        unregister_command("brake_operation_mode_increase");
        unregister_command("brake_operation_mode_decrease");
        unregister_command("ep_brake");
        unregister_command("brake_level_charging");
        unregister_command("alarm_chain");
        unregister_command("universal_brake_button");
    }
} // namespace godot
