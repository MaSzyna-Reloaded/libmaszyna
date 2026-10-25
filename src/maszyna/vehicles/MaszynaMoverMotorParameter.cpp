#include "MaszynaMoverMotorParameter.hpp"
#include "macros.hpp"

namespace godot {
    void MaszynaMoverMotorParameter::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, shunting_up);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, shunting_down);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, voltage_constant_multiplier);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, saturation_current_multiplier);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, initial_voltage_constant_multiplier);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, voltage_constant);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, saturation_current);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::FLOAT, initial_voltage_constant);
        BIND_PROPERTY(MaszynaMoverMotorParameter, Variant::BOOL, auto_switch);
    }
} // namespace godot
