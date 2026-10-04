#include "scenery/SceneryModelPlacement.hpp"

namespace godot {
    void SceneryModelPlacement::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_data_path", "value"), &SceneryModelPlacement::set_data_path);
        ClassDB::bind_method(D_METHOD("get_data_path"), &SceneryModelPlacement::get_data_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "data_path"), "set_data_path", "get_data_path");
        ClassDB::bind_method(D_METHOD("set_model_filename", "value"), &SceneryModelPlacement::set_model_filename);
        ClassDB::bind_method(D_METHOD("get_model_filename"), &SceneryModelPlacement::get_model_filename);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_filename"), "set_model_filename", "get_model_filename");
        ClassDB::bind_method(D_METHOD("set_skins", "value"), &SceneryModelPlacement::set_skins);
        ClassDB::bind_method(D_METHOD("get_skins"), &SceneryModelPlacement::get_skins);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "skins"), "set_skins", "get_skins");
        ClassDB::bind_method(D_METHOD("set_transform", "value"), &SceneryModelPlacement::set_transform);
        ClassDB::bind_method(D_METHOD("get_transform"), &SceneryModelPlacement::get_transform);
        ADD_PROPERTY(PropertyInfo(Variant::TRANSFORM3D, "transform"), "set_transform", "get_transform");
        ClassDB::bind_method(D_METHOD("set_range_min", "value"), &SceneryModelPlacement::set_range_min);
        ClassDB::bind_method(D_METHOD("get_range_min"), &SceneryModelPlacement::get_range_min);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_min"), "set_range_min", "get_range_min");
        ClassDB::bind_method(D_METHOD("set_range_max", "value"), &SceneryModelPlacement::set_range_max);
        ClassDB::bind_method(D_METHOD("get_range_max"), &SceneryModelPlacement::get_range_max);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_max"), "set_range_max", "get_range_max");
    }

    void SceneryModelPlacement::set_data_path(const String &p_value) {
        data_path = p_value;
    }

    String SceneryModelPlacement::get_data_path() const {
        return data_path;
    }

    void SceneryModelPlacement::set_model_filename(const String &p_value) {
        model_filename = p_value;
    }

    String SceneryModelPlacement::get_model_filename() const {
        return model_filename;
    }

    void SceneryModelPlacement::set_skins(const PackedStringArray &p_value) {
        skins = p_value;
    }

    PackedStringArray SceneryModelPlacement::get_skins() const {
        return skins;
    }

    void SceneryModelPlacement::set_transform(const Transform3D &p_value) {
        transform = p_value;
    }

    Transform3D SceneryModelPlacement::get_transform() const {
        return transform;
    }

    void SceneryModelPlacement::set_range_min(const float p_value) {
        range_min = p_value;
    }

    float SceneryModelPlacement::get_range_min() const {
        return range_min;
    }

    void SceneryModelPlacement::set_range_max(const float p_value) {
        range_max = p_value;
    }

    float SceneryModelPlacement::get_range_max() const {
        return range_max;
    }
} // namespace godot
