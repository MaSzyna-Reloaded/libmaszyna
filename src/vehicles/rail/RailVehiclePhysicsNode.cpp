#include "RailVehiclePhysicsNode.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    void RailVehiclePhysicsNode::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_type_name", "type_name"), &RailVehiclePhysicsNode::set_type_name);
        ClassDB::bind_method(D_METHOD("get_type_name"), &RailVehiclePhysicsNode::get_type_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "type_name"), "set_type_name", "get_type_name");
        ClassDB::bind_method(D_METHOD("set_load_name", "load_name"), &RailVehiclePhysicsNode::set_load_name);
        ClassDB::bind_method(D_METHOD("get_load_name"), &RailVehiclePhysicsNode::get_load_name);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "load_name"), "set_load_name", "get_load_name");
        ClassDB::bind_method(D_METHOD("set_load_amount", "load_amount"), &RailVehiclePhysicsNode::set_load_amount);
        ClassDB::bind_method(D_METHOD("get_load_amount"), &RailVehiclePhysicsNode::get_load_amount);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "load_amount"), "set_load_amount", "get_load_amount");
    }

    /* The vehicle becomes a rail one here - this node is what makes it one - and takes its rail
     * values before its controller starts */
    void RailVehiclePhysicsNode::_prepare_vehicle(const RID &p_vehicle) {
        RailVehicleServer *server = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(server);
        server->vehicle_attach(p_vehicle);
        server->vehicle_set_type_name(p_vehicle, type_name);
        server->vehicle_set_load(p_vehicle, load_name, load_amount);
    }

    void RailVehiclePhysicsNode::set_type_name(const String &p_type_name) {
        type_name = p_type_name;
        if (RailVehicleServer *server = RailVehicleServer::get_instance();
            server != nullptr && server->vehicle_is_attached(get_vehicle_rid())) {
            server->vehicle_set_type_name(get_vehicle_rid(), type_name);
        }
    }

    String RailVehiclePhysicsNode::get_type_name() const {
        return type_name;
    }

    void RailVehiclePhysicsNode::set_load_name(const String &p_load_name) {
        load_name = p_load_name;
        if (RailVehicleServer *server = RailVehicleServer::get_instance();
            server != nullptr && server->vehicle_is_attached(get_vehicle_rid())) {
            server->vehicle_set_load(get_vehicle_rid(), load_name, load_amount);
        }
    }

    String RailVehiclePhysicsNode::get_load_name() const {
        return load_name;
    }

    void RailVehiclePhysicsNode::set_load_amount(const double p_load_amount) {
        load_amount = p_load_amount;
        if (RailVehicleServer *server = RailVehicleServer::get_instance();
            server != nullptr && server->vehicle_is_attached(get_vehicle_rid())) {
            server->vehicle_set_load(get_vehicle_rid(), load_name, load_amount);
        }
    }

    double RailVehiclePhysicsNode::get_load_amount() const {
        return load_amount;
    }
} // namespace godot
