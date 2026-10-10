#include "GenericVehicleComponent.hpp"
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    /* Where the modder's script actually lives: on the proxy node when one authored this
     * component, on the component itself when a script extends it directly. */
    Object *GenericVehicleComponent::script_target() {
        Object *owner = ObjectDB::get_instance(script_owner);
        return owner != nullptr ? owner : static_cast<Object *>(this);
    }

    void GenericVehicleComponent::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_vehicle_state"), &GenericVehicleComponent::get_vehicle_state);
        BIND_VIRTUAL_METHOD(GenericVehicleComponent, _process_component, 2);
        BIND_VIRTUAL_METHOD(GenericVehicleComponent, _get_component_state, 1);
        BIND_VIRTUAL_METHOD(GenericVehicleComponent, _get_component_config, 1);
    }

    void GenericVehicleComponent::set_script_owner(const ObjectID &p_owner) {
        script_owner = p_owner;
    }

    void GenericVehicleComponent::_apply_configuration() {};
    /* A script's own configuration is its own - it does not come from the backend, so there is
     * nothing to guard against here. The script cannot be known at build time (that is what this
     * class is for) and Object::call() is non-const, though the call only reads. */
    void GenericVehicleComponent::_fill_config_dictionary(Dictionary &p_config) const {
        Object *target = const_cast<GenericVehicleComponent *>(this)->script_target();
        p_config.merge(target->call("_get_component_config"), true);
    }
    /* The component's tick is the modder's script: this class exists precisely because that
     * script cannot be known at build time, which is why the calls go by name. */
    void GenericVehicleComponent::_do_process_component(const double p_delta) {
        script_target()->call("_process_component", p_delta);
    }

    /* The script's own default, for a script that does not override it. */
    void GenericVehicleComponent::_process_component(const double p_delta) {}
    Dictionary GenericVehicleComponent::_get_component_state() {
        return Dictionary();
    };
    Dictionary GenericVehicleComponent::_get_component_config() {
        return {};
    };

    /* The script's own keys, asked for when the dump is composed - the same way the
     * configuration above is. Pulling them in the tick instead meant every vehicle carrying a
     * modder's component paid for a dictionary per tick whether or not anything read it. */
    void GenericVehicleComponent::_fill_state_dictionary(Dictionary &p_state) const {
        Object *target = const_cast<GenericVehicleComponent *>(this)->script_target();
        p_state.merge(target->call("_get_component_state"), true);
    }

    Dictionary GenericVehicleComponent::get_vehicle_state() {
        if (train_controller_node != nullptr) {
            return train_controller_node->get_state();
        }
        UtilityFunctions::push_error("GenericVehicleComponent has no train controller node");
        return {};
    }

} // namespace godot
