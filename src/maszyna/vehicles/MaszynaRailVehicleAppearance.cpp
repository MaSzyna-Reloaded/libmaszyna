#include "MaszynaRailVehicleAppearance.hpp"

namespace godot {
    void MaszynaRailVehicleAppearance::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_pantograph_front_arms", "value"),
                &MaszynaRailVehicleAppearance::set_pantograph_front_arms);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_front_arms"), &MaszynaRailVehicleAppearance::get_pantograph_front_arms);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "pantograph_front_arms"), "set_pantograph_front_arms",
                "get_pantograph_front_arms");
        ClassDB::bind_method(
                D_METHOD("set_pantograph_rear_arms", "value"), &MaszynaRailVehicleAppearance::set_pantograph_rear_arms);
        ClassDB::bind_method(
                D_METHOD("get_pantograph_rear_arms"), &MaszynaRailVehicleAppearance::get_pantograph_rear_arms);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_STRING_ARRAY, "pantograph_rear_arms"), "set_pantograph_rear_arms",
                "get_pantograph_rear_arms");
        ClassDB::bind_method(D_METHOD("set_wiper_arms", "value"), &MaszynaRailVehicleAppearance::set_wiper_arms);
        ClassDB::bind_method(D_METHOD("get_wiper_arms"), &MaszynaRailVehicleAppearance::get_wiper_arms);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "wiper_arms"), "set_wiper_arms", "get_wiper_arms");
        ClassDB::bind_method(D_METHOD("set_mirrors", "value"), &MaszynaRailVehicleAppearance::set_mirrors);
        ClassDB::bind_method(D_METHOD("get_mirrors"), &MaszynaRailVehicleAppearance::get_mirrors);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "mirrors"), "set_mirrors", "get_mirrors");
        ClassDB::bind_method(D_METHOD("set_doors", "value"), &MaszynaRailVehicleAppearance::set_doors);
        ClassDB::bind_method(D_METHOD("get_doors"), &MaszynaRailVehicleAppearance::get_doors);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "doors"), "set_doors", "get_doors");
        ClassDB::bind_method(
                D_METHOD("set_pantograph_factors", "value"), &MaszynaRailVehicleAppearance::set_pantograph_factors);
        ClassDB::bind_method(D_METHOD("get_pantograph_factors"), &MaszynaRailVehicleAppearance::get_pantograph_factors);
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_FLOAT64_ARRAY, "pantograph_factors"), "set_pantograph_factors",
                "get_pantograph_factors");
        ClassDB::bind_method(D_METHOD("set_pendulums", "value"), &MaszynaRailVehicleAppearance::set_pendulums);
        ClassDB::bind_method(D_METHOD("get_pendulums"), &MaszynaRailVehicleAppearance::get_pendulums);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "pendulums"), "set_pendulums", "get_pendulums");
        ClassDB::bind_method(
                D_METHOD("set_pendulum_amplitude", "value"), &MaszynaRailVehicleAppearance::set_pendulum_amplitude);
        ClassDB::bind_method(D_METHOD("get_pendulum_amplitude"), &MaszynaRailVehicleAppearance::get_pendulum_amplitude);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "pendulum_amplitude"), "set_pendulum_amplitude", "get_pendulum_amplitude");
        ClassDB::bind_method(D_METHOD("set_door_steps", "value"), &MaszynaRailVehicleAppearance::set_door_steps);
        ClassDB::bind_method(D_METHOD("get_door_steps"), &MaszynaRailVehicleAppearance::get_door_steps);
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_STRING_ARRAY, "door_steps"), "set_door_steps", "get_door_steps");
        ClassDB::bind_method(
                D_METHOD("set_low_poly_emission_energy", "value"),
                &MaszynaRailVehicleAppearance::set_low_poly_emission_energy);
        ClassDB::bind_method(
                D_METHOD("get_low_poly_emission_energy"), &MaszynaRailVehicleAppearance::get_low_poly_emission_energy);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "low_poly_emission_energy"), "set_low_poly_emission_energy",
                "get_low_poly_emission_energy");
        ClassDB::bind_method(
                D_METHOD("set_low_poly_emission_fade_time", "value"),
                &MaszynaRailVehicleAppearance::set_low_poly_emission_fade_time);
        ClassDB::bind_method(
                D_METHOD("get_low_poly_emission_fade_time"),
                &MaszynaRailVehicleAppearance::get_low_poly_emission_fade_time);
        ADD_PROPERTY(
                PropertyInfo(Variant::FLOAT, "low_poly_emission_fade_time"), "set_low_poly_emission_fade_time",
                "get_low_poly_emission_fade_time");
        ClassDB::bind_method(D_METHOD("set_joint_cabs", "value"), &MaszynaRailVehicleAppearance::set_joint_cabs);
        ClassDB::bind_method(D_METHOD("get_joint_cabs"), &MaszynaRailVehicleAppearance::get_joint_cabs);
        ADD_PROPERTY(PropertyInfo(Variant::BOOL, "joint_cabs"), "set_joint_cabs", "get_joint_cabs");
    }

    void MaszynaRailVehicleAppearance::set_pantograph_front_arms(const PackedStringArray &p_value) {
        pantograph_front_arms = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_pantograph_front_arms() const {
        return pantograph_front_arms;
    }

    void MaszynaRailVehicleAppearance::set_pantograph_rear_arms(const PackedStringArray &p_value) {
        pantograph_rear_arms = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_pantograph_rear_arms() const {
        return pantograph_rear_arms;
    }

    void MaszynaRailVehicleAppearance::set_wiper_arms(const PackedStringArray &p_value) {
        wiper_arms = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_wiper_arms() const {
        return wiper_arms;
    }

    void MaszynaRailVehicleAppearance::set_mirrors(const PackedStringArray &p_value) {
        mirrors = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_mirrors() const {
        return mirrors;
    }

    void MaszynaRailVehicleAppearance::set_doors(const PackedStringArray &p_value) {
        doors = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_doors() const {
        return doors;
    }

    void MaszynaRailVehicleAppearance::set_pantograph_factors(const PackedFloat64Array &p_value) {
        pantograph_factors = p_value;
    }

    PackedFloat64Array MaszynaRailVehicleAppearance::get_pantograph_factors() const {
        return pantograph_factors;
    }

    void MaszynaRailVehicleAppearance::set_pendulums(const PackedStringArray &p_value) {
        pendulums = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_pendulums() const {
        return pendulums;
    }

    void MaszynaRailVehicleAppearance::set_pendulum_amplitude(const double p_value) {
        pendulum_amplitude = p_value;
    }

    double MaszynaRailVehicleAppearance::get_pendulum_amplitude() const {
        return pendulum_amplitude;
    }

    void MaszynaRailVehicleAppearance::set_door_steps(const PackedStringArray &p_value) {
        door_steps = p_value;
    }

    PackedStringArray MaszynaRailVehicleAppearance::get_door_steps() const {
        return door_steps;
    }

    void MaszynaRailVehicleAppearance::set_low_poly_emission_energy(const double p_value) {
        low_poly_emission_energy = p_value;
    }

    double MaszynaRailVehicleAppearance::get_low_poly_emission_energy() const {
        return low_poly_emission_energy;
    }

    void MaszynaRailVehicleAppearance::set_low_poly_emission_fade_time(const double p_value) {
        low_poly_emission_fade_time = p_value;
    }

    double MaszynaRailVehicleAppearance::get_low_poly_emission_fade_time() const {
        return low_poly_emission_fade_time;
    }

    void MaszynaRailVehicleAppearance::set_joint_cabs(const bool p_value) {
        joint_cabs = p_value;
    }

    bool MaszynaRailVehicleAppearance::get_joint_cabs() const {
        return joint_cabs;
    }
} // namespace godot
