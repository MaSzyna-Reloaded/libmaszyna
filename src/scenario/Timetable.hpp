#pragma once
#include "TimetableEntry.hpp"
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// A train's timetable (TTrainParameters, mtable.h:45-100) - what it is, where it runs and when,
    /// station by station. Data only: how far a train has got through it belongs to whoever drives
    /// the train.
    class Timetable : public Resource {
            GDCLASS(Timetable, Resource)

        private:
            String train_name;
            String train_category;
            String train_label;
            String relation_from;
            String relation_to;
            double brake_ratio = 0.0;
            String locomotive_series;
            double locomotive_load = 0.0;
            double velocity = -1.0;
            TypedArray<TimetableEntry> entries;

        protected:
            static void _bind_methods();

        public:
            /// The train number (`TNS654323`)
            void set_train_name(const String &p_train_name);
            String get_train_name() const;
            void set_train_category(const String &p_train_category);
            String get_train_category() const;
            /// The train's own name, if it has one
            void set_train_label(const String &p_train_label);
            String get_train_label() const;
            void set_relation_from(const String &p_station);
            String get_relation_from() const;
            void set_relation_to(const String &p_station);
            String get_relation_to() const;
            /// Required braked weight, percent
            void set_brake_ratio(double p_ratio);
            double get_brake_ratio() const;
            void set_locomotive_series(const String &p_series);
            String get_locomotive_series() const;
            /// Tonnes the locomotive is rated to haul
            void set_locomotive_load(double p_load);
            double get_locomotive_load() const;
            /// The train's speed limit when the timetable is only a number (km/h), negative when the
            /// stations give it (TTrainParameters::LoadTTfile(), mtable.cpp:283-293)
            void set_velocity(double p_velocity);
            double get_velocity() const;
            /// The stations, in the order the train reaches them
            void set_entries(const TypedArray<TimetableEntry> &p_entries);
            TypedArray<TimetableEntry> get_entries() const;
    };
} // namespace godot
