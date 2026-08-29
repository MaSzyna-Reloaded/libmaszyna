#include "../core/TrainController.hpp"
#include "../core/TrainPart.hpp"
#include "../core/TrainSystem.hpp"
#include "../engines/TrainEngine.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

    const char *TrainController::mover_config_changed_signal = "mover_config_changed";
    const char *TrainController::mover_initialized_signal = "mover_initialized";
    const char *TrainController::power_changed_signal = "power_changed";
    const char *TrainController::command_received = "command_received";
    const char *TrainController::radio_toggled = "radio_toggled";
    const char *TrainController::radio_channel_changed = "radio_channel_changed";
    const char *TrainController::config_changed = "config_changed";

    void TrainController::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_state"), &TrainController::get_state);
        ClassDB::bind_method(D_METHOD("get_config"), &TrainController::get_config);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::DICTIONARY, "state", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_DEFAULT),
                "", "get_state");
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::DICTIONARY, "config", PROPERTY_HINT_NONE, "",
                        PROPERTY_USAGE_READ_ONLY | PROPERTY_USAGE_DEFAULT),
                "", "get_config");

        ClassDB::bind_method(
                D_METHOD("send_command", "command", "p1", "p2"), &TrainController::send_command, DEFVAL(Variant()),
                DEFVAL(Variant()));

        ClassDB::bind_method(
                D_METHOD("broadcast_command", "command", "p1", "p2"), &TrainController::broadcast_command,
                DEFVAL(Variant()), DEFVAL(Variant()));


        ClassDB::bind_method(D_METHOD("register_command", "command", "callable"), &TrainController::register_command);
        ClassDB::bind_method(
                D_METHOD("unregister_command", "command", "callable"), &TrainController::unregister_command);
        ClassDB::bind_method(D_METHOD("battery", "enabled"), &TrainController::battery);
        ClassDB::bind_method(
                D_METHOD("main_controller_increase", "step"), &TrainController::main_controller_increase, DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("main_controller_decrease", "step"), &TrainController::main_controller_decrease, DEFVAL(1));
        ClassDB::bind_method(D_METHOD("direction_increase"), &TrainController::direction_increase);
        ClassDB::bind_method(D_METHOD("direction_decrease"), &TrainController::direction_decrease);
        ClassDB::bind_method(D_METHOD("radio", "enabled"), &TrainController::radio);
        ClassDB::bind_method(
                D_METHOD("radio_channel_increase", "step"), &TrainController::radio_channel_increase, DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("radio_channel_decrease", "step"), &TrainController::radio_channel_decrease, DEFVAL(1));
        ClassDB::bind_method(D_METHOD("update_mover"), &TrainController::update_mover);
        ClassDB::bind_method(D_METHOD("update_state"), &TrainController::update_state);
        ClassDB::bind_method(D_METHOD("update_config"), &TrainController::update_config);

        BIND_PROPERTY(
                Variant::STRING, "train_id", "train_id", &TrainController::set_train_id, &TrainController::get_train_id,
                "train_id");
        BIND_PROPERTY(
                Variant::STRING, "type_name", "type_name", &TrainController::set_type_name,
                &TrainController::get_type_name, "type_name");
        BIND_PROPERTY(Variant::FLOAT, "mass", "mass", &TrainController::set_mass, &TrainController::get_mass, "mass");
        BIND_PROPERTY(
                Variant::FLOAT, "power", "power", &TrainController::set_power, &TrainController::get_power, "power");
        BIND_PROPERTY(
                Variant::FLOAT, "max_velocity", "max_velocity", &TrainController::set_max_velocity,
                &TrainController::get_max_velocity, "max_velocity");
        BIND_PROPERTY(
                Variant::INT, "radio_channel_min", "radio_channel/min", &TrainController::set_radio_channel_min,
                &TrainController::get_radio_channel_min, "radio_channel_min");
        BIND_PROPERTY(
                Variant::INT, "radio_channel_max", "radio_channel/max", &TrainController::set_radio_channel_max,
                &TrainController::get_radio_channel_max, "radio_channel_max");
        /* FIXME: move to TrainPower section? */
        BIND_PROPERTY_W_HINT(
                Variant::FLOAT, "battery_voltage", "battery_voltage", &TrainController::set_battery_voltage,
                &TrainController::get_battery_voltage, "battery_voltage", PROPERTY_HINT_RANGE, "0,500,1");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "category", "category", &TrainController::set_category, &TrainController::get_category,
                "category", PROPERTY_HINT_ENUM, "Train,Road,Ship,Airplane");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "train_type", "train_type", &TrainController::set_train_type,
                &TrainController::get_train_type, "train_type", PROPERTY_HINT_ENUM,
                "Default,EZT,ET41,ET42,PseudoDiesel,ET22,SN61,EP05,ET40,T181,DMU");
        BIND_PROPERTY(
                Variant::FLOAT, "reduced_mass", "reduced_mass", &TrainController::set_reduced_mass,
                &TrainController::get_reduced_mass, "reduced_mass");
        BIND_PROPERTY(
                Variant::FLOAT, "sand_capacity", "sand_capacity", &TrainController::set_sand_capacity,
                &TrainController::get_sand_capacity, "sand_capacity");
        BIND_PROPERTY(
                Variant::FLOAT, "heating_power", "heating_power", &TrainController::set_heating_power,
                &TrainController::get_heating_power, "heating_power");
        BIND_PROPERTY(
                Variant::FLOAT, "light_power", "light_power", &TrainController::set_light_power,
                &TrainController::get_light_power, "light_power");
        BIND_PROPERTY(
                Variant::FLOAT, "length", "dimensions/length", &TrainController::set_length,
                &TrainController::get_length, "length");
        BIND_PROPERTY(
                Variant::FLOAT, "height", "dimensions/height", &TrainController::set_height,
                &TrainController::get_height, "height");
        BIND_PROPERTY(
                Variant::FLOAT, "width", "dimensions/width", &TrainController::set_width, &TrainController::get_width,
                "width");
        BIND_PROPERTY(
                Variant::FLOAT, "drag_coefficient", "dimensions/drag_coefficient",
                &TrainController::set_drag_coefficient, &TrainController::get_drag_coefficient, "drag_coefficient");
        BIND_PROPERTY(
                Variant::FLOAT, "floor_height", "dimensions/floor_height", &TrainController::set_floor_height,
                &TrainController::get_floor_height, "floor_height");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "battery_start_mode", "cntrl/battery_start_mode", &TrainController::set_battery_start_mode,
                &TrainController::get_battery_start_mode, "battery_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "ground_relay_start_mode", "cntrl/ground_relay_start_mode",
                &TrainController::set_ground_relay_start_mode, &TrainController::get_ground_relay_start_mode,
                "ground_relay_start_mode", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "compartment_lights_start_mode", "cntrl/compartment_lights_start_mode",
                &TrainController::set_compartment_lights_start_mode,
                &TrainController::get_compartment_lights_start_mode, "compartment_lights_start_mode",
                PROPERTY_HINT_ENUM, "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(
                Variant::BOOL, "automatic_cab_activation", "cntrl/automatic_cab_activation",
                &TrainController::set_automatic_cab_activation, &TrainController::get_automatic_cab_activation,
                "automatic_cab_activation");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "inactive_cab_flag", "cntrl/inactive_cab_flag", &TrainController::set_inactive_cab_flag,
                &TrainController::get_inactive_cab_flag, "inactive_cab_flag", PROPERTY_HINT_FLAGS,
                "Emergency Brake,Toggle Mirrors,Raise Second Pantograph,End Of Train Lights,Grant Both Side Permits,"
                "Apply Spring Brake,Release Spring Brake,Reset Direction");

        ADD_SIGNAL(MethodInfo(mover_config_changed_signal));
        ADD_SIGNAL(MethodInfo(mover_initialized_signal));
        ADD_SIGNAL(MethodInfo(power_changed_signal, PropertyInfo(Variant::BOOL, "is_powered")));
        ADD_SIGNAL(MethodInfo(radio_toggled, PropertyInfo(Variant::BOOL, "is_enabled")));
        ADD_SIGNAL(MethodInfo(radio_channel_changed, PropertyInfo(Variant::INT, "channel")));
        ADD_SIGNAL(MethodInfo(config_changed));
        ADD_SIGNAL(MethodInfo(
                command_received, PropertyInfo(Variant::STRING, "command"), PropertyInfo(Variant::NIL, "p1"),
                PropertyInfo(Variant::NIL, "p2")));

        BIND_ENUM_CONSTANT(POWER_SOURCE_NOT_DEFINED);
        BIND_ENUM_CONSTANT(POWER_SOURCE_INTERNAL);
        BIND_ENUM_CONSTANT(POWER_SOURCE_TRANSDUCER);
        BIND_ENUM_CONSTANT(POWER_SOURCE_GENERATOR);
        BIND_ENUM_CONSTANT(POWER_SOURCE_ACCUMULATOR);
        BIND_ENUM_CONSTANT(POWER_SOURCE_CURRENTCOLLECTOR);
        BIND_ENUM_CONSTANT(POWER_SOURCE_POWERCABLE);
        BIND_ENUM_CONSTANT(POWER_SOURCE_HEATER);
        BIND_ENUM_CONSTANT(POWER_SOURCE_MAIN);

        BIND_ENUM_CONSTANT(POWER_TYPE_NONE);
        BIND_ENUM_CONSTANT(POWER_TYPE_BIO);
        BIND_ENUM_CONSTANT(POWER_TYPE_MECH);
        BIND_ENUM_CONSTANT(POWER_TYPE_ELECTRIC);
        BIND_ENUM_CONSTANT(POWER_TYPE_STEAM);

        BIND_ENUM_CONSTANT(CATEGORY_TRAIN);
        BIND_ENUM_CONSTANT(CATEGORY_ROAD);
        BIND_ENUM_CONSTANT(CATEGORY_SHIP);
        BIND_ENUM_CONSTANT(CATEGORY_AIRPLANE);

        BIND_ENUM_CONSTANT(TRAIN_TYPE_DEFAULT);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_EZT);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_ET41);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_ET42);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_PSEUDODIESEL);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_ET22);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_SN61);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_EP05);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_ET40);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_181);
        BIND_ENUM_CONSTANT(TRAIN_TYPE_DMU);

        BIND_ENUM_CONSTANT(START_MODE_DISABLED);
        BIND_ENUM_CONSTANT(START_MODE_MANUAL);
        BIND_ENUM_CONSTANT(START_MODE_AUTOMATIC);
        BIND_ENUM_CONSTANT(START_MODE_MANUAL_WITH_AUTO_FALLBACK);
        BIND_ENUM_CONSTANT(START_MODE_CONVERTER);
        BIND_ENUM_CONSTANT(START_MODE_BATTERY);
        BIND_ENUM_CONSTANT(START_MODE_DIRECTION);
    }

    TMoverParameters *TrainController::get_mover() const {
        return mover;
    }

    void TrainController::initialize_mover() {
        const auto initial_vel = this->initial_velocity;
        const auto mover_type_name = std::string(type_name.utf8().ptr());
        const auto name = std::string(this->get_name().left(this->get_name().length()).utf8().ptr());
        mover = std::make_unique<TMoverParameters>(initial_vel, mover_type_name, name, this->cabin_number).release();

        dirty = true;
        dirty_prop = true;
        _update_mover_config_if_dirty();

        /* FIXME: CheckLocomotiveParameters should be called after (re)initialization */
        mover->CheckLocomotiveParameters(true, 0); // FIXME: brakujace parametery

        /* CheckLocomotiveParameters() will reset some parameters, so the changes
         * must be applied second time */

        dirty = true;
        dirty_prop = true;
        _update_mover_config_if_dirty();

        /* FIXME: remove test data */
        mover->CabActive = 1;
        mover->CabMaster = true;
        mover->CabOccupied = 1;
        mover->AutomaticCabActivation = true;
        mover->CabActivisationAuto();
        mover->CabActivisation();

        /* switch_physics() raczej trzeba zostawic */
        mover->switch_physics(true);

        DEBUG("[MaSzyna::TMoverParameters] Mover initialized successfully");
        emit_signal(mover_initialized_signal);
    }

    void TrainController::register_command(const String &p_command, const Callable &p_callable) {
        TrainSystem::get_instance()->register_command(train_id, p_command, p_callable);
    }

    void TrainController::unregister_command(const String &p_command, const Callable &p_callable) {
        TrainSystem::get_instance()->unregister_command(train_id, p_command, p_callable);
    }

    void TrainController::_notification(const int p_what) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE:
                TrainSystem::get_instance()->register_train(train_id, this);
                register_command("battery", Callable(this, "battery"));
                register_command("main_controller_increase", Callable(this, "main_controller_increase"));
                register_command("main_controller_decrease", Callable(this, "main_controller_decrease"));
                register_command("direction_increase", Callable(this, "direction_increase"));
                register_command("direction_decrease", Callable(this, "direction_decrease"));
                register_command("radio", Callable(this, "radio"));
                register_command("radio_channel_set", Callable(this, "radio_channel_set"));
                register_command("radio_channel_increase", Callable(this, "radio_channel_increase"));
                register_command("radio_channel_decrease", Callable(this, "radio_channel_decrease"));
                break;
            case NOTIFICATION_EXIT_TREE:
                unregister_command("battery", Callable(this, "battery"));
                unregister_command("main_controller_increase", Callable(this, "main_controller_increase"));
                unregister_command("main_controller_decrease", Callable(this, "main_controller_decrease"));
                unregister_command("direction_increase", Callable(this, "direction_increase"));
                unregister_command("direction_decrease", Callable(this, "direction_decrease"));
                unregister_command("radio", Callable(this, "radio"));
                unregister_command("radio_channel_set", Callable(this, "radio_channel_set"));
                unregister_command("radio_channel_increase", Callable(this, "radio_channel_increase"));
                unregister_command("radio_channel_decrease", Callable(this, "radio_channel_decrease"));
                TrainSystem::get_instance()->unregister_train(train_id);
                break;
            case NOTIFICATION_READY:
                initialize_mover();
                update_state();
                DEBUG("TrainController::_ready() signals connected to train parts");

                emit_signal(power_changed_signal, prev_is_powered);
                emit_signal(radio_channel_changed, prev_radio_channel);
                break;
            default:;
        }
    }

    void TrainController::_update_mover_config_if_dirty() {
        if (dirty) {
            /* update all train parts
             */
            emit_signal(mover_config_changed_signal);

            dirty = false;
            dirty_prop = true; // sforsowanie odswiezenia stanu lokalnych propsow
        }

        if (dirty_prop) {
            update_mover();
            dirty_prop = false;
        }
    }

    void TrainController::_process_mover(const double p_delta) {
        TLocation mock_location;
        TRotation mock_rotation;
        mover->ComputeTotalForce(p_delta);
        // mover->compute_movement_(delta);
        mover->ComputeMovement(
                p_delta, p_delta, mover->RunningShape, mover->RunningTrack, mover->RunningTraction, mock_location,
                mock_rotation);

        _handle_mover_update();
    }

    void TrainController::update_state() {
        _handle_mover_update();
    }

    void TrainController::_handle_mover_update() {
        state.merge(get_mover_state(), true);

        const bool new_is_powered = (state.get("power24_available", false) || state.get("power110_available", false));
        if (prev_is_powered != new_is_powered) {
            prev_is_powered = new_is_powered; // FIXME: I don't like this
            emit_signal(power_changed_signal, prev_is_powered);
        }

        if (const bool new_radio_enabled = state.get("radio_enabled", false) && new_is_powered;
            prev_radio_enabled != new_radio_enabled) {
            prev_radio_enabled = new_radio_enabled; // FIXME: I don't like this
            emit_signal(radio_toggled, new_radio_enabled);
        }

        if (const int new_radio_channel = state.get("radio_channel", 0); prev_radio_channel != new_radio_channel) {
            prev_radio_channel = new_radio_channel; // FIXME: I don't like this
            emit_signal(radio_channel_changed, new_radio_channel);
        }
    }

    void TrainController::_process(const double p_delta) {
        /* nie daj borze w edytorze */
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }

        _update_mover_config_if_dirty();
        _process_mover(p_delta);
    }

    void TrainController::_do_update_internal_mover(TMoverParameters *p_mover) const {
        p_mover->Mass = mass;
        p_mover->Power = power;
        p_mover->Vmax = max_velocity;
        p_mover->Mred = reduced_mass;

        p_mover->ComputeMass();

        p_mover->CategoryFlag = category;
        p_mover->TrainType = train_type;
        p_mover->SandCapacity = sand_capacity;
        p_mover->HeatingPower = heating_power;
        p_mover->LightPower = light_power;

        p_mover->Dim.L = length;
        p_mover->Dim.H = height;
        p_mover->Dim.W = width;
        p_mover->Cx = drag_coefficient;
        p_mover->Floor = static_cast<float>(floor_height);

        p_mover->BatteryStart = start_mode_map.at(battery_start_mode);
        p_mover->GroundRelayStart = start_mode_map.at(ground_relay_start_mode);
        p_mover->CompartmentLights.start_type = start_mode_map.at(compartment_lights_start_mode);
        p_mover->AutomaticCabActivation = automatic_cab_activation;
        p_mover->InactiveCabFlag = inactive_cab_flag;

        // FIXME: move to TrainPower
        p_mover->BatteryVoltage = battery_voltage;
        p_mover->NominalBatteryVoltage = static_cast<float>(battery_voltage); // LoadFIZ_Light
    }

    void TrainController::_do_fetch_config_from_mover(const TMoverParameters *p_mover, Dictionary &p_config) const {}

    void TrainController::update_mover() {
        if (TMoverParameters *mover = get_mover(); mover != nullptr) {
            _do_update_internal_mover(mover);
            Dictionary new_config;
            _do_fetch_config_from_mover(mover, new_config);
            update_config(new_config);

            /* FIXME: CheckLocomotiveParameters should be called after (re)initialization */
            mover->CheckLocomotiveParameters(true, 0); // FIXME: brakujace parametery
        } else {
            UtilityFunctions::push_warning("TrainController::update_mover() failed: internal mover not initialized");
        }
    }

    Dictionary TrainController::get_mover_state() {
        if (TMoverParameters *mover = get_mover(); mover != nullptr) {
            _do_fetch_state_from_mover(mover, state);
        } else {
            UtilityFunctions::push_warning("TrainController::get_mover_state() failed: internal mover not initialized");
        }
        return internal_state;
    }

    void TrainController::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        internal_state["mass_total"] = p_mover->TotalMass;
        internal_state["velocity"] = p_mover->V;
        internal_state["speed"] = p_mover->Vel;
        internal_state["total_distance"] = p_mover->DistCounter;
        internal_state["direction"] = p_mover->DirActive;
        internal_state["cabin"] = p_mover->CabActive;
        internal_state["cabin_controleable"] = p_mover->IsCabMaster();
        internal_state["cabin_occupied"] = p_mover->CabOccupied;

        /* FIXME: move to TrainPower section? */
        internal_state["battery_enabled"] = p_mover->Battery;
        internal_state["battery_voltage"] = p_mover->BatteryVoltage;

        /* FIXME: move to TrainRadio section? */
        internal_state["radio_enabled"] = p_mover->Radio;
        internal_state["radio_powered"] =
                p_mover->Radio && (p_mover->Power24vIsAvailable || p_mover->Power110vIsAvailable);
        internal_state["radio_channel"] = radio_channel;

        /* FIXME: move to TrainPower section */
        internal_state["power24_voltage"] = p_mover->Power24vVoltage;
        internal_state["power24_available"] = p_mover->Power24vIsAvailable;
        internal_state["power110_available"] = p_mover->Power110vIsAvailable;
        internal_state["current0"] = p_mover->ShowCurrent(0);
        internal_state["current1"] = p_mover->ShowCurrent(1);
        internal_state["current2"] = p_mover->ShowCurrent(2);
        internal_state["relay_novolt"] = p_mover->NoVoltRelay;
        internal_state["relay_overvoltage"] = p_mover->OvervoltageRelay;
        internal_state["relay_ground"] = p_mover->GroundRelay;
        internal_state["train_damage"] = p_mover->DamageFlag;
        internal_state["controller_second_position"] = p_mover->ScndCtrlPos;
        internal_state["controller_main_position"] = p_mover->MainCtrlPos;
    }

    Dictionary TrainController::get_config() const {
        return config;
    }

    void TrainController::update_config(const Dictionary &p_config) {
        config.merge(p_config, true);
        emit_signal(config_changed);
    }

    Dictionary TrainController::get_state() {
        return state;
    }

    void
    TrainController::emit_command_received_signal(const String &p_command, const Variant &p_p1, const Variant &p_p2) {
        emit_signal(command_received, p_command, p_p1, p_p2);
    }

    void TrainController::broadcast_command(const String &p_command, const Variant &p_p1, const Variant &p_p2) {
        TrainSystem::get_instance()->broadcast_command(p_command, p_p1, p_p2);
    }

    void TrainController::send_command(const StringName &p_command, const Variant &p_p1, const Variant &p_p2) const {
        TrainSystem::get_instance()->send_command(train_id, String(p_command), p_p1, p_p2);
    }

    void TrainController::battery(const bool p_enabled) const {
        mover->BatterySwitch(p_enabled);
    }

    void TrainController::main_controller_increase(const int p_step) const {
        const int step = p_step > 0 ? p_step : 1;
        mover->IncMainCtrl(step);
    }

    void TrainController::main_controller_decrease(const int p_step) const {
        const int step = p_step > 0 ? p_step : 1;
        mover->DecMainCtrl(step);
    }

    void TrainController::direction_increase() const {
        mover->DirectionForward();
    }

    void TrainController::direction_decrease() const {
        mover->DirectionBackward();
    }

    void TrainController::radio_channel_increase(const int p_step) {
        const int step = p_step > 0 ? p_step : 1;
        radio_channel = Math::clamp(radio_channel + step, radio_channel_min, radio_channel_max);
    }

    void TrainController::radio_channel_decrease(const int p_step) {
        const int step = (p_step != 0) ? p_step : 1;
        radio_channel = Math::clamp(radio_channel - step, radio_channel_min, radio_channel_max);
    }

    void TrainController::radio_channel_set(const int p_channel) {
        radio_channel = Math::clamp(p_channel, radio_channel_min, radio_channel_max);
    }

    void TrainController::radio(const bool p_enabled) {
        mover->Radio = p_enabled;
    }
} // namespace godot
