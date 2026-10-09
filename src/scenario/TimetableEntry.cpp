#include "TimetableEntry.hpp"

namespace godot {
    void TimetableEntry::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_kilometre", "kilometre"), &TimetableEntry::set_kilometre);
        ClassDB::bind_method(D_METHOD("get_kilometre"), &TimetableEntry::get_kilometre);
        ClassDB::bind_method(D_METHOD("set_velocity", "velocity"), &TimetableEntry::set_velocity);
        ClassDB::bind_method(D_METHOD("get_velocity"), &TimetableEntry::get_velocity);
        ClassDB::bind_method(D_METHOD("set_station_name", "station_name"), &TimetableEntry::set_station_name);
        ClassDB::bind_method(D_METHOD("get_station_name"), &TimetableEntry::get_station_name);
        ClassDB::bind_method(D_METHOD("set_facilities", "facilities"), &TimetableEntry::set_facilities);
        ClassDB::bind_method(D_METHOD("get_facilities"), &TimetableEntry::get_facilities);
        ClassDB::bind_method(D_METHOD("set_track_count", "track_count"), &TimetableEntry::set_track_count);
        ClassDB::bind_method(D_METHOD("get_track_count"), &TimetableEntry::get_track_count);
        ClassDB::bind_method(D_METHOD("set_arrival", "hours"), &TimetableEntry::set_arrival);
        ClassDB::bind_method(D_METHOD("get_arrival"), &TimetableEntry::get_arrival);
        ClassDB::bind_method(D_METHOD("set_departure", "hours"), &TimetableEntry::set_departure);
        ClassDB::bind_method(D_METHOD("get_departure"), &TimetableEntry::get_departure);
        ClassDB::bind_method(D_METHOD("set_travel_minutes", "minutes"), &TimetableEntry::set_travel_minutes);
        ClassDB::bind_method(D_METHOD("get_travel_minutes"), &TimetableEntry::get_travel_minutes);
        ClassDB::bind_method(D_METHOD("set_radio_channel", "channel"), &TimetableEntry::set_radio_channel);
        ClassDB::bind_method(D_METHOD("get_radio_channel"), &TimetableEntry::get_radio_channel);
        ClassDB::bind_method(D_METHOD("is_stop"), &TimetableEntry::is_stop);

        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "kilometre"), "set_kilometre", "get_kilometre");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "velocity"), "set_velocity", "get_velocity");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "station_name"), "set_station_name", "get_station_name");
        ADD_PROPERTY(PropertyInfo(Variant::STRING, "facilities"), "set_facilities", "get_facilities");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "track_count"), "set_track_count", "get_track_count");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "arrival"), "set_arrival", "get_arrival");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "departure"), "set_departure", "get_departure");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "travel_minutes"), "set_travel_minutes", "get_travel_minutes");
        ADD_PROPERTY(PropertyInfo(Variant::INT, "radio_channel"), "set_radio_channel", "get_radio_channel");
    }

    void TimetableEntry::set_kilometre(const double p_kilometre) {
        kilometre = p_kilometre;
    }

    double TimetableEntry::get_kilometre() const {
        return kilometre;
    }

    void TimetableEntry::set_velocity(const double p_velocity) {
        velocity = p_velocity;
    }

    double TimetableEntry::get_velocity() const {
        return velocity;
    }

    void TimetableEntry::set_station_name(const String &p_station_name) {
        station_name = p_station_name;
    }

    String TimetableEntry::get_station_name() const {
        return station_name;
    }

    void TimetableEntry::set_facilities(const String &p_facilities) {
        facilities = p_facilities;
    }

    String TimetableEntry::get_facilities() const {
        return facilities;
    }

    void TimetableEntry::set_track_count(const int p_track_count) {
        track_count = p_track_count;
    }

    int TimetableEntry::get_track_count() const {
        return track_count;
    }

    void TimetableEntry::set_arrival(const double p_hours) {
        arrival = p_hours;
    }

    double TimetableEntry::get_arrival() const {
        return arrival;
    }

    void TimetableEntry::set_departure(const double p_hours) {
        departure = p_hours;
    }

    double TimetableEntry::get_departure() const {
        return departure;
    }

    void TimetableEntry::set_travel_minutes(const double p_minutes) {
        travel_minutes = p_minutes;
    }

    double TimetableEntry::get_travel_minutes() const {
        return travel_minutes;
    }

    void TimetableEntry::set_radio_channel(const int p_channel) {
        radio_channel = p_channel;
    }

    int TimetableEntry::get_radio_channel() const {
        return radio_channel;
    }

    bool TimetableEntry::is_stop() const {
        return arrival >= 0.0;
    }
} // namespace godot
