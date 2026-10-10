#include "RailVehicleInverter.hpp"

namespace godot {
    void RailVehicleInverter::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_active", "active"), &RailVehicleInverter::set_active);
        ClassDB::bind_method(D_METHOD("get_active"), &RailVehicleInverter::get_active);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "active"), "set_active", "get_active");
        ClassDB::bind_method(D_METHOD("set_error", "error"), &RailVehicleInverter::set_error);
        ClassDB::bind_method(D_METHOD("get_error"), &RailVehicleInverter::get_error);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "error"), "set_error", "get_error");
        ClassDB::bind_method(D_METHOD("set_allow", "allow"), &RailVehicleInverter::set_allow);
        ClassDB::bind_method(D_METHOD("get_allow"), &RailVehicleInverter::get_allow);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "allow"), "set_allow", "get_allow");
    }

    void RailVehicleInverter::set_active(const bool p_active) {
        active = p_active;
    }

    bool RailVehicleInverter::get_active() const {
        return active;
    }

    void RailVehicleInverter::set_error(const bool p_error) {
        error = p_error;
    }

    bool RailVehicleInverter::get_error() const {
        return error;
    }

    void RailVehicleInverter::set_allow(const bool p_allow) {
        allow = p_allow;
    }

    bool RailVehicleInverter::get_allow() const {
        return allow;
    }
} // namespace godot
