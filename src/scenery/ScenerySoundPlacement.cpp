#include "scenery/ScenerySoundPlacement.hpp"

namespace godot {
    void ScenerySoundPlacement::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_stream", "value"), &ScenerySoundPlacement::set_stream);
        ClassDB::bind_method(D_METHOD("get_stream"), &ScenerySoundPlacement::get_stream);
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "stream", PROPERTY_HINT_RESOURCE_TYPE, "AudioStream"), "set_stream",
                "get_stream");
        ClassDB::bind_method(D_METHOD("set_position", "value"), &ScenerySoundPlacement::set_position);
        ClassDB::bind_method(D_METHOD("get_position"), &ScenerySoundPlacement::get_position);
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
        ClassDB::bind_method(D_METHOD("set_reach", "value"), &ScenerySoundPlacement::set_reach);
        ClassDB::bind_method(D_METHOD("get_reach"), &ScenerySoundPlacement::get_reach);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "reach"), "set_reach", "get_reach");
        ClassDB::bind_method(D_METHOD("set_volume_db", "value"), &ScenerySoundPlacement::set_volume_db);
        ClassDB::bind_method(D_METHOD("get_volume_db"), &ScenerySoundPlacement::get_volume_db);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "volume_db"), "set_volume_db", "get_volume_db");
    }

    void ScenerySoundPlacement::set_stream(const Ref<AudioStream> &p_value) {
        stream = p_value;
    }

    Ref<AudioStream> ScenerySoundPlacement::get_stream() const {
        return stream;
    }

    void ScenerySoundPlacement::set_position(const Vector3 &p_value) {
        position = p_value;
    }

    Vector3 ScenerySoundPlacement::get_position() const {
        return position;
    }

    void ScenerySoundPlacement::set_reach(const float p_value) {
        reach = p_value;
    }

    float ScenerySoundPlacement::get_reach() const {
        return reach;
    }

    void ScenerySoundPlacement::set_volume_db(const float p_value) {
        volume_db = p_value;
    }

    float ScenerySoundPlacement::get_volume_db() const {
        return volume_db;
    }
} // namespace godot
