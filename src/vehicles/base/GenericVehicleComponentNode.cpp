#include "GenericVehicleComponentNode.hpp"
#include "VehiclePhysicsNode.hpp"
#include "VehicleServer.hpp"
#include <godot_cpp/classes/engine.hpp>

namespace godot {
    void GenericVehicleComponentNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_component"), &GenericVehicleComponentNode::get_component);
        ClassDB::bind_method(
                D_METHOD("register_command", "command", "callable"), &GenericVehicleComponentNode::register_command);
        ClassDB::bind_method(
                D_METHOD("unregister_command", "command"), &GenericVehicleComponentNode::unregister_command);
        ClassDB::bind_method(D_METHOD("get_controller"), &GenericVehicleComponentNode::get_controller);
        ClassDB::bind_method(D_METHOD("log_debug", "line"), &GenericVehicleComponentNode::log_debug);
        ClassDB::bind_method(D_METHOD("log_warning", "line"), &GenericVehicleComponentNode::log_warning);
    }

    void GenericVehicleComponentNode::_notification(const int p_what) {
        if (Engine::get_singleton()->is_editor_hint()) {
            return;
        }
        switch (p_what) {
            case NOTIFICATION_ENTER_TREE: {
                // the vehicle is whatever this node sits under - that is the whole point of it
                Node *parent = get_parent();
                VehiclePhysicsNode *vehicle_node = nullptr;
                while (parent != nullptr && vehicle_node == nullptr) {
                    vehicle_node = Object::cast_to<VehiclePhysicsNode>(parent);
                    parent = parent->get_parent();
                }
                if (vehicle_node == nullptr) {
                    ERR_PRINT("GenericVehicleComponentNode has no VehiclePhysicsNode above it.");
                    return;
                }
                component.instantiate();
                component->set_script_owner(ObjectID(get_instance_id()));
                vehicle_node->add_component(component);
                vehicle = vehicle_node->get_vehicle_rid();
            } break;
            case NOTIFICATION_EXIT_TREE: {
                // out of the vehicle, not only detached: a component left in it is still ticked
                // and read, and calls back into this node after it is freed
                const VehicleServer *server = VehicleServer::get_instance();
                const Ref<VehicleController> controller =
                        server != nullptr ? server->vehicle_get_controller(vehicle) : Ref<VehicleController>();
                if (controller.is_valid() && component.is_valid()) {
                    controller->remove_component(component);
                }
                component.unref();
                vehicle = RID();
            } break;
            default:;
        }
    }

    Ref<GenericVehicleComponent> GenericVehicleComponentNode::get_component() const {
        return component;
    }

    void GenericVehicleComponentNode::register_command(const String &p_command, const Callable &p_callback) {
        ERR_FAIL_COND(component.is_null());
        component->register_command(p_command, p_callback);
    }

    void GenericVehicleComponentNode::unregister_command(const String &p_command) {
        ERR_FAIL_COND(component.is_null());
        component->unregister_command(p_command);
    }

    Ref<VehicleController> GenericVehicleComponentNode::get_controller() const {
        return component.is_valid() ? component->get_controller() : Ref<VehicleController>();
    }

    void GenericVehicleComponentNode::log_debug(const String &p_line) {
        ERR_FAIL_COND(component.is_null());
        component->log_debug(p_line);
    }

    void GenericVehicleComponentNode::log_warning(const String &p_line) {
        ERR_FAIL_COND(component.is_null());
        component->log_warning(p_line);
    }
} // namespace godot
