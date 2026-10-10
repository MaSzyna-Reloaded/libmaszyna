#include "SignalAspect.hpp"

namespace godot {
    void SignalAspect::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_lights", "lights"), &SignalAspect::set_lights);
        ClassDB::bind_method(D_METHOD("get_lights"), &SignalAspect::get_lights);
        ClassDB::bind_method(D_METHOD("set_on_times", "times"), &SignalAspect::set_on_times);
        ClassDB::bind_method(D_METHOD("get_on_times"), &SignalAspect::get_on_times);
        ClassDB::bind_method(D_METHOD("set_off_times", "times"), &SignalAspect::set_off_times);
        ClassDB::bind_method(D_METHOD("get_off_times"), &SignalAspect::get_off_times);
        ClassDB::bind_method(D_METHOD("set_phases", "phases"), &SignalAspect::set_phases);
        ClassDB::bind_method(D_METHOD("get_phases"), &SignalAspect::get_phases);

        ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "lights"), "set_lights", "get_lights");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "on_times"), "set_on_times", "get_on_times");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "off_times"), "set_off_times", "get_off_times");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "phases"), "set_phases", "get_phases");

        BIND_ENUM_CONSTANT(LIGHT_OFF);
        BIND_ENUM_CONSTANT(LIGHT_ON);
        BIND_ENUM_CONSTANT(LIGHT_BLINK);
        BIND_ENUM_CONSTANT(LIGHT_KEEP);
    }

    void SignalAspect::set_lights(const PackedInt32Array &p_lights) {
        lights = p_lights;
    }

    PackedInt32Array SignalAspect::get_lights() const {
        return lights;
    }

    void SignalAspect::set_on_times(const PackedFloat32Array &p_times) {
        on_times = p_times;
    }

    PackedFloat32Array SignalAspect::get_on_times() const {
        return on_times;
    }

    void SignalAspect::set_off_times(const PackedFloat32Array &p_times) {
        off_times = p_times;
    }

    PackedFloat32Array SignalAspect::get_off_times() const {
        return off_times;
    }

    void SignalAspect::set_phases(const PackedFloat32Array &p_phases) {
        phases = p_phases;
    }

    PackedFloat32Array SignalAspect::get_phases() const {
        return phases;
    }
} // namespace godot
