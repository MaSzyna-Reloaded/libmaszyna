#include "VehicleComponent.hpp"
#include "VehicleController.hpp"
#include "logging/GameLogger.hpp"
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void VehicleComponent::_bind_methods() {
        ClassDB::bind_method(D_METHOD("emit_config_changed_signal"), &VehicleComponent::emit_config_changed_signal);
        ClassDB::bind_method(D_METHOD("mark_dirty"), &VehicleComponent::mark_dirty);
        ClassDB::bind_method(D_METHOD("register_command", "command", "callable"), &VehicleComponent::register_command);
        ClassDB::bind_method(D_METHOD("unregister_command", "command"), &VehicleComponent::unregister_command);
        ClassDB::bind_method(D_METHOD("apply_config"), &VehicleComponent::apply_config);
        ClassDB::bind_method(D_METHOD("set_component_tag", "tag"), &VehicleComponent::set_component_tag);
        ClassDB::bind_method(D_METHOD("get_component_tag"), &VehicleComponent::get_component_tag);
        ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "component_tag"), "set_component_tag", "get_component_tag");
        ClassDB::bind_method(D_METHOD("get_controller"), &VehicleComponent::get_controller);
        ClassDB::bind_method(D_METHOD("get_state"), &VehicleComponent::get_state);
        ClassDB::bind_method(D_METHOD("get_config"), &VehicleComponent::get_config);
        ClassDB::bind_method(
                D_METHOD("send_command", "command", "p1", "p2"), &VehicleComponent::send_command, DEFVAL(Variant()),
                DEFVAL(Variant()));
        ClassDB::bind_method(D_METHOD("log", "loglevel", "line"), &VehicleComponent::log);
        ClassDB::bind_method(D_METHOD("log_debug", "line"), &VehicleComponent::log_debug);
        ClassDB::bind_method(D_METHOD("log_info", "line"), &VehicleComponent::log_info);
        ClassDB::bind_method(D_METHOD("log_warning", "line"), &VehicleComponent::log_warning);
        ClassDB::bind_method(D_METHOD("log_error", "line"), &VehicleComponent::log_error);

        ClassDB::bind_method(D_METHOD("set_enabled"), &VehicleComponent::set_enabled);
        ClassDB::bind_method(D_METHOD("get_enabled"), &VehicleComponent::get_enabled);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "enabled"), "set_enabled", "get_enabled");

        ADD_SIGNAL(MethodInfo("config_changed"));
        ADD_SIGNAL(MethodInfo("enable_changed", PropertyInfo(Variant::BOOL, "enabled")));
        ADD_SIGNAL(MethodInfo("component_enabled"));
        ADD_SIGNAL(MethodInfo("component_disabled"));
    }

    int VehicleComponent::get_component_type() const {
        return VehicleComponentType::COMPONENT_NONE;
    }

    void VehicleComponent::_fill_state_dictionary(Dictionary &p_state) const {}

    void VehicleComponent::_register_commands() {};
    void VehicleComponent::_unregister_commands() {};

    void VehicleComponent::attach_implementation(const ObjectID &p_implementation) {
        if (implementation == p_implementation) {
            return;
        }
        implementation = p_implementation;
        _implementation_changed();
    }

    ObjectID VehicleComponent::get_implementation() const {
        return implementation;
    }

    void VehicleComponent::set_component_tag(const StringName &p_tag) {
        component_tag = p_tag;
    }

    StringName VehicleComponent::get_component_tag() const {
        return component_tag;
    }

    Ref<VehicleController> VehicleComponent::get_controller() const {
        return Ref<VehicleController>(train_controller_node);
    }

    void VehicleComponent::attach(VehicleController *p_controller) {
        train_controller_node = p_controller;
        if (train_controller_node == nullptr) {
            return;
        }
        train_controller_node->register_component(this);
        if (enabled) {
            _register_commands();
            _commands_registered = true;
        }
    }

    void VehicleComponent::detach() {
        if (_commands_registered) {
            _unregister_commands();
            _commands_registered = false;
        }
        if (train_controller_node != nullptr) {
            train_controller_node->unregister_component(this);
        }
        train_controller_node = nullptr;
    }

    void VehicleComponent::log(const GameLog::LogLevel p_level, const String &p_line) {
        if (train_controller_node == nullptr) {
            return;
        }
        if (GameLog *game_log = GameLog::get_instance(); game_log != nullptr) {
            game_log->get_logger(GameLog::GAME_LOGGER)
                    ->log(p_level, vformat(String("%s: %s"), train_controller_node->get_vehicle_id(), p_line));
        }
    }
    void VehicleComponent::log_debug(const String &p_line) {
        log(GameLog::LogLevel::DEBUG, p_line);
    }

    void VehicleComponent::log_info(const String &p_line) {
        log(GameLog::LogLevel::INFO, p_line);
    }

    void VehicleComponent::log_warning(const String &p_line) {
        log(GameLog::LogLevel::WARNING, p_line);
    }

    void VehicleComponent::log_error(const String &p_line) {
        log(GameLog::LogLevel::ERROR, p_line);
    }

    void VehicleComponent::register_command(const String &p_command, const Callable &p_callback) {
        ERR_FAIL_NULL_MSG(train_controller_node, "A component registers commands once it has a vehicle.");
        train_controller_node->register_command(p_command, p_callback);
    }

    void VehicleComponent::unregister_command(const String &p_command) {
        ERR_FAIL_NULL(train_controller_node);
        train_controller_node->unregister_command(p_command);
    }

    void VehicleComponent::emit_config_changed_signal() {
        emit_signal("config_changed");
    }

    void VehicleComponent::mark_dirty() {
        dirty = true;
    }

    void VehicleComponent::process(const double p_delta) {
        if (dirty) {
            apply_config();
        }

        if (enabled) {
            _do_process_component(p_delta);
        }

        if (enabled_changed) {
            enabled_changed = false;
            if (enabled && !_commands_registered) {
                log_debug("Registering commands for component " + get_class());
                _register_commands();
                _commands_registered = true;
            } else if (!enabled && _commands_registered) {
                log_debug("Unregistering commands for component " + get_class());
                _unregister_commands();
                _commands_registered = false;
            }
            emit_signal("enable_changed", enabled);
            emit_signal(enabled ? "component_enabled" : "component_disabled");
        }
    }

    void VehicleComponent::_do_process_component(const double p_delta) {}

    void VehicleComponent::_fill_config_dictionary(Dictionary &p_config) const {}

    Dictionary VehicleComponent::get_config() {
        Dictionary result;
        _fill_config_dictionary(result);
        return result;
    }
    bool VehicleComponent::is_simulation_ready() const {
        return train_controller_node != nullptr && train_controller_node->is_simulation_ready();
    }

    void VehicleComponent::_apply_configuration() {};

    void VehicleComponent::apply_vehicle_config() {}

    /* Writing the component's configuration into the backend and saying so. A component that is
     * not in a vehicle yet, or whose vehicle has not started its backend yet, simply has nowhere
     * to write - it is configured before it is attached, and initialize() applies all of it. */
    void VehicleComponent::apply_config() {
        if (train_controller_node == nullptr) {
            return;
        }
        _apply_configuration();
        // what changed is in the backend now - a change before it (the description copied with its
        // properties) leaves nothing to apply again on the next step
        dirty = false;
        train_controller_node->emit_config_changed();
    }

    /// The dump of this component alone. Nothing is stored and nothing is computed until asked:
    /// the live values are this component's own typed properties, read straight from the backend.
    Dictionary VehicleComponent::get_state() {
        Dictionary result;
        _fill_state_dictionary(result);
        return result;
    }

    void VehicleComponent::set_enabled(const bool p_value) {
        enabled_changed = (enabled != p_value);
        enabled = p_value;
        dirty = true;
    }

    bool VehicleComponent::get_enabled() {
        return enabled;
    }

    Variant VehicleComponent::send_command(const String &p_command, const Variant &p_p1, const Variant &p_p2) {
        if (train_controller_node == nullptr) {
            return {};
        }
        return train_controller_node->send_command(p_command, p_p1, p_p2);
    }

} // namespace godot
