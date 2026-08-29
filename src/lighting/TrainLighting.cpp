#include "TrainLighting.hpp"
namespace godot {
    const char *TrainLighting::selector_position_changed_signal = "selector_position_changed";

    void TrainLighting::_bind_methods() {
        BIND_PROPERTY(
                Variant::COLOR, "head_light_color", "head_light/color", &TrainLighting::set_head_light_color,
                &TrainLighting::get_head_light_color, "color");
        BIND_PROPERTY(
                Variant::FLOAT, "head_light_dimmed_multiplier", "head_light/dimmed_multiplier",
                &TrainLighting::set_dimming_multiplier, &TrainLighting::get_dimming_multiplier, "multiplier");
        BIND_PROPERTY(
                Variant::FLOAT, "head_light_normal_multiplier", "head_light/normal_multiplier",
                &TrainLighting::set_normal_multiplier, &TrainLighting::get_normal_multiplier, "multiplier");
        BIND_PROPERTY(
                Variant::FLOAT, "high_beam_dimmed_multiplier", "head_light/high_beam/dimmed_multiplier",
                &TrainLighting::set_high_beam_dimmed_multiplier, &TrainLighting::get_high_beam_dimmed_multiplier,
                "multiplier");
        BIND_PROPERTY(
                Variant::FLOAT, "high_beam_normal_multiplier", "head_light/high_beam/normal_multiplier",
                &TrainLighting::set_high_beam_multiplier, &TrainLighting::get_high_beam_multiplier, "multiplier");
        BIND_PROPERTY(
                Variant::INT, "lights_default_selector_position", "lights/default_selector_position",
                &TrainLighting::set_default_selector_position, &TrainLighting::get_default_selector_position,
                "default_selector_position");
        BIND_PROPERTY(
                Variant::INT, "lights_selector_position", "lights/selector_position",
                &TrainLighting::set_selector_position, &TrainLighting::get_selector_position, "selector_position");
        BIND_PROPERTY(
                Variant::BOOL, "wrap_light_selector", "lights/wrap_selector", &TrainLighting::set_wrap_light_selector,
                &TrainLighting::get_wrap_light_selector, "wrap_selector");
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                Variant::ARRAY, "light_position_list", "lights/list", &TrainLighting::set_light_position_list,
                &TrainLighting::get_light_position_list, "light_position_list", PROPERTY_HINT_TYPE_STRING,
                "LightListItem");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "light_source", "light/source", &TrainLighting::set_light_source,
                &TrainLighting::get_light_source, "source", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "generator_engine", "source/generator/engine", &TrainLighting::set_generator_engine,
                &TrainLighting::get_generator_engine, "generator_engine", PROPERTY_HINT_ENUM,
                "None,Dumb,WheelsDriven,ElectricSeriesMotor,ElectricInductionMotor,DieselEngine,SteamEngine,"
                "DieselElectric,Main");
        BIND_PROPERTY(
                Variant::FLOAT, "max_accumulator_voltage", "source/accumulator/max_voltage",
                &TrainLighting::set_max_accumulator_voltage, &TrainLighting::get_max_accumulator_voltage,
                "max_voltage");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "alternative_light_source", "light/alternative/source",
                &TrainLighting::set_alternative_light_source, &TrainLighting::get_alternative_light_source, "source",
                PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY(
                Variant::FLOAT, "alternative_max_voltage", "light/alternative/max_voltage",
                &TrainLighting::set_alternative_max_voltage, &TrainLighting::get_alternative_max_voltage,
                "max_voltage");
        BIND_PROPERTY(
                Variant::FLOAT, "alternative_light_capacity", "light/alternative/capacity",
                &TrainLighting::set_alternative_light_capacity, &TrainLighting::get_alternative_light_capacity,
                "capacity");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "accumulator_recharge_source", "source/accumulator/recharge_source",
                &TrainLighting::set_accumulator_recharge_source, &TrainLighting::get_accumulator_recharge_source,
                "recharge_source", PROPERTY_HINT_ENUM,
                "NotDefined,InternalSource,Transducer,Generator,Accumulator,CurrentCollector,PowerCable,Heater,Main");
        BIND_PROPERTY(
                Variant::INT, "instrument_type", "instrument_type", &TrainLighting::set_instrument_light_type,
                &TrainLighting::get_instrument_light_type, "instrument_type");
        ClassDB::bind_method(
                D_METHOD("increase_light_selector_position"), &TrainLighting::increase_light_selector_position);
        ClassDB::bind_method(
                D_METHOD("decrease_light_selector_position"), &TrainLighting::decrease_light_selector_position);
        ADD_SIGNAL(MethodInfo(selector_position_changed_signal, PropertyInfo(Variant::INT, "position")));
    }

    void TrainLighting::_do_update_internal_mover(TMoverParameters *p_mover) {
        ASSERT_MOVER(p_mover);
        TrainPart::_do_update_internal_mover(p_mover);
        p_mover->LightsPosNo =
                static_cast<int>(light_position_list.size()); // To fix narrowing conversion from int64_t to int
        p_mover->LightsWrap = wrap_light_selector;
        p_mover->LightsDefPos = default_selector_position;
        p_mover->LightPowerSource.SourceType = train_controller_node->power_source_map.at(light_source);
        p_mover->AlterLightPowerSource.SourceType =
                train_controller_node->power_source_map.at(alternative_light_source);
        p_mover->LightsPos = selector_position;
    }

    void TrainLighting::_do_fetch_state_from_mover(TMoverParameters *p_mover, Dictionary &p_state) {
        ASSERT_MOVER(p_mover);
        p_state["light_position"] = p_mover->LightsPosNo;
        p_state["light_power"] = p_mover->LightPower;
        p_state["power_source"] = train_controller_node->tpower_source_map.at(p_mover->LightPowerSource.SourceType);
    }

    void TrainLighting::_do_fetch_config_from_mover(TMoverParameters *p_mover, Dictionary &p_config) {
        TrainPart::_do_fetch_config_from_mover(p_mover, p_config);
    }

    void TrainLighting::_register_commands() {
        register_command("increase_light_selector_position", Callable(this, "increase_light_selector_position"));
        register_command("decrease_light_selector_position", Callable(this, "decrease_light_selector_position"));
        TrainPart::_register_commands();
    }

    void TrainLighting::_unregister_commands() {
        unregister_command("increase_light_selector_position", Callable(this, "increase_light_selector_position"));
        unregister_command("decrease_light_selector_position", Callable(this, "decrease_light_selector_position"));
        TrainPart::_unregister_commands();
    }

    void TrainLighting::increase_light_selector_position() {
        if ((selector_position + 1) < light_position_list.size()) {
            selector_position++;
        }
    }
    void TrainLighting::decrease_light_selector_position() {
        if ((selector_position + 1) > light_position_list.size()) {
            selector_position--;
        }
    }

} // namespace godot
