#include "RailVehicleMotorParameter.hpp"
#include "macros.hpp"

namespace godot {
    void RailVehicleMotorParameter::_bind_methods() {
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, shunting_up);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, shunting_down);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, voltage_constant_multiplier);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, saturation_current_multiplier);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, initial_voltage_constant_multiplier);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, voltage_constant);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, saturation_current);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::FLOAT, initial_voltage_constant);
        BIND_PROPERTY(RailVehicleMotorParameter, Variant::BOOL, auto_switch);
    }
} // namespace godot
