#include "../brakes/TrainBrake.hpp"
#include "../core/TrainController.hpp"
#include "../core/utils.hpp"
#include <algorithm>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void TrainBrake::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                Variant::INT, "valve_type", "valve/type", &TrainBrake::set_valve_type, &TrainBrake::get_valve_type,
                "valve_type", PROPERTY_HINT_ENUM,
                "NoValve,W,W_Lu_VI,W_Lu_L,W_Lu_XR,K,Kg,Kp,Kss,Kkg,Kkp,Kks,Hikg1,Hikss,Hikp1,KE,SW,EStED,NESt3,ESt3,LSt,"
                "ESt4,ESt3AL2,EP1,EP2,M483,CV1_L_TR,CV1,CV1_R,Other")
        BIND_PROPERTY(
                Variant::INT, "friction_elements_per_axle", "friction_elements_per_axle",
                &TrainBrake::set_friction_elements_per_axle, &TrainBrake::get_friction_elements_per_axle,
                "friction_elements_per_axle");
        BIND_PROPERTY(
                Variant::FLOAT, "max_brake_force", "brake_force/max", &TrainBrake::set_max_brake_force,
                &TrainBrake::get_max_brake_force, "max_brake_force");
        BIND_PROPERTY(
                Variant::INT, "est_valve_size", "est_valve/size", &TrainBrake::set_valve_size,
                &TrainBrake::get_valve_size, "valve_size");
        BIND_PROPERTY(
                Variant::FLOAT, "traction_brake_force", "brake_force/traction", &TrainBrake::set_traction_brake_force,
                &TrainBrake::get_traction_brake_force, "traction_brake_force");
        BIND_PROPERTY(
                Variant::FLOAT, "max_cylinder_pressure", "max_cylinder_pressure",
                &TrainBrake::set_max_cylinder_pressure, &TrainBrake::get_max_cylinder_pressure,
                "max_cylinder_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "max_aux_pressure", "max_aux_pressure", &TrainBrake::set_max_aux_pressure,
                &TrainBrake::get_max_aux_pressure, "max_aux_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "max_tare_pressure", "max_tare_pressure", &TrainBrake::set_max_tare_pressure,
                &TrainBrake::get_max_tare_pressure, "max_tare_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "max_medium_pressure", "max_medium_pressure", &TrainBrake::set_max_medium_pressure,
                &TrainBrake::get_max_medium_pressure, "max_medium_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "max_antislip_pressure", "max_antislip_pressure",
                &TrainBrake::set_max_antislip_pressure, &TrainBrake::get_max_antislip_pressure,
                "max_antislip_pressure");
        BIND_PROPERTY(
                Variant::INT, "cylinder_count", "cylinder/count", &TrainBrake::set_cylinder_count,
                &TrainBrake::get_cylinder_count, "cylinder_count");
        BIND_PROPERTY(
                Variant::FLOAT, "cylinder_radius", "cylinder/radius", &TrainBrake::set_cylinder_radius,
                &TrainBrake::get_cylinder_radius, "cylinder_radius");
        BIND_PROPERTY(
                Variant::FLOAT, "cylinder_distance", "cylinder/distance", &TrainBrake::set_cylinder_distance,
                &TrainBrake::get_cylinder_distance, "cylinder_distance");
        BIND_PROPERTY(
                Variant::FLOAT, "cylinder_spring_force", "cylinder/spring_force",
                &TrainBrake::set_cylinder_spring_force, &TrainBrake::get_cylinder_spring_force,
                "cylinder_spring_force");
        BIND_PROPERTY(
                Variant::FLOAT, "piston_stroke_adjuster_resistance", "piston_stroke/adjuster_resistance",
                &TrainBrake::set_piston_stroke_adjuster_resistance, &TrainBrake::get_piston_stroke_adjuster_resistance,
                "");
        BIND_PROPERTY(
                Variant::FLOAT, "cylinder_gear_ratio", "cylinder/gear_ratio", &TrainBrake::set_cylinder_gear_ratio,
                &TrainBrake::get_cylinder_gear_ratio, "cylinder_gear_ratio");
        BIND_PROPERTY(
                Variant::FLOAT, "cylinder_gear_ratio_low", "cylinder/gear_ratio_low",
                &TrainBrake::set_cylinder_gear_ratio_low, &TrainBrake::get_cylinder_gear_ratio_low,
                "cylinder_gear_ratio_low");
        BIND_PROPERTY(
                Variant::FLOAT, "cylinder_gear_ratio_high", "cylinder/gear_ratio_high",
                &TrainBrake::set_cylinder_gear_ratio_high, &TrainBrake::get_cylinder_gear_ratio_high,
                "cylinder_gear_ratio_high");
        BIND_PROPERTY(
                Variant::FLOAT, "pipe_pressure_min", "pipe/pressure_min", &TrainBrake::set_pipe_pressure_min,
                &TrainBrake::get_pipe_pressure_min, "pipe_pressure_min");
        BIND_PROPERTY(
                Variant::FLOAT, "pipe_pressure_max", "pipe/pressure_max", &TrainBrake::set_pipe_pressure_max,
                &TrainBrake::get_pipe_pressure_max, "pipe_pressure_max");
        BIND_PROPERTY(
                Variant::FLOAT, "main_tank_volume", "tank/volume_main", &TrainBrake::set_main_tank_volume,
                &TrainBrake::get_main_tank_volume, "main_tank_volume");
        BIND_PROPERTY(
                Variant::FLOAT, "aux_tank_volume", "tank/volume_aux", &TrainBrake::set_aux_tank_volume,
                &TrainBrake::get_aux_tank_volume, "aux_tank_volume");
        BIND_PROPERTY(
                Variant::FLOAT, "compressor_pressure_cab_a_min", "compressor/cab_a/min_pressure",
                &TrainBrake::set_compressor_pressure_cab_a_min, &TrainBrake::get_compressor_pressure_cab_a_min,
                "compressor_pressure_min");
        BIND_PROPERTY(
                Variant::FLOAT, "compressor_pressure_cab_a_max", "compressor/cab_a/max_pressure",
                &TrainBrake::set_compressor_pressure_cab_a_max, &TrainBrake::get_compressor_pressure_cab_a_max,
                "compressor_pressure_max");
        BIND_PROPERTY(
                Variant::FLOAT, "compressor_pressure_cab_b_min", "compressor/cab_b/min_pressure",
                &TrainBrake::set_compressor_pressure_cab_b_min, &TrainBrake::get_compressor_pressure_cab_b_min,
                "compressor_pressure_min");
        BIND_PROPERTY(
                Variant::FLOAT, "compressor_pressure_cab_b_max", "compressor/cab_b/max_pressure",
                &TrainBrake::set_compressor_pressure_cab_b_max, &TrainBrake::get_compressor_pressure_cab_b_max,
                "compressor_pressure_max");
        BIND_PROPERTY(
                Variant::FLOAT, "compressor_speed", "compressor/speed", &TrainBrake::set_compressor_speed,
                &TrainBrake::get_compressor_speed, "compressor_speed");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "compressor_power", "compressor/power", &TrainBrake::set_compressor_power,
                &TrainBrake::get_compressor_power, "compressor_power", PROPERTY_HINT_ENUM,
                "Main,Unused,Converter,Engine,Coupler1,Coupler2");
        BIND_PROPERTY(
                Variant::FLOAT, "rig_effectiveness", "rig_effectiveness", &TrainBrake::set_rig_effectiveness,
                &TrainBrake::get_rig_effectiveness, "rig_effectiveness");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "brake_method", "brake/method", &TrainBrake::set_brake_method,
                &TrainBrake::get_brake_method, "brake_method", PROPERTY_HINT_ENUM,
                "P10-Bg,P10-Bgu,FR513,FR510,Cosid,P10yBg,P10yBgu,Disk1,Disk1+Mg,Disk2");
        BIND_PROPERTY(
                Variant::FLOAT, "rapid_transfer", "rapid/transfer", &TrainBrake::set_rapid_transfer,
                &TrainBrake::get_rapid_transfer, "rapid_transfer");
        BIND_PROPERTY(
                Variant::FLOAT, "rapid_switching_speed", "rapid/switching_speed",
                &TrainBrake::set_rapid_switching_speed, &TrainBrake::get_rapid_switching_speed,
                "rapid_switching_speed");
        BIND_PROPERTY(
                Variant::FLOAT, "air_leak_multiplier", "air_leak_multiplier", &TrainBrake::set_air_leak_multiplier,
                &TrainBrake::get_air_leak_multiplier, "air_leak_multiplier")
        BIND_PROPERTY(
                Variant::BOOL, "compressor_tank_valve_active", "compressor/tank_valve_active",
                &TrainBrake::set_compressor_tank_valve_active, &TrainBrake::get_compressor_tank_valve_active,
                "compressor_tank_valve_active")
        BIND_PROPERTY(
                Variant::FLOAT, "lower_emergency_closing_pressure", "compressor/lower_emergency_closing_pressure",
                &TrainBrake::set_lower_emergency_closing_pressure, &TrainBrake::get_lower_emergency_closing_pressure,
                "lower_emergency_closing_pressure")
        BIND_PROPERTY(
                Variant::FLOAT, "higher_emergency_closing_pressure", "compressor/higher_emergency_closing_pressure",
                &TrainBrake::set_higher_emergency_closing_pressure, &TrainBrake::get_higher_emergency_closing_pressure,
                "higher_emergency_closing_pressure")
        BIND_PROPERTY(
                Variant::FLOAT, "main_pipe_blocking_pressure", "main_pipe/blocking_pressure",
                &TrainBrake::set_main_pipe_blocking_pressure, &TrainBrake::get_main_pipe_blocking_pressure,
                "main_pipe_blocking_pressure")
        BIND_PROPERTY(
                Variant::FLOAT, "main_pipe_unblocking_pressure", "main_pipe/unblocking_pressure",
                &TrainBrake::set_main_pipe_unblocking_pressure, &TrainBrake::get_main_pipe_unblocking_pressure,
                "main_pipe_unblocking_pressure")
        BIND_PROPERTY(
                Variant::FLOAT, "main_pipe_minimum_unblocking_handle_position",
                "main_pipe/minimum_unblocking_handle_position",
                &TrainBrake::set_main_pipe_minimum_unblocking_handle_position,
                &TrainBrake::get_main_pipe_minimum_unblocking_handle_position,
                "main_pipe_minimum_unblocking_handle_position")
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "brake_pressure_table", "brake_pressure_table", &TrainBrake::set_brake_pressure_table,
                &TrainBrake::get_brake_pressure_table, "brake_pressure_table", PROPERTY_HINT_TYPE_STRING,
                "BrakePressureTableItem");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "compressor_list", "compressor_list", &TrainBrake::set_compressor_list,
                &TrainBrake::get_compressor_list, "compressor_list", PROPERTY_HINT_TYPE_STRING, "CompressorListItem");
        BIND_PROPERTY(
                Variant::FLOAT, "emergency_valve_area", "compressor/emergency_valve_area",
                &TrainBrake::set_emergency_valve_area, &TrainBrake::get_emergency_valve_area, "emergency_valve_area")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "universal_brake_button_1", "universal_brake_button/1",
                &TrainBrake::set_universal_brake_button_1, &TrainBrake::get_universal_brake_button_1,
                "universal_brake_button_1", PROPERTY_HINT_FLAGS,
                "Releaser,Bridge Emergency Valve,High Pressure Impulse,Assimilation,Anti-Skid Brake")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "universal_brake_button_2", "universal_brake_button/2",
                &TrainBrake::set_universal_brake_button_2, &TrainBrake::get_universal_brake_button_2,
                "universal_brake_button_2", PROPERTY_HINT_FLAGS,
                "Releaser,Bridge Emergency Valve,High Pressure Impulse,Assimilation,Anti-Skid Brake")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "universal_brake_button_3", "universal_brake_button/3",
                &TrainBrake::set_universal_brake_button_3, &TrainBrake::get_universal_brake_button_3,
                "universal_brake_button_3", PROPERTY_HINT_FLAGS,
                "Releaser,Bridge Emergency Valve,High Pressure Impulse,Assimilation,Anti-Skid Brake")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "brake_system", "cntrl/brake_system", &TrainBrake::set_brake_system,
                &TrainBrake::get_brake_system, "brake_system", PROPERTY_HINT_ENUM,
                "Individual,Pneumatic,ElectroPneumatic")
        BIND_PROPERTY(
                Variant::INT, "brake_ctrl_position_count", "cntrl/brake_ctrl_position_count",
                &TrainBrake::set_brake_ctrl_position_count, &TrainBrake::get_brake_ctrl_position_count,
                "brake_ctrl_position_count")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "brake_delays", "cntrl/brake_delays", &TrainBrake::set_brake_delays,
                &TrainBrake::get_brake_delays, "brake_delays", PROPERTY_HINT_ENUM, "G:1,P:2,R:4,GP:3,PR:6,GPR:7,PR+Mg:14,GPR+Mg:15")
        BIND_PROPERTY(
                Variant::FLOAT, "brake_delay_1", "cntrl/brake_delay_1", &TrainBrake::set_brake_delay_1,
                &TrainBrake::get_brake_delay_1, "brake_delay_1")
        BIND_PROPERTY(
                Variant::FLOAT, "brake_delay_2", "cntrl/brake_delay_2", &TrainBrake::set_brake_delay_2,
                &TrainBrake::get_brake_delay_2, "brake_delay_2")
        BIND_PROPERTY(
                Variant::FLOAT, "brake_delay_3", "cntrl/brake_delay_3", &TrainBrake::set_brake_delay_3,
                &TrainBrake::get_brake_delay_3, "brake_delay_3")
        BIND_PROPERTY(
                Variant::FLOAT, "brake_delay_4", "cntrl/brake_delay_4", &TrainBrake::set_brake_delay_4,
                &TrainBrake::get_brake_delay_4, "brake_delay_4")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "brake_op_modes", "cntrl/brake_op_modes", &TrainBrake::set_brake_op_modes,
                &TrainBrake::get_brake_op_modes, "brake_op_modes", PROPERTY_HINT_ENUM, "PN:3,PNEPMED:15")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "brake_handle_type", "cntrl/brake_handle_type", &TrainBrake::set_brake_handle_type,
                &TrainBrake::get_brake_handle_type, "brake_handle_type", PROPERTY_HINT_ENUM,
                "NoHandle,Westinghouse,FV4a,M394,M254,FVE408,FVel6,D2,Knorr,FD1,BS2,testH,St113,MHZ_P,MHZ_T,MHZ_EN57,"
                "MHZ_K5P,MHZ_K8P,MHZ_6P")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "anti_skid_brake_type", "cntrl/anti_skid_brake_type", &TrainBrake::set_anti_skid_brake_type,
                &TrainBrake::get_anti_skid_brake_type, "anti_skid_brake_type", PROPERTY_HINT_ENUM,
                "None,Manual,Automatic")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "local_brake_type", "cntrl/local_brake_type", &TrainBrake::set_local_brake_type,
                &TrainBrake::get_local_brake_type, "local_brake_type", PROPERTY_HINT_ENUM,
                "None,Manual,Pneumatic,Hydraulic")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "local_brake_handle_type", "cntrl/local_brake_handle_type",
                &TrainBrake::set_local_brake_handle_type, &TrainBrake::get_local_brake_handle_type,
                "local_brake_handle_type", PROPERTY_HINT_ENUM,
                "NoHandle,Westinghouse,FV4a,M394,M254,FVE408,FVel6,D2,Knorr,FD1,BS2,testH,St113,MHZ_P,MHZ_T,MHZ_EN57,"
                "MHZ_K5P,MHZ_K8P,MHZ_6P")
        BIND_PROPERTY(
                Variant::BOOL, "manual_brake_present", "cntrl/manual_brake_present", &TrainBrake::set_manual_brake_present,
                &TrainBrake::get_manual_brake_present, "manual_brake_present")
        BIND_PROPERTY_W_HINT(
                Variant::INT, "dynamic_brake_type", "cntrl/dynamic_brake_type", &TrainBrake::set_dynamic_brake_type,
                &TrainBrake::get_dynamic_brake_type, "dynamic_brake_type", PROPERTY_HINT_ENUM,
                "None:0,Passive:1,Switch:2,Reversal:4,Automatic:8")
        BIND_PROPERTY(
                Variant::BOOL, "local_brake_traxx", "cntrl/local_brake_traxx", &TrainBrake::set_local_brake_traxx,
                &TrainBrake::get_local_brake_traxx, "local_brake_traxx")
        BIND_PROPERTY(
                Variant::BOOL, "release_parking_by_spring_brake", "cntrl/release_parking_by_spring_brake",
                &TrainBrake::set_release_parking_by_spring_brake, &TrainBrake::get_release_parking_by_spring_brake,
                "release_parking_by_spring_brake")
        BIND_PROPERTY(
                Variant::BOOL, "release_parking_by_spring_brake_when_door_open",
                "cntrl/release_parking_by_spring_brake_when_door_open",
                &TrainBrake::set_release_parking_by_spring_brake_when_door_open,
                &TrainBrake::get_release_parking_by_spring_brake_when_door_open,
                "release_parking_by_spring_brake_when_door_open")
        BIND_PROPERTY(
                Variant::BOOL, "spring_brake_cuts_off_drive", "cntrl/spring_brake_cuts_off_drive",
                &TrainBrake::set_spring_brake_cuts_off_drive, &TrainBrake::get_spring_brake_cuts_off_drive,
                "spring_brake_cuts_off_drive")
        BIND_PROPERTY(
                Variant::FLOAT, "spring_brake_drive_emergency_velocity", "cntrl/spring_brake_drive_emergency_velocity",
                &TrainBrake::set_spring_brake_drive_emergency_velocity,
                &TrainBrake::get_spring_brake_drive_emergency_velocity, "spring_brake_drive_emergency_velocity")

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

        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_NONE);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_PASSIVE);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_SWITCH);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_REVERSAL);
        BIND_ENUM_CONSTANT(DYNAMIC_BRAKE_AUTOMATIC);

        BIND_ENUM_CONSTANT(BRAKE_DELAY_G);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_P);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_R);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_GP);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_PR);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_GPR);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_PR_MG);
        BIND_ENUM_CONSTANT(BRAKE_DELAY_GPR_MG);

        BIND_ENUM_CONSTANT(BRAKE_OP_MODE_PN);
        BIND_ENUM_CONSTANT(BRAKE_OP_MODE_PNEPMED);

        BIND_ENUM_CONSTANT(BRAKE_SYSTEM_INDIVIDUAL);
        BIND_ENUM_CONSTANT(BRAKE_SYSTEM_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_SYSTEM_ELECTRO_PNEUMATIC);

        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_MAIN);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_UNUSED);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_CONVERTER);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_ENGINE);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_COUPLER1);
        BIND_ENUM_CONSTANT(COMPRESSOR_POWER_COUPLER2);

        BIND_ENUM_CONSTANT(BRAKE_HANDLE_POSITION_MIN);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_POSITION_MAX);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_POSITION_DRIVE);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_POSITION_FULL);
        BIND_ENUM_CONSTANT(BRAKE_HANDLE_POSITION_EMERGENCY);

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
        BIND_ENUM_CONSTANT(BRAKE_METHOD_D1MG);

        ClassDB::bind_method(D_METHOD("brake_releaser", "enabled"), &TrainBrake::brake_releaser);
        ClassDB::bind_method(D_METHOD("brake_level_set", "level"), &TrainBrake::brake_level_set);
        ClassDB::bind_method(D_METHOD("brake_level_set_position", "position"), &TrainBrake::brake_level_set_position);
        ClassDB::bind_method(
                D_METHOD("brake_level_set_position_str", "position"), &TrainBrake::brake_level_set_position_str);
        ClassDB::bind_method(D_METHOD("brake_level_increase"), &TrainBrake::brake_level_increase);
        ClassDB::bind_method(D_METHOD("brake_level_decrease"), &TrainBrake::brake_level_decrease);
    }

    void TrainBrake::_register_commands() {
        register_command("brake_releaser", Callable(this, "brake_releaser"));
        register_command("brake_level_set", Callable(this, "brake_level_set"));
        register_command("brake_level_set_position", Callable(this, "brake_level_set_position_str"));
        register_command("brake_level_increase", Callable(this, "brake_level_increase"));
        register_command("brake_level_decrease", Callable(this, "brake_level_decrease"));
    }

    void TrainBrake::_unregister_commands() {
        unregister_command("brake_releaser", Callable(this, "brake_releaser"));
        unregister_command("brake_level_set", Callable(this, "brake_level_set"));
        unregister_command("brake_level_set_position", Callable(this, "brake_level_set_position_str"));
        unregister_command("brake_level_increase", Callable(this, "brake_level_increase"));
        unregister_command("brake_level_decrease", Callable(this, "brake_level_decrease"));
    }

    void TrainBrake::brake_releaser(const bool p_pressed) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->BrakeReleaser(p_pressed ? 1 : 0);
    }

    void TrainBrake::brake_level_set(const double p_level) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        const double level = CLAMP(p_level, 0.0, 1.0);
        const double brake_controller_min = mover->Handle->GetPos(bh_MIN);
        const double brake_controller_max = mover->Handle->GetPos(bh_MAX);
        const double brake_controller_pos =
                brake_controller_min + (level * (brake_controller_max - brake_controller_min));
        mover->BrakeLevelSet(brake_controller_pos);
    }

    void TrainBrake::brake_level_set_position(const BrakeHandlePosition p_position) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        if (const std::unordered_map<BrakeHandlePosition, int>::const_iterator it =
                    brake_handle_position_map.find(p_position);
            it != brake_handle_position_map.end()) {
            mover->BrakeLevelSet(mover->Handle->GetPos(it->second));
        } else {
            log_error("Unhandled brake level position: " + String::num(static_cast<int>(p_position)));
        }
    }

    void TrainBrake::brake_level_set_position_str(const String &p_position) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        const std::unordered_map<std::string, int>::const_iterator it =
                brake_handle_position_string_map.find(std::string(p_position.utf8()));
        if (it != brake_handle_position_string_map.end()) {
            mover->BrakeLevelSet(mover->Handle->GetPos(it->second));
        } else {
            log_error("Unhandled brake level position: " + p_position);
        }
    }

    void TrainBrake::brake_level_increase() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->IncBrakeLevel();
    }

    void TrainBrake::brake_level_decrease() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER_BRAKE(mover);
        mover->DecBrakeLevel();
    }

    void TrainBrake::_do_fetch_config_from_mover(TMoverParameters *p_mover, Dictionary &p_config) {
        if (p_mover->Handle == nullptr) {
            return;
        }
        p_config["brakes_controller_position_min"] = p_mover->Handle->GetPos(bh_MIN);
        p_config["brakes_controller_position_max"] = p_mover->Handle->GetPos(bh_MAX);
    }

    void TrainBrake::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        const double brake_controller_pos = p_mover->fBrakeCtrlPos;
        const double brake_controller_min = p_mover->Handle->GetPos(bh_MIN);
        const double brake_controller_max = p_mover->Handle->GetPos(bh_MAX);
        double brake_controller_pos_normalized = 0.0;
        if (brake_controller_max != brake_controller_min) {
            brake_controller_pos_normalized =
                    (brake_controller_pos - brake_controller_min) / (brake_controller_max - brake_controller_min);
        }
        p_state["brake_air_pressure"] = p_mover->BrakePress;
        p_state["brake_loco_pressure"] = p_mover->LocBrakePress;
        p_state["brake_pipe_pressure"] = p_mover->PipeBrakePress;
        p_state["pipe_pressure"] = p_mover->PipePress;
        p_state["brake_tank_volume"] = p_mover->Volume;
        p_state["brake_controller_position"] = brake_controller_pos;
        p_state["brake_controller_position_normalized"] = brake_controller_pos_normalized;
    }

    void TrainBrake::_do_update_internal_mover(TMoverParameters *p_mover) {
        /* logika z Mover::LoadFiz_Brake */
        p_mover->BrakeSystem = brake_system_type_map.at(brake_system);        // BrakeSystem
        p_mover->BrakeCtrlPosNo = brake_ctrl_position_count;                  // BCPN
        p_mover->BrakeDelay[0] = brake_delay_1;                               // BDelay1
        p_mover->BrakeDelay[1] = brake_delay_2;                               // BDelay2
        p_mover->BrakeDelay[2] = brake_delay_3;                               // BDelay3
        p_mover->BrakeDelay[3] = brake_delay_4;                               // BDelay4
        p_mover->BrakeDelays = brake_delays;                                  // BrakeDelays
        p_mover->BrakeOpModes = brake_op_modes;                               // BrakeOpModes
        p_mover->BrakeHandle = brake_handle_type_map.at(brake_handle_type);   // BrakeHandle
        p_mover->BrakeLocHandle = brake_handle_type_map.at(local_brake_handle_type); // LocBrakeHandle
        p_mover->ASBType = anti_skid_brake_type;                             // ASB
        p_mover->LocalBrake = local_brake_type_map.at(local_brake_type);     // LocalBrake
        p_mover->MBrake = manual_brake_present;                              // ManualBrake
        p_mover->LocHandleTimeTraxx = local_brake_traxx;                     // LocalBrakeTraxx
        p_mover->DynamicBrakeType = dynamic_brake_type;                      // DynamicBrake
        p_mover->ReleaseParkingBySpringBrake = release_parking_by_spring_brake;
        p_mover->ReleaseParkingBySpringBrakeWhenDoorIsOpen = release_parking_by_spring_brake_when_door_open;
        p_mover->SpringBrakeCutsOffDrive = spring_brake_cuts_off_drive;
        p_mover->SpringBrakeDriveEmergencyVel = spring_brake_drive_emergency_velocity;

        /* FIXME: BrakeValve nie jest tylko enumem, jesli w FIZ wpisze sie nieznany symbol zawierający ESt, to EXE
         * ustawi BrakeValve=ESt3. Powinien to ogarnąć importer FIZ
         *
         * Whoever thought making BrakeValve half-enum, half-parser-voodoo was a good idea
         * condemned everyone else to cargo-cult their bugs. Thanks a lot, dear original MaSzyna code authors.
         */

        // assuming same int values between our TrainBrakeValve and mover's TBrakeValve
        p_mover->BrakeValve = static_cast<TBrakeValve>(static_cast<int>(valve_type));

        const std::unordered_map<TBrakeValve, TBrakeSubSystem>::const_iterator it =
                brake_valve_to_subsystem_map.find(p_mover->BrakeValve);
        p_mover->BrakeSubsystem = it != brake_valve_to_subsystem_map.end() ? it->second : TBrakeSubSystem::ss_None;

        p_mover->NBpA = CLAMP<int, int, int>(friction_elements_per_axle, 0, 4);
        p_mover->MaxBrakeForce = max_brake_force;
        p_mover->BrakeValveSize = valve_size;
        p_mover->TrackBrakeForce = traction_brake_force * 1000.0;
        p_mover->MaxBrakePress[3] = max_cylinder_pressure;
        if (max_cylinder_pressure > 0.0) {
            p_mover->BrakeCylNo = cylinder_count;

            if (cylinder_count > 0) {
                p_mover->MaxBrakePress[0] = max_aux_pressure < 0.01 ? max_cylinder_pressure : max_aux_pressure;
                p_mover->MaxBrakePress[1] = max_tare_pressure;
                p_mover->MaxBrakePress[2] = max_medium_pressure;
                p_mover->MaxBrakePress[4] = max_antislip_pressure < 0.01 ? 0.0 : max_antislip_pressure;

                p_mover->BrakeCylRadius = cylinder_radius;
                p_mover->BrakeCylDist = cylinder_distance;
                p_mover->BrakeCylSpring = cylinder_spring_force;
                p_mover->BrakeSlckAdj = piston_stroke_adjuster_resistance;
                p_mover->BrakeRigEff = rig_effectiveness;

                p_mover->BrakeCylMult[0] = cylinder_gear_ratio;
                p_mover->BrakeCylMult[1] = cylinder_gear_ratio_low;
                p_mover->BrakeCylMult[2] = cylinder_gear_ratio_high;

                p_mover->P2FTrans = 100 * M_PI * std::pow(cylinder_radius, 2);

                p_mover->LoadFlag = (cylinder_gear_ratio_low > 0.0 || max_tare_pressure > 0.0) ? 1 : 0;

                p_mover->BrakeVolume = M_PI * std::pow(cylinder_radius, 2) * cylinder_distance * cylinder_count;
                p_mover->BrakeVVolume = aux_tank_volume;

                const std::unordered_map<BrakeMethod, int>::const_iterator lookup;
                p_mover->BrakeMethod = lookup != brake_method_map.find(brake_method) ? brake_method : 0;
                p_mover->BrakeMethod = brake_method;
                p_mover->RapidMult = rapid_transfer;
                p_mover->RapidVel = rapid_switching_speed;
            }
        } else {
            p_mover->P2FTrans = 0;
        }

        p_mover->CntrlPipePress =
                5 + (0.001 * (UtilityFunctions::randf_range(0.0, 10.0) - UtilityFunctions::randf_range(0.0, 10.0)));
        /* PipePress i HighPipePress musza byc skopiowane */
        p_mover->HighPipePress = pipe_pressure_max;
        p_mover->LowPipePress = pipe_pressure_min;
        p_mover->VeselVolume = main_tank_volume;
        p_mover->MinCompressor = compressor_pressure_cab_a_min;
        p_mover->MaxCompressor = compressor_pressure_cab_a_max;
        p_mover->MinCompressor_cabB = compressor_pressure_cab_b_min;
        p_mover->MaxCompressor_cabB = compressor_pressure_cab_b_max;

        p_mover->CompressorTankValve = compressor_tank_valve_active;
        p_mover->EmergencyValveOff = lower_emergency_closing_pressure;
        p_mover->EmergencyValveOn = higher_emergency_closing_pressure;

        p_mover->EmergencyValveArea = emergency_valve_area;
        p_mover->UniversalBrakeButtonFlag[0] = universal_brake_button_1;
        p_mover->UniversalBrakeButtonFlag[1] = universal_brake_button_2;
        p_mover->UniversalBrakeButtonFlag[2] = universal_brake_button_3;

        p_mover->LockPipeOn = main_pipe_blocking_pressure;
        p_mover->LockPipeOff = main_pipe_unblocking_pressure;
        p_mover->HandleUnlock = main_pipe_minimum_unblocking_handle_position;
        p_mover->EmergencyCutsOffHandle = false; //@TODO: Figure out wtf is this

        p_mover->CompressorSpeed = compressor_speed;
        p_mover->CompressorPower = compressor_power;

        // According to the original code - the parameter is provided in the form of a multiplier, where 1.0 means the
        // default rate of 0.01
        p_mover->AirLeakRate = air_leak_multiplier * 0.01;

        // By default, this should be set to true if an engine type is diesel or diesel-electric and false, otherwise
        //  this action should be performed by FIZ parser
        p_mover->ReleaserEnabledOnlyAtNoPowerPos = releaser_enabled_only_at_no_power_pos;
        if (p_mover->MinCompressor_cabB > 0.0) {
            p_mover->MinCompressor_cabA = p_mover->MinCompressor;
            p_mover->CabDependentCompressor = true;
        } else {
            p_mover->MinCompressor_cabB = p_mover->MinCompressor;
        }
        if (p_mover->MaxCompressor_cabB > 0.0) {
            p_mover->MaxCompressor_cabA = p_mover->MaxCompressor;
            p_mover->CabDependentCompressor = true;
        } else {
            p_mover->MaxCompressor_cabB = p_mover->MaxCompressor;
        }

        /* BPT: tabelka hamulcowa, wyszczegolnienie cisnien w rurze wg pozycji krana */
        p_mover->BrakePressureTable.clear();
        for (int i = 0; i < brake_pressure_table.size(); i++) {
            const Ref<BrakePressureTableItem> &row = brake_pressure_table[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning(
                        "[TrainBrake]: brake_pressure_table property is null at index " + String::num(i));
                continue;
            }
            Maszyna::TBrakePressure entry;
            entry.PipePressureVal = row->get_pipe_pressure();
            entry.BrakePressureVal = row->get_brake_cylinder_pressure();
            entry.FlowSpeedVal = row->get_fill_speed();
            entry.BrakeType = brake_pressure_table_type_map.at(row->get_brake_type());
            p_mover->BrakePressureTable[row->get_handle_position()] = entry;
        }

        /* CompressorList: programator sprezarek */
        constexpr int MAX_COMPRESSOR_LIST = 8;
        const int compressor_list_size = static_cast<int>(compressor_list.size());
        if (compressor_list_size > MAX_COMPRESSOR_LIST) {
            UtilityFunctions::push_warning(
                    "[TrainBrake]: compressor_list has " + String::num(compressor_list_size) +
                    " entries, exceeding the mover's limit of " + String::num(MAX_COMPRESSOR_LIST) + "; truncating.");
        }
        for (int i = 0; i < std::min(MAX_COMPRESSOR_LIST, compressor_list_size); i++) {
            const Ref<CompressorListItem> &row = compressor_list[i];
            if (row == nullptr || !row.is_valid()) {
                UtilityFunctions::push_warning("[TrainBrake]: compressor_list property is null at index " + String::num(i));
                continue;
            }
            p_mover->CompressorList[Maszyna::TCompressorList::cl_Allow][i + 1] = row->get_allow();
            p_mover->CompressorList[Maszyna::TCompressorList::cl_SpeedFactor][i + 1] = row->get_speed_factor();
            p_mover->CompressorList[Maszyna::TCompressorList::cl_MinFactor][i + 1] = row->get_min_pressure_factor();
            p_mover->CompressorList[Maszyna::TCompressorList::cl_MaxFactor][i + 1] = row->get_max_pressure_factor();
        }
    }
} // namespace godot
