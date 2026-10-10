#include "VehicleComponent.hpp"
#include "VehiclePhysicsNode.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <godot_cpp/classes/class_db_singleton.hpp>
#include <godot_cpp/classes/engine.hpp>

namespace godot {
    const char *VehiclePhysicsNode::vehicle_changed_signal = "vehicle_changed";
    StringName &VehiclePhysicsNode::controller_implementation() {
        static StringName implementation;
        return implementation;
    }

    void VehiclePhysicsNode::set_controller_implementation(const StringName &p_class) {
        controller_implementation() = p_class;
    }

    void VehiclePhysicsNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_controller", "controller"), &VehiclePhysicsNode::set_controller);
        ClassDB::bind_method(D_METHOD("get_controller"), &VehiclePhysicsNode::get_controller);
        /* Shown, never stored: a controller built from a source file (a .fiz) would otherwise be
         * embedded in whatever scene holds this node and drift from the file it came from. A
         * vehicle authored as a .tres is referenced by the subclass that loads it. */
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::OBJECT, "controller", PROPERTY_HINT_RESOURCE_TYPE, "VehicleController",
                        PROPERTY_USAGE_EDITOR),
                "set_controller", "get_controller");
        GDVIRTUAL_BIND(_build_controller);

        ClassDB::bind_method(D_METHOD("set_vehicle_id", "vehicle_id"), &VehiclePhysicsNode::set_vehicle_id);
        ClassDB::bind_method(D_METHOD("get_vehicle_id"), &VehiclePhysicsNode::get_vehicle_id);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "vehicle_id"), "set_vehicle_id", "get_vehicle_id");
        ClassDB::bind_method(D_METHOD("set_initial_velocity", "velocity"), &VehiclePhysicsNode::set_initial_velocity);
        ClassDB::bind_method(D_METHOD("get_initial_velocity"), &VehiclePhysicsNode::get_initial_velocity);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "initial_velocity"), "set_initial_velocity", "get_initial_velocity");

        ClassDB::bind_method(D_METHOD("get_vehicle_rid"), &VehiclePhysicsNode::get_vehicle_rid);
        ClassDB::bind_method(D_METHOD("add_component", "component"), &VehiclePhysicsNode::add_component);

        ADD_SIGNAL(MethodInfo(vehicle_changed_signal));
    }

    void VehiclePhysicsNode::_notification(const int p_what) {
        // on entering, not on ready: Godot readies children before their parent, and a
        // component proxy below this node has to find a vehicle already standing
        if (p_what == NOTIFICATION_ENTER_TREE && !vehicle_rid.is_valid()) {
            // with the controller the node was given before it entered, or the one it builds;
            // without either the vehicle still comes up, empty - components can be added to it,
            // or a controller given later
            if (controller.is_null()) {
                GDVIRTUAL_CALL(_build_controller, controller);
            }
            _build();
        }
        if (p_what == NOTIFICATION_PREDELETE) {
            if (VehicleServer *server = VehicleServer::get_instance(); server != nullptr) {
                server->vehicle_free(vehicle_rid);
                server->controller_free(controller_rid);
            }
            vehicle_rid = RID();
            controller_rid = RID();
        }
    }

    /* The vehicle is built here and nowhere else: one owner of the handle, one owner of the
     * controller, both freed with this node. */
    void VehiclePhysicsNode::set_controller(const Ref<VehicleController> &p_controller) {
        controller = p_controller;
        if (is_inside_tree()) {
            _build();
        }
    }

    /* The vehicle and its controller are VehicleServer's; this node creates both once, has the
     * controller configured from the description - or from an empty vehicle of the simulation the
     * extension ships - and binds it, which (re)starts the vehicle. Its handle stays across a
     * rebuild, and everything outside the vehicle layer holds that. */
    void VehiclePhysicsNode::_build() {
        VehicleServer *server = VehicleServer::get_instance();
        ERR_FAIL_NULL(server);
        const Ref<VehicleController> configuration =
                controller.is_valid() ? controller
                                      : Ref<VehicleController>(ClassDBSingleton::get_singleton()->instantiate(
                                                controller_implementation()));
        ERR_FAIL_COND_MSG(configuration.is_null(), "The vehicle's controller could not be made");
        const bool created = !vehicle_rid.is_valid();
        if (created) {
            vehicle_rid = server->vehicle_create();
            controller_rid = server->controller_create();
        }
        // the scenery's values first: the controller takes them when its simulation starts
        server->vehicle_set_name(vehicle_rid, vehicle_id);
        server->vehicle_set_initial_velocity(vehicle_rid, initial_velocity);
        _prepare_vehicle(vehicle_rid);
        // configuring a bound controller restarts the vehicle on it; the first time it is bound
        server->controller_configure(controller_rid, configuration);
        if (created) {
            server->vehicle_bind_controller(vehicle_rid, controller_rid);
        }
        emit_signal(vehicle_changed_signal);
    }

    Ref<VehicleController> VehiclePhysicsNode::get_controller() const {
        return controller;
    }

    RID VehiclePhysicsNode::get_vehicle_rid() const {
        return vehicle_rid;
    }

    void VehiclePhysicsNode::add_component(const Ref<VehicleComponent> &p_component) {
        ERR_FAIL_COND(p_component.is_null());
        const VehicleServer *server = VehicleServer::get_instance();
        const Ref<VehicleController> vehicle =
                server != nullptr ? server->vehicle_get_controller(vehicle_rid) : Ref<VehicleController>();
        ERR_FAIL_COND_MSG(vehicle.is_null(), "VehiclePhysicsNode has no vehicle to add a component to yet.");
        vehicle->add_component(p_component);
    }

    void VehiclePhysicsNode::set_vehicle_id(const String &p_vehicle_id) {
        vehicle_id = p_vehicle_id;
        if (VehicleServer *server = VehicleServer::get_instance(); server != nullptr && vehicle_rid.is_valid()) {
            server->vehicle_set_name(vehicle_rid, vehicle_id);
        }
    }

    String VehiclePhysicsNode::get_vehicle_id() const {
        return vehicle_id;
    }


    void VehiclePhysicsNode::set_initial_velocity(const double p_velocity) {
        initial_velocity = p_velocity;
        if (VehicleServer *server = VehicleServer::get_instance(); server != nullptr && vehicle_rid.is_valid()) {
            server->vehicle_set_initial_velocity(vehicle_rid, initial_velocity);
        }
    }

    double VehiclePhysicsNode::get_initial_velocity() const {
        return initial_velocity;
    }


} // namespace godot
