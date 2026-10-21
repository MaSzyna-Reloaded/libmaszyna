#include "MaszynaMoverEngineConfiguration.hpp"

namespace godot {
    void MaszynaMoverEngineConfiguration::_bind_methods() {
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                MaszynaMoverEngineConfiguration, Variant::ARRAY, motor_param_table, PROPERTY_HINT_TYPE_STRING,
                "MaszynaMoverMotorParameter");
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_0);
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_1);
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_2);
        BIND_ENUM_CONSTANT(EIM_CONTROL_TYPE_3);
        BIND_PROPERTY(MaszynaMoverEngineConfiguration, Variant::BOOL, cntrl_eim_control_additional_zeros, "cntrl");
        BIND_PROPERTY(MaszynaMoverEngineConfiguration, Variant::BOOL, cntrl_eim_control_emergency, "cntrl");
        BIND_PROPERTY_W_HINT(
                MaszynaMoverEngineConfiguration, Variant::INT, cntrl_eim_control_type, "cntrl", PROPERTY_HINT_ENUM,
                "0,1,2,3");
        BIND_PROPERTY(MaszynaMoverEngineConfiguration, Variant::BOOL, cntrl_series_shunt_on_series_position, "cntrl");
        BIND_PROPERTY(MaszynaMoverEngineConfiguration, Variant::BOOL, cntrl_fast_series_circuit, "cntrl");
        BIND_PROPERTY(MaszynaMoverEngineConfiguration, Variant::FLOAT, motor_blowers_speed, "motor_blowers");
    }
} // namespace godot
