#include "Timetable.hpp"

namespace godot {
    void Timetable::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_train_name", "train_name"), &Timetable::set_train_name);
        ClassDB::bind_method(D_METHOD("get_train_name"), &Timetable::get_train_name);
        ClassDB::bind_method(D_METHOD("set_train_category", "train_category"), &Timetable::set_train_category);
        ClassDB::bind_method(D_METHOD("get_train_category"), &Timetable::get_train_category);
        ClassDB::bind_method(D_METHOD("set_train_label", "train_label"), &Timetable::set_train_label);
        ClassDB::bind_method(D_METHOD("get_train_label"), &Timetable::get_train_label);
        ClassDB::bind_method(D_METHOD("set_relation_from", "station"), &Timetable::set_relation_from);
        ClassDB::bind_method(D_METHOD("get_relation_from"), &Timetable::get_relation_from);
        ClassDB::bind_method(D_METHOD("set_relation_to", "station"), &Timetable::set_relation_to);
        ClassDB::bind_method(D_METHOD("get_relation_to"), &Timetable::get_relation_to);
        ClassDB::bind_method(D_METHOD("set_brake_ratio", "ratio"), &Timetable::set_brake_ratio);
        ClassDB::bind_method(D_METHOD("get_brake_ratio"), &Timetable::get_brake_ratio);
        ClassDB::bind_method(D_METHOD("set_locomotive_series", "series"), &Timetable::set_locomotive_series);
        ClassDB::bind_method(D_METHOD("get_locomotive_series"), &Timetable::get_locomotive_series);
        ClassDB::bind_method(D_METHOD("set_locomotive_load", "load"), &Timetable::set_locomotive_load);
        ClassDB::bind_method(D_METHOD("get_locomotive_load"), &Timetable::get_locomotive_load);
        ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &Timetable::set_velocity);
        ClassDB::bind_method(D_METHOD("get_velocity"), &Timetable::get_velocity);
        ClassDB::bind_method(D_METHOD("set_entries", "entries"), &Timetable::set_entries);
        ClassDB::bind_method(D_METHOD("get_entries"), &Timetable::get_entries);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "train_name"), "set_train_name", "get_train_name");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "train_category"), "set_train_category", "get_train_category");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "train_label"), "set_train_label", "get_train_label");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "relation_from"), "set_relation_from", "get_relation_from");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "relation_to"), "set_relation_to", "get_relation_to");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "brake_ratio"), "set_brake_ratio", "get_brake_ratio");
        ADD_PROPERTY(
                PropertyInfo(Variant::STRING, "locomotive_series"), "set_locomotive_series", "get_locomotive_series");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "locomotive_load"), "set_locomotive_load", "get_locomotive_load");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity"), "set_velocity", "get_velocity");
        ADD_PROPERTY(
                PropertyInfo(Variant::ARRAY, "entries", PROPERTY_HINT_ARRAY_TYPE, "TimetableEntry"), "set_entries",
                "get_entries");
    }

    void Timetable::set_train_name(const String &p_train_name) {
        train_name = p_train_name;
    }

    String Timetable::get_train_name() const {
        return train_name;
    }

    void Timetable::set_train_category(const String &p_train_category) {
        train_category = p_train_category;
    }

    String Timetable::get_train_category() const {
        return train_category;
    }

    void Timetable::set_train_label(const String &p_train_label) {
        train_label = p_train_label;
    }

    String Timetable::get_train_label() const {
        return train_label;
    }

    void Timetable::set_relation_from(const String &p_station) {
        relation_from = p_station;
    }

    String Timetable::get_relation_from() const {
        return relation_from;
    }

    void Timetable::set_relation_to(const String &p_station) {
        relation_to = p_station;
    }

    String Timetable::get_relation_to() const {
        return relation_to;
    }

    void Timetable::set_brake_ratio(const double p_ratio) {
        brake_ratio = p_ratio;
    }

    double Timetable::get_brake_ratio() const {
        return brake_ratio;
    }

    void Timetable::set_locomotive_series(const String &p_series) {
        locomotive_series = p_series;
    }

    String Timetable::get_locomotive_series() const {
        return locomotive_series;
    }

    void Timetable::set_locomotive_load(const double p_load) {
        locomotive_load = p_load;
    }

    double Timetable::get_locomotive_load() const {
        return locomotive_load;
    }

    void Timetable::set_velocity(const double p_velocity) {
        velocity = p_velocity;
    }

    double Timetable::get_velocity() const {
        return velocity;
    }

    void Timetable::set_entries(const TypedArray<TimetableEntry> &p_entries) {
        entries = p_entries;
    }

    TypedArray<TimetableEntry> Timetable::get_entries() const {
        return entries;
    }
} // namespace godot
