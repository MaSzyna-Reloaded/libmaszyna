#include "RailVehicleController.hpp"
#include "vehicles/base/VehicleComponent.hpp"

namespace godot {
    const char *RailVehicleController::trainset_changed_signal = "trainset_changed";
    const char *RailVehicleController::coupler_attached_signal = "coupler_attached";
    const char *RailVehicleController::coupler_adapter_attached_signal = "coupler_adapter_attached";
    const char *RailVehicleController::coupler_adapter_removed_signal = "coupler_adapter_removed";
    const char *RailVehicleController::coupler_detached_signal = "coupler_detached";

    RailVehicleController::CouplerEnd RailVehicleController::opposite_end(const CouplerEnd p_end) {
        return p_end == COUPLER_END_FRONT ? COUPLER_END_REAR : COUPLER_END_FRONT;
    }

    Ref<VehicleComponent> RailVehicleController::get_rail_component(const RailVehicleComponentType::Type p_type) const {
        return _get_component_of_type(p_type);
    }

    TypedArray<VehicleComponent>
    RailVehicleController::find_rail_components(const RailVehicleComponentType::Type p_type) const {
        return _find_components_of_type(p_type);
    }

    void RailVehicleController::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_rail_component", "type"), &RailVehicleController::get_rail_component);
        ClassDB::bind_method(D_METHOD("find_rail_components", "type"), &RailVehicleController::find_rail_components);
        ClassDB::bind_method(D_METHOD("cab_activation", "enabled"), &RailVehicleController::cab_activation);
        ClassDB::bind_method(D_METHOD("compartment_lights", "enabled"), &RailVehicleController::compartment_lights);
        ClassDB::bind_method(
                D_METHOD("compartment_lights_switch_off", "enabled"),
                &RailVehicleController::compartment_lights_switch_off);
        ClassDB::bind_method(D_METHOD("cab_activation_auto"), &RailVehicleController::cab_activation_auto);
        ClassDB::bind_method(D_METHOD("cab_deactivation_auto"), &RailVehicleController::cab_deactivation_auto);
        ClassDB::bind_method(D_METHOD("cab_controls_reset"), &RailVehicleController::cab_controls_reset);
        ClassDB::bind_method(D_METHOD("cabin_leave"), &RailVehicleController::cabin_leave);
        ClassDB::bind_method(D_METHOD("cabin_enter"), &RailVehicleController::cabin_enter);
        ClassDB::bind_method(D_METHOD("ground_relay_reset"), &RailVehicleController::ground_relay_reset);
        ClassDB::bind_method(D_METHOD("antislip"), &RailVehicleController::antislip);
        ClassDB::bind_method(
                D_METHOD("main_controller_increase", "step"), &RailVehicleController::main_controller_increase,
                DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("main_controller_decrease", "step"), &RailVehicleController::main_controller_decrease,
                DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("main_controller_set_position", "position"),
                &RailVehicleController::main_controller_set_position);
        ClassDB::bind_method(
                D_METHOD("second_controller_increase", "step"), &RailVehicleController::second_controller_increase,
                DEFVAL(1));
        ClassDB::bind_method(
                D_METHOD("second_controller_decrease", "step"), &RailVehicleController::second_controller_decrease,
                DEFVAL(1));
        ClassDB::bind_method(D_METHOD("direction_increase"), &RailVehicleController::direction_increase);
        ClassDB::bind_method(D_METHOD("direction_decrease"), &RailVehicleController::direction_decrease);
        ClassDB::bind_method(D_METHOD("process_movement", "delta"), &RailVehicleController::process_movement);
        ClassDB::bind_method(D_METHOD("update_location"), &RailVehicleController::update_location);
        ClassDB::bind_method(D_METHOD("compute_forces", "delta"), &RailVehicleController::compute_forces);
        ClassDB::bind_method(D_METHOD("compute_movement", "delta"), &RailVehicleController::compute_movement);
        ClassDB::bind_method(D_METHOD("compute_fast_movement", "delta"), &RailVehicleController::compute_fast_movement);
        ClassDB::bind_method(
                D_METHOD("couple", "other", "end", "other_end", "coupling"), &RailVehicleController::couple);
        ClassDB::bind_static_method(
                "RailVehicleController", D_METHOD("opposite_end", "end"), &RailVehicleController::opposite_end);
        ClassDB::bind_method(D_METHOD("uncouple", "end"), &RailVehicleController::uncouple);
        ClassDB::bind_method(D_METHOD("is_coupled", "end"), &RailVehicleController::is_coupled);
        ClassDB::bind_method(D_METHOD("is_coupled_by", "end", "flags"), &RailVehicleController::is_coupled_by);
        ClassDB::bind_method(D_METHOD("get_coupled_controller", "end"), &RailVehicleController::get_coupled_controller);
        ClassDB::bind_method(D_METHOD("get_coupled_end", "end"), &RailVehicleController::get_coupled_end);
        ClassDB::bind_method(D_METHOD("coupler_connect", "where"), &RailVehicleController::coupler_connect);
        ClassDB::bind_method(D_METHOD("coupler_disconnect", "where"), &RailVehicleController::coupler_disconnect);
        ClassDB::bind_method(
                D_METHOD("coupler_adapter_attach", "where"), &RailVehicleController::coupler_adapter_attach);
        ClassDB::bind_method(D_METHOD("coupler_adapter_fit", "end"), &RailVehicleController::coupler_adapter_fit);
        ClassDB::bind_method(D_METHOD("is_coupler_automatic", "end"), &RailVehicleController::is_coupler_automatic);
        ClassDB::bind_method(
                D_METHOD("get_coupler_joinable_flags", "end"), &RailVehicleController::get_coupler_joinable_flags);
        ClassDB::bind_method(
                D_METHOD("coupler_adapter_remove", "where"), &RailVehicleController::coupler_adapter_remove);
        ClassDB::bind_method(
                D_METHOD("get_coupler_adapter_fitted_model", "end"),
                &RailVehicleController::get_coupler_adapter_fitted_model);
        ClassDB::bind_method(
                D_METHOD("get_coupler_adapter_fitted_length", "end"),
                &RailVehicleController::get_coupler_adapter_fitted_length);
        ClassDB::bind_method(
                D_METHOD("get_coupler_adapter_fitted_height", "end"),
                &RailVehicleController::get_coupler_adapter_fitted_height);
        ClassDB::bind_method(
                D_METHOD("set_coupler_adapter_model", "value"), &RailVehicleController::set_coupler_adapter_model);
        ClassDB::bind_method(D_METHOD("get_coupler_adapter_model"), &RailVehicleController::get_coupler_adapter_model);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "coupler_adapter_model"), "set_coupler_adapter_model",
                "get_coupler_adapter_model");
        ClassDB::bind_method(
                D_METHOD("set_coupler_adapter_length", "value"), &RailVehicleController::set_coupler_adapter_length);
        ClassDB::bind_method(
                D_METHOD("get_coupler_adapter_length"), &RailVehicleController::get_coupler_adapter_length);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "coupler_adapter_length"), "set_coupler_adapter_length",
                "get_coupler_adapter_length");
        ClassDB::bind_method(
                D_METHOD("set_coupler_adapter_height", "value"), &RailVehicleController::set_coupler_adapter_height);
        ClassDB::bind_method(
                D_METHOD("get_coupler_adapter_height"), &RailVehicleController::get_coupler_adapter_height);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "coupler_adapter_height"), "set_coupler_adapter_height",
                "get_coupler_adapter_height");
        BIND_PROPERTY_W_HINT(
                RailVehicleController, Variant::INT, train_type, PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"Default", TRAIN_TYPE_DEFAULT},
                         {"EZT", TRAIN_TYPE_EZT},
                         {"ET41", TRAIN_TYPE_ET41},
                         {"ET42", TRAIN_TYPE_ET42},
                         {"PseudoDiesel", TRAIN_TYPE_PSEUDODIESEL},
                         {"ET22", TRAIN_TYPE_ET22},
                         {"SN61", TRAIN_TYPE_SN61},
                         {"EP05", TRAIN_TYPE_EP05},
                         {"ET40", TRAIN_TYPE_ET40},
                         {"T181", TRAIN_TYPE_181},
                         {"DMU", TRAIN_TYPE_DMU}}));
        BIND_PROPERTY(RailVehicleController, Variant::STRING, type_name);
        BIND_PROPERTY(RailVehicleController, Variant::STRING, load_name);
        BIND_PROPERTY(RailVehicleController, Variant::FLOAT, load_amount);
        BIND_PROPERTY(RailVehicleController, Variant::FLOAT, reduced_mass);
        BIND_PROPERTY(RailVehicleController, Variant::FLOAT, sand_capacity);
        BIND_PROPERTY(RailVehicleController, Variant::FLOAT, heating_power);
        BIND_PROPERTY(RailVehicleController, Variant::FLOAT, light_power);
        BIND_PROPERTY_W_HINT(
                RailVehicleController, Variant::INT, cntrl_ground_relay_start_mode, "cntrl", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY_W_HINT(
                RailVehicleController, Variant::INT, cntrl_compartment_lights_start_mode, "cntrl", PROPERTY_HINT_ENUM,
                "Disabled,Manual,Automatic,ManualWithAutoFallback,Converter,Battery,Direction");
        BIND_PROPERTY(RailVehicleController, Variant::BOOL, cntrl_automatic_cab_activation, "cntrl");
        BIND_PROPERTY_W_HINT(
                RailVehicleController, Variant::INT, cntrl_inactive_cab_flag, "cntrl", PROPERTY_HINT_FLAGS,
                "Emergency Brake,Toggle Mirrors,Raise Second Pantograph,End Of Train Lights,Grant Both Side Permits,"
                "Apply Spring Brake,Release Spring Brake,Reset Direction");

        ADD_SIGNAL(MethodInfo(trainset_changed_signal));
        const PropertyInfo coupling_flag(
                Variant::INT, "flag", PROPERTY_HINT_NONE, "", PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_BITFIELD,
                "RailVehicleController.CouplingFlags");
        ADD_SIGNAL(MethodInfo(coupler_attached_signal, coupling_flag));
        ADD_SIGNAL(MethodInfo(coupler_detached_signal, coupling_flag));
        ADD_SIGNAL(MethodInfo(coupler_adapter_attached_signal, PropertyInfo(Variant::INT, "end")));
        ADD_SIGNAL(MethodInfo(coupler_adapter_removed_signal, PropertyInfo(Variant::INT, "end")));

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

        BIND_ENUM_CONSTANT(COUPLER_END_FRONT);
        BIND_ENUM_CONSTANT(COUPLER_END_REAR);

        BIND_BITFIELD_FLAG(COUPLING_FLAG_NONE);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_COUPLER);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_BRAKEHOSE);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_CONTROL);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_HIGHVOLTAGE);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_GANGWAY);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_MAINHOSE);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_HEATING);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_PERMANENT);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_POWER_24V);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_POWER_110V);
        BIND_BITFIELD_FLAG(COUPLING_FLAG_POWER_3X400V);

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

        ClassDB::bind_method(D_METHOD("get_direction_absolute"), &RailVehicleController::get_direction_absolute);
        ClassDB::bind_method(D_METHOD("get_train_damage"), &RailVehicleController::get_train_damage);
        ClassDB::bind_method(D_METHOD("get_mass_reduced"), &RailVehicleController::get_mass_reduced);
        ClassDB::bind_method(D_METHOD("get_coupler_stretched"), &RailVehicleController::get_coupler_stretched);
        ClassDB::bind_method(
                D_METHOD("get_compartment_lights_enabled"), &RailVehicleController::get_compartment_lights_enabled);
        ClassDB::bind_method(
                D_METHOD("get_compartment_lights_active"), &RailVehicleController::get_compartment_lights_active);
    }

    void RailVehicleController::_register_commands() {
        register_command("cab_deactivation_auto", Callable(this, "cab_deactivation_auto"));
        register_command("cab_controls_reset", Callable(this, "cab_controls_reset"));
        register_command("cabin_leave", Callable(this, "cabin_leave"));
        register_command("cabin_enter", Callable(this, "cabin_enter"));
        register_command("ground_relay_reset", Callable(this, "ground_relay_reset"));
        register_command("antislip", Callable(this, "antislip"));
        register_command("cab_activation", Callable(this, "cab_activation"));
        register_command("compartment_lights", Callable(this, "compartment_lights"));
        register_command("compartment_lights_switch_off", Callable(this, "compartment_lights_switch_off"));
        register_command("cab_activation_auto", Callable(this, "cab_activation_auto"));
        register_command("main_controller_increase", Callable(this, "main_controller_increase"));
        register_command("main_controller_decrease", Callable(this, "main_controller_decrease"));
        register_command("main_controller_set_position", Callable(this, "main_controller_set_position"));
        register_command("second_controller_increase", Callable(this, "second_controller_increase"));
        register_command("second_controller_decrease", Callable(this, "second_controller_decrease"));
        register_command("direction_increase", Callable(this, "direction_increase"));
        register_command("direction_decrease", Callable(this, "direction_decrease"));
        register_command("coupler_connect", Callable(this, "coupler_connect"));
        register_command("coupler_disconnect", Callable(this, "coupler_disconnect"));
        register_command("coupler_adapter_attach", Callable(this, "coupler_adapter_attach"));
        register_command("coupler_adapter_remove", Callable(this, "coupler_adapter_remove"));
    }

    void RailVehicleController::_unregister_commands() {
        unregister_command("cab_deactivation_auto");
        unregister_command("cab_controls_reset");
        unregister_command("cabin_leave");
        unregister_command("cabin_enter");
        unregister_command("ground_relay_reset");
        unregister_command("antislip");
        unregister_command("cab_activation");
        unregister_command("compartment_lights");
        unregister_command("compartment_lights_switch_off");
        unregister_command("cab_activation_auto");
        unregister_command("main_controller_increase");
        unregister_command("main_controller_decrease");
        unregister_command("main_controller_set_position");
        unregister_command("second_controller_increase");
        unregister_command("second_controller_decrease");
        unregister_command("direction_increase");
        unregister_command("direction_decrease");
        unregister_command("coupler_connect");
        unregister_command("coupler_disconnect");
        unregister_command("coupler_adapter_attach");
        unregister_command("coupler_adapter_remove");
    }

    /* The vehicle's own state only - what it may or may not have, each component fills itself
     * (VehicleController::get_state()). */
    void RailVehicleController::_fill_state_dictionary(Dictionary &p_state) const {
        VehicleController::_fill_state_dictionary(p_state);
        if (!is_simulation_ready()) {
            return;
        }
        p_state["direction_absolute"] = get_direction_absolute();
        p_state["train_damage"] = get_train_damage();
        p_state["mass_reduced"] = get_mass_reduced();
        p_state["coupler_stretched"] = get_coupler_stretched();
        p_state["compartment_lights_enabled"] = get_compartment_lights_enabled();
        p_state["compartment_lights_active"] = get_compartment_lights_active();
    }

    void RailVehicleController::set_coupler_adapter_model(const String &p_value) {
        coupler_adapter_model = p_value;
    }

    String RailVehicleController::get_coupler_adapter_model() const {
        return coupler_adapter_model;
    }

    void RailVehicleController::set_coupler_adapter_length(const double p_value) {
        coupler_adapter_length = p_value;
    }

    double RailVehicleController::get_coupler_adapter_length() const {
        return coupler_adapter_length;
    }

    void RailVehicleController::set_coupler_adapter_height(const double p_value) {
        coupler_adapter_height = p_value;
    }

    double RailVehicleController::get_coupler_adapter_height() const {
        return coupler_adapter_height;
    }

    void RailVehicleController::set_driver_cabin_kind(const RailVehicleCabinKind::Kind p_kind) {
        driver_cabin_kind = p_kind;
    }

    RailVehicleCabinKind::Kind RailVehicleController::get_driver_cabin_kind() const {
        return driver_cabin_kind;
    }
} // namespace godot
