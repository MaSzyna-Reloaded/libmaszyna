#include "RailVehicleAppearance.hpp"

namespace godot {
    void RailVehicleAppearance::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_data_path", "value"), &RailVehicleAppearance::set_data_path);
        ClassDB::bind_method(D_METHOD("get_data_path"), &RailVehicleAppearance::get_data_path);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "data_path"), "set_data_path", "get_data_path");
        ClassDB::bind_method(D_METHOD("set_model_filename", "value"), &RailVehicleAppearance::set_model_filename);
        ClassDB::bind_method(D_METHOD("get_model_filename"), &RailVehicleAppearance::get_model_filename);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "model_filename"), "set_model_filename", "get_model_filename");
        ClassDB::bind_method(
                D_METHOD("set_low_poly_model_filename", "value"), &RailVehicleAppearance::set_low_poly_model_filename);
        ClassDB::bind_method(
                D_METHOD("get_low_poly_model_filename"), &RailVehicleAppearance::get_low_poly_model_filename);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "low_poly_model_filename"), "set_low_poly_model_filename",
                "get_low_poly_model_filename");
        ClassDB::bind_method(
                D_METHOD("set_passengers_model_filename", "value"),
                &RailVehicleAppearance::set_passengers_model_filename);
        ClassDB::bind_method(
                D_METHOD("get_passengers_model_filename"), &RailVehicleAppearance::get_passengers_model_filename);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "passengers_model_filename"), "set_passengers_model_filename",
                "get_passengers_model_filename");
        ClassDB::bind_method(
                D_METHOD("set_attachment_model_filenames", "value"),
                &RailVehicleAppearance::set_attachment_model_filenames);
        ClassDB::bind_method(
                D_METHOD("get_attachment_model_filenames"), &RailVehicleAppearance::get_attachment_model_filenames);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "attachment_model_filenames"),
                "set_attachment_model_filenames", "get_attachment_model_filenames");
        ClassDB::bind_method(D_METHOD("set_skins", "value"), &RailVehicleAppearance::set_skins);
        ClassDB::bind_method(D_METHOD("get_skins"), &RailVehicleAppearance::get_skins);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "skins"), "set_skins", "get_skins");
        ClassDB::bind_method(D_METHOD("set_model_transform", "value"), &RailVehicleAppearance::set_model_transform);
        ClassDB::bind_method(D_METHOD("get_model_transform"), &RailVehicleAppearance::get_model_transform);
        ADD_PROPERTY(
                PropertyInfo(Variant::TRANSFORM3D, "model_transform"), "set_model_transform", "get_model_transform");
        ClassDB::bind_method(D_METHOD("set_front_bogie", "value"), &RailVehicleAppearance::set_front_bogie);
        ClassDB::bind_method(D_METHOD("get_front_bogie"), &RailVehicleAppearance::get_front_bogie);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "front_bogie"), "set_front_bogie", "get_front_bogie");
        ClassDB::bind_method(D_METHOD("set_rear_bogie", "value"), &RailVehicleAppearance::set_rear_bogie);
        ClassDB::bind_method(D_METHOD("get_rear_bogie"), &RailVehicleAppearance::get_rear_bogie);
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "rear_bogie"), "set_rear_bogie", "get_rear_bogie");
        ClassDB::bind_method(
                D_METHOD("set_front_rolling_wheels", "value"), &RailVehicleAppearance::set_front_rolling_wheels);
        ClassDB::bind_method(D_METHOD("get_front_rolling_wheels"), &RailVehicleAppearance::get_front_rolling_wheels);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "front_rolling_wheels"), "set_front_rolling_wheels",
                "get_front_rolling_wheels");
        ClassDB::bind_method(D_METHOD("set_powered_wheels", "value"), &RailVehicleAppearance::set_powered_wheels);
        ClassDB::bind_method(D_METHOD("get_powered_wheels"), &RailVehicleAppearance::get_powered_wheels);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "powered_wheels"), "set_powered_wheels",
                "get_powered_wheels");
        ClassDB::bind_method(
                D_METHOD("set_rear_rolling_wheels", "value"), &RailVehicleAppearance::set_rear_rolling_wheels);
        ClassDB::bind_method(D_METHOD("get_rear_rolling_wheels"), &RailVehicleAppearance::get_rear_rolling_wheels);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "rear_rolling_wheels"), "set_rear_rolling_wheels",
                "get_rear_rolling_wheels");
        ClassDB::bind_method(
                D_METHOD("set_pantograph_front_arms", "value"), &RailVehicleAppearance::set_pantograph_front_arms);
        ClassDB::bind_method(D_METHOD("get_pantograph_front_arms"), &RailVehicleAppearance::get_pantograph_front_arms);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "pantograph_front_arms"), "set_pantograph_front_arms",
                "get_pantograph_front_arms");
        ClassDB::bind_method(
                D_METHOD("set_pantograph_rear_arms", "value"), &RailVehicleAppearance::set_pantograph_rear_arms);
        ClassDB::bind_method(D_METHOD("get_pantograph_rear_arms"), &RailVehicleAppearance::get_pantograph_rear_arms);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "pantograph_rear_arms"), "set_pantograph_rear_arms",
                "get_pantograph_rear_arms");
        ClassDB::bind_method(D_METHOD("set_wiper_arms", "value"), &RailVehicleAppearance::set_wiper_arms);
        ClassDB::bind_method(D_METHOD("get_wiper_arms"), &RailVehicleAppearance::get_wiper_arms);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "wiper_arms"), "set_wiper_arms", "get_wiper_arms");
        ClassDB::bind_method(D_METHOD("set_mirrors", "value"), &RailVehicleAppearance::set_mirrors);
        ClassDB::bind_method(D_METHOD("get_mirrors"), &RailVehicleAppearance::get_mirrors);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "mirrors"), "set_mirrors", "get_mirrors");
        ClassDB::bind_method(D_METHOD("set_doors", "value"), &RailVehicleAppearance::set_doors);
        ClassDB::bind_method(D_METHOD("get_doors"), &RailVehicleAppearance::get_doors);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "doors"), "set_doors", "get_doors");
        ClassDB::bind_method(
                D_METHOD("set_pantograph_factors", "value"), &RailVehicleAppearance::set_pantograph_factors);
        ClassDB::bind_method(D_METHOD("get_pantograph_factors"), &RailVehicleAppearance::get_pantograph_factors);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "pantograph_factors"), "set_pantograph_factors",
                "get_pantograph_factors");
        ClassDB::bind_method(D_METHOD("set_pendulums", "value"), &RailVehicleAppearance::set_pendulums);
        ClassDB::bind_method(D_METHOD("get_pendulums"), &RailVehicleAppearance::get_pendulums);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "pendulums"), "set_pendulums", "get_pendulums");
        ClassDB::bind_method(
                D_METHOD("set_pendulum_amplitude", "value"), &RailVehicleAppearance::set_pendulum_amplitude);
        ClassDB::bind_method(D_METHOD("get_pendulum_amplitude"), &RailVehicleAppearance::get_pendulum_amplitude);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "pendulum_amplitude"), "set_pendulum_amplitude", "get_pendulum_amplitude");
        ClassDB::bind_method(D_METHOD("set_door_steps", "value"), &RailVehicleAppearance::set_door_steps);
        ClassDB::bind_method(D_METHOD("get_door_steps"), &RailVehicleAppearance::get_door_steps);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "door_steps"), "set_door_steps", "get_door_steps");
        ClassDB::bind_method(
                D_METHOD("set_head_display_submodel", "value"), &RailVehicleAppearance::set_head_display_submodel);
        ClassDB::bind_method(D_METHOD("get_head_display_submodel"), &RailVehicleAppearance::get_head_display_submodel);
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "head_display_submodel"), "set_head_display_submodel",
                "get_head_display_submodel");
        ClassDB::bind_method(
                D_METHOD("set_low_poly_emission_energy", "value"),
                &RailVehicleAppearance::set_low_poly_emission_energy);
        ClassDB::bind_method(
                D_METHOD("get_low_poly_emission_energy"), &RailVehicleAppearance::get_low_poly_emission_energy);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "low_poly_emission_energy"), "set_low_poly_emission_energy",
                "get_low_poly_emission_energy");
        ClassDB::bind_method(
                D_METHOD("set_low_poly_emission_fade_time", "value"),
                &RailVehicleAppearance::set_low_poly_emission_fade_time);
        ClassDB::bind_method(
                D_METHOD("get_low_poly_emission_fade_time"), &RailVehicleAppearance::get_low_poly_emission_fade_time);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "low_poly_emission_fade_time"), "set_low_poly_emission_fade_time",
                "get_low_poly_emission_fade_time");
        ClassDB::bind_method(D_METHOD("set_joint_cabs", "value"), &RailVehicleAppearance::set_joint_cabs);
        ClassDB::bind_method(D_METHOD("get_joint_cabs"), &RailVehicleAppearance::get_joint_cabs);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "joint_cabs"), "set_joint_cabs", "get_joint_cabs");
    }

    void RailVehicleAppearance::set_data_path(const String &p_value) {
        data_path = p_value;
    }

    String RailVehicleAppearance::get_data_path() const {
        return data_path;
    }

    void RailVehicleAppearance::set_model_filename(const String &p_value) {
        model_filename = p_value;
    }

    String RailVehicleAppearance::get_model_filename() const {
        return model_filename;
    }

    void RailVehicleAppearance::set_low_poly_model_filename(const String &p_value) {
        low_poly_model_filename = p_value;
    }

    String RailVehicleAppearance::get_low_poly_model_filename() const {
        return low_poly_model_filename;
    }

    void RailVehicleAppearance::set_passengers_model_filename(const String &p_value) {
        passengers_model_filename = p_value;
    }

    String RailVehicleAppearance::get_passengers_model_filename() const {
        return passengers_model_filename;
    }

    void RailVehicleAppearance::set_attachment_model_filenames(const PackedStringArray &p_value) {
        attachment_model_filenames = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_attachment_model_filenames() const {
        return attachment_model_filenames;
    }

    void RailVehicleAppearance::set_skins(const PackedStringArray &p_value) {
        skins = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_skins() const {
        return skins;
    }

    void RailVehicleAppearance::set_model_transform(const Transform3D &p_value) {
        model_transform = p_value;
    }

    Transform3D RailVehicleAppearance::get_model_transform() const {
        return model_transform;
    }

    void RailVehicleAppearance::set_front_bogie(const String &p_value) {
        front_bogie = p_value;
    }

    String RailVehicleAppearance::get_front_bogie() const {
        return front_bogie;
    }

    void RailVehicleAppearance::set_rear_bogie(const String &p_value) {
        rear_bogie = p_value;
    }

    String RailVehicleAppearance::get_rear_bogie() const {
        return rear_bogie;
    }

    void RailVehicleAppearance::set_front_rolling_wheels(const PackedStringArray &p_value) {
        front_rolling_wheels = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_front_rolling_wheels() const {
        return front_rolling_wheels;
    }

    void RailVehicleAppearance::set_powered_wheels(const PackedStringArray &p_value) {
        powered_wheels = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_powered_wheels() const {
        return powered_wheels;
    }

    void RailVehicleAppearance::set_rear_rolling_wheels(const PackedStringArray &p_value) {
        rear_rolling_wheels = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_rear_rolling_wheels() const {
        return rear_rolling_wheels;
    }

    void RailVehicleAppearance::set_pantograph_front_arms(const PackedStringArray &p_value) {
        pantograph_front_arms = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_pantograph_front_arms() const {
        return pantograph_front_arms;
    }

    void RailVehicleAppearance::set_pantograph_rear_arms(const PackedStringArray &p_value) {
        pantograph_rear_arms = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_pantograph_rear_arms() const {
        return pantograph_rear_arms;
    }

    void RailVehicleAppearance::set_wiper_arms(const PackedStringArray &p_value) {
        wiper_arms = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_wiper_arms() const {
        return wiper_arms;
    }

    void RailVehicleAppearance::set_mirrors(const PackedStringArray &p_value) {
        mirrors = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_mirrors() const {
        return mirrors;
    }

    void RailVehicleAppearance::set_doors(const PackedStringArray &p_value) {
        doors = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_doors() const {
        return doors;
    }

    void RailVehicleAppearance::set_pantograph_factors(const PackedFloat64Array &p_value) {
        pantograph_factors = p_value;
    }

    PackedFloat64Array RailVehicleAppearance::get_pantograph_factors() const {
        return pantograph_factors;
    }

    void RailVehicleAppearance::set_pendulums(const PackedStringArray &p_value) {
        pendulums = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_pendulums() const {
        return pendulums;
    }

    void RailVehicleAppearance::set_pendulum_amplitude(const double p_value) {
        pendulum_amplitude = p_value;
    }

    double RailVehicleAppearance::get_pendulum_amplitude() const {
        return pendulum_amplitude;
    }

    void RailVehicleAppearance::set_door_steps(const PackedStringArray &p_value) {
        door_steps = p_value;
    }

    PackedStringArray RailVehicleAppearance::get_door_steps() const {
        return door_steps;
    }

    void RailVehicleAppearance::set_head_display_submodel(const String &p_value) {
        head_display_submodel = p_value;
    }

    String RailVehicleAppearance::get_head_display_submodel() const {
        return head_display_submodel;
    }


    void RailVehicleAppearance::set_low_poly_emission_energy(const double p_value) {
        low_poly_emission_energy = p_value;
    }

    double RailVehicleAppearance::get_low_poly_emission_energy() const {
        return low_poly_emission_energy;
    }

    void RailVehicleAppearance::set_low_poly_emission_fade_time(const double p_value) {
        low_poly_emission_fade_time = p_value;
    }

    double RailVehicleAppearance::get_low_poly_emission_fade_time() const {
        return low_poly_emission_fade_time;
    }

    void RailVehicleAppearance::set_joint_cabs(const bool p_value) {
        joint_cabs = p_value;
    }

    bool RailVehicleAppearance::get_joint_cabs() const {
        return joint_cabs;
    }
} // namespace godot
