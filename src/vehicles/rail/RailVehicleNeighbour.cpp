#include "RailVehicleNeighbour.hpp"

namespace godot {
    void RailVehicleNeighbour::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_vehicle_rid", "vehicle_rid"), &RailVehicleNeighbour::set_vehicle_rid);
        ClassDB::bind_method(D_METHOD("get_vehicle_rid"), &RailVehicleNeighbour::get_vehicle_rid);
        ADD_PROPERTY(PropertyInfo(Variant::RID, "vehicle_rid"), "set_vehicle_rid", "get_vehicle_rid");
        ClassDB::bind_method(D_METHOD("set_end", "end"), &RailVehicleNeighbour::set_end);
        ClassDB::bind_method(D_METHOD("get_end"), &RailVehicleNeighbour::get_end);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::INT, "end", PROPERTY_HINT_ENUM, "Front,Rear",
                        PROPERTY_USAGE_DEFAULT | PROPERTY_USAGE_CLASS_IS_ENUM, "RailVehicleController.CouplerEnd"),
                "set_end", "get_end");
        ClassDB::bind_method(D_METHOD("set_distance", "distance"), &RailVehicleNeighbour::set_distance);
        ClassDB::bind_method(D_METHOD("get_distance"), &RailVehicleNeighbour::get_distance);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "distance"), "set_distance", "get_distance");
    }

    void RailVehicleNeighbour::set_vehicle_rid(const RID &p_vehicle_rid) {
        vehicle_rid = p_vehicle_rid;
    }

    RID RailVehicleNeighbour::get_vehicle_rid() const {
        return vehicle_rid;
    }

    void RailVehicleNeighbour::set_end(const RailVehicleController::CouplerEnd p_end) {
        end = p_end;
    }

    RailVehicleController::CouplerEnd RailVehicleNeighbour::get_end() const {
        return end;
    }

    void RailVehicleNeighbour::set_distance(const double p_distance) {
        distance = p_distance;
    }

    double RailVehicleNeighbour::get_distance() const {
        return distance;
    }
} // namespace godot
