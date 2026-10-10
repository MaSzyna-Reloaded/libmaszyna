#pragma once
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    /// One station of a Timetable (TMTableLine, mtable.h:23-39). Times are hours since midnight,
    /// fractional; a negative arrival means the train passes without stopping.
    class TimetableEntry : public Resource {
            GDCLASS(TimetableEntry, Resource)

        public:
            /// A time left out of the table
            static constexpr double NO_TIME = -1.0;

        private:
            double kilometre = 0.0;
            double velocity = -1.0;
            String station_name;
            String facilities;
            int track_count = 1;
            double arrival = NO_TIME;
            double departure = NO_TIME;
            double travel_minutes = 0.0;
            int radio_channel = -1;

        protected:
            static void _bind_methods();

        public:
            /// Where the station is on the line, km
            void set_kilometre(double p_kilometre);
            double get_kilometre() const;
            /// The timetable speed on the way to this station, km/h; negative when unknown
            void set_velocity(double p_velocity);
            double get_velocity() const;
            /// `_` in place of spaces, as the table writes it
            void set_station_name(const String &p_station_name);
            String get_station_name() const;
            /// The station's type and equipment, comma separated (`R4,RT,H,PP`, `@` for a change of
            /// direction there)
            void set_facilities(const String &p_facilities);
            String get_facilities() const;
            /// Tracks of the line, 1 or 2
            void set_track_count(int p_track_count);
            int get_track_count() const;
            void set_arrival(double p_hours);
            double get_arrival() const;
            void set_departure(double p_hours);
            double get_departure() const;
            /// The running time to this station, minutes
            void set_travel_minutes(double p_minutes);
            double get_travel_minutes() const;
            /// The radio channel used from this station on, -1 for none given
            void set_radio_channel(int p_channel);
            int get_radio_channel() const;
            /// Whether the train stops here - an arrival time is given (TTrainParameters::IsStop())
            bool is_stop() const;
    };
} // namespace godot
