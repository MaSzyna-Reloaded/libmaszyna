#include "RoadTrafficVehicleType.hpp"

namespace godot {
    void RoadTrafficVehicleType::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_data_path", "value"), &RoadTrafficVehicleType::set_data_path);
        ClassDB::bind_method(D_METHOD("get_data_path"), &RoadTrafficVehicleType::get_data_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "data_path"), "set_data_path", "get_data_path");
        ClassDB::bind_method(D_METHOD("set_model_filename", "value"), &RoadTrafficVehicleType::set_model_filename);
        ClassDB::bind_method(D_METHOD("get_model_filename"), &RoadTrafficVehicleType::get_model_filename);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_filename"), "set_model_filename", "get_model_filename");
        ClassDB::bind_method(D_METHOD("set_skins", "value"), &RoadTrafficVehicleType::set_skins);
        ClassDB::bind_method(D_METHOD("get_skins"), &RoadTrafficVehicleType::get_skins);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "skins"), "set_skins", "get_skins");
        ClassDB::bind_method(D_METHOD("set_length", "value"), &RoadTrafficVehicleType::set_length);
        ClassDB::bind_method(D_METHOD("get_length"), &RoadTrafficVehicleType::get_length);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "length", PROPERTY_HINT_NONE, "suffix:m"), "set_length", "get_length");
        ClassDB::bind_method(D_METHOD("set_width", "value"), &RoadTrafficVehicleType::set_width);
        ClassDB::bind_method(D_METHOD("get_width"), &RoadTrafficVehicleType::get_width);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "width", PROPERTY_HINT_NONE, "suffix:m"), "set_width", "get_width");
        ClassDB::bind_method(D_METHOD("set_max_velocity", "value"), &RoadTrafficVehicleType::set_max_velocity);
        ClassDB::bind_method(D_METHOD("get_max_velocity"), &RoadTrafficVehicleType::get_max_velocity);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "max_velocity", PROPERTY_HINT_NONE, "suffix:km/h"), "set_max_velocity",
                "get_max_velocity");
        ClassDB::bind_method(D_METHOD("set_weight", "value"), &RoadTrafficVehicleType::set_weight);
        ClassDB::bind_method(D_METHOD("get_weight"), &RoadTrafficVehicleType::get_weight);
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "weight"), "set_weight", "get_weight");
    }

    void RoadTrafficVehicleType::set_data_path(const String &p_value) {
        data_path = p_value;
    }

    String RoadTrafficVehicleType::get_data_path() const {
        return data_path;
    }

    void RoadTrafficVehicleType::set_model_filename(const String &p_value) {
        model_filename = p_value;
    }

    String RoadTrafficVehicleType::get_model_filename() const {
        return model_filename;
    }

    void RoadTrafficVehicleType::set_skins(const PackedStringArray &p_value) {
        skins = p_value;
    }

    PackedStringArray RoadTrafficVehicleType::get_skins() const {
        return skins;
    }

    void RoadTrafficVehicleType::set_length(const double p_value) {
        length = p_value;
    }

    double RoadTrafficVehicleType::get_length() const {
        return length;
    }

    void RoadTrafficVehicleType::set_width(const double p_value) {
        width = p_value;
    }

    double RoadTrafficVehicleType::get_width() const {
        return width;
    }

    void RoadTrafficVehicleType::set_max_velocity(const double p_value) {
        max_velocity = p_value;
    }

    double RoadTrafficVehicleType::get_max_velocity() const {
        return max_velocity;
    }

    void RoadTrafficVehicleType::set_weight(const double p_value) {
        weight = p_value;
    }

    double RoadTrafficVehicleType::get_weight() const {
        return weight;
    }
} // namespace godot
