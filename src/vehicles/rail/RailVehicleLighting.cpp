#include "RailVehicleLighting.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void RailVehicleLighting::_bind_methods() {
        BIND_PROPERTY(RailVehicleLighting, Variant::COLOR, head_light_color, "head_light");
        BIND_PROPERTY(RailVehicleLighting, Variant::FLOAT, head_light_dimmed_multiplier, "head_light");
        BIND_PROPERTY(RailVehicleLighting, Variant::FLOAT, head_light_normal_multiplier, "head_light");
        BIND_PROPERTY(
                RailVehicleLighting, Variant::FLOAT, head_light_high_beam_dimmed_multiplier, "head_light/high_beam");
        BIND_PROPERTY(
                RailVehicleLighting, Variant::FLOAT, head_light_high_beam_normal_multiplier, "head_light/high_beam");
        BIND_PROPERTY(RailVehicleLighting, Variant::INT, lights_default_selector_position, "lights");
        BIND_PROPERTY(RailVehicleLighting, Variant::BOOL, lights_wrap_selector, "lights");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleLighting, Variant::ARRAY, lights_list, "lights", PROPERTY_HINT_TYPE_STRING,
                "RailVehicleLightListItem");
        BIND_PROPERTY_W_HINT(
                RailVehicleLighting, Variant::INT, light_source, "light", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY_W_HINT(
                RailVehicleLighting, Variant::INT, source_generator_engine, "source/generator", PROPERTY_HINT_ENUM,
                "None,Dumb,WheelsDriven,ElectricSeriesMotor,ElectricInductionMotor,DieselEngine,SteamEngine,"
                "DieselElectric,Main");
        BIND_PROPERTY(RailVehicleLighting, Variant::FLOAT, source_accumulator_max_voltage, "source/accumulator");
        BIND_PROPERTY_W_HINT(
                RailVehicleLighting, Variant::INT, light_alternative_source, "light/alternative", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY(RailVehicleLighting, Variant::FLOAT, light_alternative_max_voltage, "light/alternative");
        BIND_PROPERTY(RailVehicleLighting, Variant::FLOAT, light_alternative_capacity, "light/alternative");
        BIND_PROPERTY_W_HINT(
                RailVehicleLighting, Variant::INT, source_accumulator_recharge_source, "source/accumulator",
                PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY(RailVehicleLighting, Variant::INT, instrument_type);
        ClassDB::bind_method(
                D_METHOD("increase_light_selector_position"), &RailVehicleLighting::increase_light_selector_position);
        ClassDB::bind_method(
                D_METHOD("decrease_light_selector_position"), &RailVehicleLighting::decrease_light_selector_position);
        ClassDB::bind_method(D_METHOD("light", "light", "enabled"), &RailVehicleLighting::light);
        ClassDB::bind_method(D_METHOD("light_is_enabled", "light"), &RailVehicleLighting::light_is_enabled);
        ClassDB::bind_method(D_METHOD("light_switch", "light", "enabled"), &RailVehicleLighting::light_switch);
        ClassDB::bind_method(D_METHOD("headlights_dim", "enabled"), &RailVehicleLighting::headlights_dim);
        ClassDB::bind_method(D_METHOD("get_headlights_dimmed"), &RailVehicleLighting::get_headlights_dimmed);
        ClassDB::bind_method(D_METHOD("get_any_light_enabled"), &RailVehicleLighting::get_any_light_enabled);
        ADD_SIGNAL(MethodInfo(selector_position_changed_signal, PropertyInfo(Variant::INT, "position")));

        ClassDB::bind_method(D_METHOD("get_position"), &RailVehicleLighting::get_position);
        ClassDB::bind_method(D_METHOD("get_power"), &RailVehicleLighting::get_power);
        ClassDB::bind_method(D_METHOD("get_power_source"), &RailVehicleLighting::get_power_source);
        ClassDB::bind_method(
                D_METHOD("get_front_headlight_upper_enabled"), &RailVehicleLighting::get_front_headlight_upper_enabled);
        ClassDB::bind_method(
                D_METHOD("get_front_headlight_left_enabled"), &RailVehicleLighting::get_front_headlight_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_front_headlight_right_enabled"), &RailVehicleLighting::get_front_headlight_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_front_redmarker_left_enabled"), &RailVehicleLighting::get_front_redmarker_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_front_redmarker_right_enabled"), &RailVehicleLighting::get_front_redmarker_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_rear_headlight_upper_enabled"), &RailVehicleLighting::get_rear_headlight_upper_enabled);
        ClassDB::bind_method(
                D_METHOD("get_rear_headlight_left_enabled"), &RailVehicleLighting::get_rear_headlight_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_rear_headlight_right_enabled"), &RailVehicleLighting::get_rear_headlight_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_rear_redmarker_left_enabled"), &RailVehicleLighting::get_rear_redmarker_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_rear_redmarker_right_enabled"), &RailVehicleLighting::get_rear_redmarker_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_active_headlight_upper_enabled"),
                &RailVehicleLighting::get_active_headlight_upper_enabled);
        ClassDB::bind_method(
                D_METHOD("get_active_headlight_left_enabled"), &RailVehicleLighting::get_active_headlight_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_active_headlight_right_enabled"),
                &RailVehicleLighting::get_active_headlight_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_active_redmarker_left_enabled"), &RailVehicleLighting::get_active_redmarker_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_active_redmarker_right_enabled"),
                &RailVehicleLighting::get_active_redmarker_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_opposite_headlight_upper_enabled"),
                &RailVehicleLighting::get_opposite_headlight_upper_enabled);
        ClassDB::bind_method(
                D_METHOD("get_opposite_headlight_left_enabled"),
                &RailVehicleLighting::get_opposite_headlight_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_opposite_headlight_right_enabled"),
                &RailVehicleLighting::get_opposite_headlight_right_enabled);
        ClassDB::bind_method(
                D_METHOD("get_opposite_redmarker_left_enabled"),
                &RailVehicleLighting::get_opposite_redmarker_left_enabled);
        ClassDB::bind_method(
                D_METHOD("get_opposite_redmarker_right_enabled"),
                &RailVehicleLighting::get_opposite_redmarker_right_enabled);
    }

    const char *RailVehicleLighting::selector_position_changed_signal = "selector_position_changed";


    void RailVehicleLighting::_register_commands() {
        register_command("increase_light_selector_position", Callable(this, "increase_light_selector_position"));
        register_command("decrease_light_selector_position", Callable(this, "decrease_light_selector_position"));
        register_command("light", Callable(this, "light"));
        register_command("light_switch", Callable(this, "light_switch"));
        register_command("headlights_dim", Callable(this, "headlights_dim"));
        VehicleComponent::_register_commands();
    }

    void RailVehicleLighting::_unregister_commands() {
        unregister_command("increase_light_selector_position");
        unregister_command("decrease_light_selector_position");
        unregister_command("light");
        unregister_command("light_switch");
        unregister_command("headlights_dim");
        VehicleComponent::_unregister_commands();
    }
} // namespace godot
