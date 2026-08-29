#include "TrainEngine.hpp"
#include "macros.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    class TrainController;
    void TrainEngine::_bind_methods() {
        ClassDB::bind_method(D_METHOD("main_switch", "enabled"), &TrainEngine::main_switch);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "motor_param_table", "motor_param_table", &TrainEngine::set_motor_param_table,
                &TrainEngine::get_motor_param_table, "motor_param_table", PROPERTY_HINT_TYPE_STRING, "MotorParameter");
        BIND_PROPERTY(
                Variant::INT, "gear_teeth_motor", "transmission/gear_teeth_motor", &TrainEngine::set_gear_teeth_motor,
                &TrainEngine::get_gear_teeth_motor, "gear_teeth_motor");
        BIND_PROPERTY(
                Variant::INT, "gear_teeth_wheel", "transmission/gear_teeth_wheel", &TrainEngine::set_gear_teeth_wheel,
                &TrainEngine::get_gear_teeth_wheel, "gear_teeth_wheel");
        BIND_PROPERTY(
                Variant::FLOAT, "gear_efficiency", "transmission/efficiency", &TrainEngine::set_gear_efficiency,
                &TrainEngine::get_gear_efficiency, "gear_efficiency");
        BIND_PROPERTY(
                Variant::FLOAT, "maximum_traction_force", "maximum_traction_force",
                &TrainEngine::set_traction_force_max, &TrainEngine::get_traction_force_max, "maximum_traction_force");
        BIND_PROPERTY(
                Variant::FLOAT, "motor_blowers_speed", "motor_blowers/speed", &TrainEngine::set_motor_blowers_speed,
                &TrainEngine::get_motor_blowers_speed, "motor_blowers_speed");
        BIND_PROPERTY(
                Variant::FLOAT, "motor_blowers_sustain_time", "motor_blowers/sustain_time",
                &TrainEngine::set_motor_blowers_sustain_time, &TrainEngine::get_motor_blowers_sustain_time,
                "motor_blowers_sustain_time");
        BIND_PROPERTY(
                Variant::FLOAT, "motor_blowers_start_velocity", "motor_blowers/start_velocity",
                &TrainEngine::set_motor_blowers_start_velocity, &TrainEngine::get_motor_blowers_start_velocity,
                "motor_blowers_start_velocity");
        BIND_PROPERTY(
                Variant::BOOL, "pressure_switch_present", "pressure_switch_present",
                &TrainEngine::set_pressure_switch_present, &TrainEngine::get_pressure_switch_present,
                "pressure_switch_present");
        BIND_PROPERTY(
                Variant::INT, "inverters_count", "inverters_count", &TrainEngine::set_inverters_count,
                &TrainEngine::get_inverters_count, "inverters_count");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "motor_blowers_start_mode", "motor_blowers/start_mode",
                &TrainEngine::set_motor_blowers_start_mode, &TrainEngine::get_motor_blowers_start_mode,
                "motor_blowers_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                Variant::INT, "main_controller_position_count", "cntrl/main_controller_position_count",
                &TrainEngine::set_main_controller_position_count, &TrainEngine::get_main_controller_position_count,
                "main_controller_position_count");
        BIND_PROPERTY(
                Variant::INT, "shunt_controller_position_count", "cntrl/shunt_controller_position_count",
                &TrainEngine::set_shunt_controller_position_count, &TrainEngine::get_shunt_controller_position_count,
                "shunt_controller_position_count");
        BIND_PROPERTY(
                Variant::INT, "direction_change_max_position", "cntrl/direction_change_max_position",
                &TrainEngine::set_direction_change_max_position, &TrainEngine::get_direction_change_max_position,
                "direction_change_max_position");
        BIND_PROPERTY(
                Variant::BOOL, "eim_control_additional_zeros", "cntrl/eim_control_additional_zeros",
                &TrainEngine::set_eim_control_additional_zeros, &TrainEngine::get_eim_control_additional_zeros,
                "eim_control_additional_zeros");
        BIND_PROPERTY(
                Variant::BOOL, "eim_control_emergency", "cntrl/eim_control_emergency",
                &TrainEngine::set_eim_control_emergency, &TrainEngine::get_eim_control_emergency,
                "eim_control_emergency");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "eim_control_type", "cntrl/eim_control_type", &TrainEngine::set_eim_control_type,
                &TrainEngine::get_eim_control_type, "eim_control_type", PROPERTY_HINT_ENUM, "0,1,2,3");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "auto_relay_mode", "cntrl/auto_relay_mode", &TrainEngine::set_auto_relay_mode,
                &TrainEngine::get_auto_relay_mode, "auto_relay_mode", PROPERTY_HINT_ENUM, "No,Yes,Optional");
        BIND_PROPERTY(
                Variant::BOOL, "coupled_controllers", "cntrl/coupled_controllers", &TrainEngine::set_coupled_controllers,
                &TrainEngine::get_coupled_controllers, "coupled_controllers");
        BIND_PROPERTY(
                Variant::BOOL, "has_camshaft", "cntrl/has_camshaft", &TrainEngine::set_has_camshaft,
                &TrainEngine::get_has_camshaft, "has_camshaft");
        BIND_PROPERTY(
                Variant::BOOL, "series_shunt_on_series_position", "cntrl/series_shunt_on_series_position",
                &TrainEngine::set_series_shunt_on_series_position, &TrainEngine::get_series_shunt_on_series_position,
                "series_shunt_on_series_position");
        BIND_PROPERTY(
                Variant::FLOAT, "initial_controller_delay", "cntrl/initial_controller_delay",
                &TrainEngine::set_initial_controller_delay, &TrainEngine::get_initial_controller_delay,
                "initial_controller_delay");
        BIND_PROPERTY(
                Variant::FLOAT, "controller_step_delay", "cntrl/controller_step_delay",
                &TrainEngine::set_controller_step_delay, &TrainEngine::get_controller_step_delay,
                "controller_step_delay");
        BIND_PROPERTY(
                Variant::FLOAT, "controller_step_down_delay", "cntrl/controller_step_down_delay",
                &TrainEngine::set_controller_step_down_delay, &TrainEngine::get_controller_step_down_delay,
                "controller_step_down_delay");
        BIND_PROPERTY(
                Variant::BOOL, "fast_series_circuit", "cntrl/fast_series_circuit", &TrainEngine::set_fast_series_circuit,
                &TrainEngine::get_fast_series_circuit, "fast_series_circuit");
        ADD_SIGNAL(MethodInfo("engine_start"));
        ADD_SIGNAL(MethodInfo("engine_stop"));

        BIND_ENUM_CONSTANT(NONE);
        BIND_ENUM_CONSTANT(DUMB);
        BIND_ENUM_CONSTANT(WHEELS_DRIVEN);
        BIND_ENUM_CONSTANT(ELECTRIC_SERIES_MOTOR);
        BIND_ENUM_CONSTANT(ELECTRIC_INDUCTION_MOTOR);
        BIND_ENUM_CONSTANT(DIESEL);
        BIND_ENUM_CONSTANT(STEAM);
        BIND_ENUM_CONSTANT(DIESEL_ELECTRIC);
        BIND_ENUM_CONSTANT(MAIN);

        BIND_ENUM_CONSTANT(START_MODE_DISABLED);
        BIND_ENUM_CONSTANT(START_MODE_MANUAL);
        BIND_ENUM_CONSTANT(START_MODE_AUTOMATIC);
        BIND_ENUM_CONSTANT(START_MODE_MANUAL_WITH_AUTO_FALLBACK);
        BIND_ENUM_CONSTANT(START_MODE_CONVERTER);
        BIND_ENUM_CONSTANT(START_MODE_BATTERY);
        BIND_ENUM_CONSTANT(START_MODE_DIRECTION);

        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_0);
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_1);
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_2);
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_3);

        BIND_ENUM_CONSTANT(AUTO_RELAY_NO);
        BIND_ENUM_CONSTANT(AUTO_RELAY_YES);
        BIND_ENUM_CONSTANT(AUTO_RELAY_OPTIONAL);
    }

    void TrainEngine::_do_update_internal_mover(TMoverParameters *p_mover) {
        p_mover->EngineType = engine_type_map.at(get_engine_type());

        p_mover->Transmision.NToothM = gear_teeth_motor;
        p_mover->Transmision.NToothW = gear_teeth_wheel;
        p_mover->Transmision.Efficiency = gear_efficiency;
        p_mover->Ftmax = traction_force_max;
        p_mover->HasControlPressureSwitch = pressure_switch_present;
        p_mover->InvertersNo = inverters_count;
        for (auto &fan: p_mover->MotorBlowers) {
            fan.speed = static_cast<float>(motor_blowers_speed);
            fan.sustain_time = static_cast<float>(motor_blowers_sustain_time);
            fan.min_start_velocity = static_cast<float>(motor_blowers_start_velocity);
            fan.start_type = start_mode_map.at(motor_blowers_start_mode);
        }

        p_mover->MainCtrlPosNo = main_controller_position_count;
        p_mover->ScndCtrlPosNo = shunt_controller_position_count;
        p_mover->MainCtrlMaxDirChangePos = direction_change_max_position;
        p_mover->EIMCtrlAdditionalZeros = eim_control_additional_zeros;
        p_mover->EIMCtrlEmergency = eim_control_emergency;
        p_mover->EIMCtrlType = eim_control_type;
        p_mover->AutoRelayType = auto_relay_mode;
        p_mover->CoupledCtrl = coupled_controllers;
        p_mover->HasCamshaft = has_camshaft;
        p_mover->ScndS = series_shunt_on_series_position;
        p_mover->InitialCtrlDelay = initial_controller_delay;
        p_mover->CtrlDelay = controller_step_delay;
        p_mover->CtrlDownDelay = controller_step_down_delay;
        p_mover->FastSerialCircuit = fast_series_circuit;

        /* FIXME: for testing purposes */
        p_mover->GroundRelay = true;
        p_mover->NoVoltRelay = true;
        p_mover->OvervoltageRelay = true;
        p_mover->DamageFlag = 0;
        p_mover->EngDmgFlag = 0;
        p_mover->ConvOvldFlag = false;
        /* end testing */

        /* motor param table */
        constexpr int MAX = Maszyna::MotorParametersArraySize;
        for (int i = 0; i < std::min(MAX, static_cast<int>(motor_param_table.size())); i++) {
            const Ref<MotorParameter> &row = motor_param_table[i];
            if (row == nullptr || !row.is_valid() || row.is_null()) {
                UtilityFunctions::push_warning(
                        "[TrainEngine]: motor_param_table property is null at index " + String::num(i));
                return;
            }

            p_mover->MotorParam[i].mIsat = row->get_saturation_current_multiplier();
            p_mover->MotorParam[i].fi = row->get_voltage_constant();
            p_mover->MotorParam[i].mfi = row->get_voltage_constant_multiplier();
            p_mover->MotorParam[i].Isat = row->get_saturation_current();
            p_mover->MPTRelay[i].Iup = row->get_shunting_up();     // bocznikowanie
            p_mover->MPTRelay[i].Idown = row->get_shunting_down(); // bocznikowanie;
        }
    }

    void TrainEngine::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        const bool previous_main_switch = (p_state.get("main_switch_enabled", false));
        p_state["main_switch_enabled"] = p_mover->Mains;
        p_state["Mm"] = p_mover->Mm;
        p_state["Mw"] = p_mover->Mw;
        p_state["Fw"] = p_mover->Fw;
        p_state["Ft"] = p_mover->Ft;
        p_state["Im"] = p_mover->Im;
        p_state["compressor_enabled"] = p_mover->CompressorFlag;
        p_state["compressor_allowed"] = p_mover->CompressorAllow;
        p_state["engine_power"] = p_mover->EnginePower;
        p_state["engine_rpm_count"] = p_mover->enrot;
        p_state["engine_rpm_ratio"] = p_mover->EngineRPMRatio();
        p_state["engine_current"] = p_mover->Im;
        p_state["engine_damage"] = p_mover->EngDmgFlag;
        p_state["main_switch_time"] = p_mover->MainsInitTimeCountdown;
        p_state["main_no_power_pos"] = p_mover->IsMainCtrlNoPowerPos();
        p_state["camshaft_available"] = p_mover->HasCamshaft;
        p_state["converter_overload"] = p_mover->ConvOvldFlag;
        p_state["line_breaker_delay"] = p_mover->CtrlDelay;
        p_state["line_breaker_initial_delay"] = p_mover->InitialCtrlDelay;
        p_state["line_breaker_closes_at_no_power"] = p_mover->LineBreakerClosesOnlyAtNoPowerPos;

        if (!previous_main_switch && (static_cast<bool>(p_state["main_switch_enabled"]))) {
            emit_signal("engine_start");
        } else if (previous_main_switch && !(static_cast<bool>(p_state["main_switch_enabled"]))) {
            emit_signal("engine_stop");
        }
    }

    void TrainEngine::_do_fetch_config_from_mover(TMoverParameters *p_mover, Dictionary &p_config) {
        p_config["main_controller_position_max"] = p_mover->MainCtrlPosNo;
    }

    void TrainEngine::main_switch(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->MainSwitch(p_enabled);
    }

    void TrainEngine::_register_commands() {
        register_command("main_switch", Callable(this, "main_switch"));
    }

    void TrainEngine::_unregister_commands() {
        unregister_command("main_switch", Callable(this, "main_switch"));
    }
} // namespace godot
