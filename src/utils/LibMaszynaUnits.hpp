#pragma once
#include <cstdint>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    /* Unit conversion factors shared by C++ and GDScript. A class of its own, holding nothing,
     * because GDScript sees constants only as members of a registered class. Godot binds a class
     * constant only as an integer, and every factor here is a whole number; a caller dividing an
     * integer by one casts it to double first. */
    class LibMaszynaUnits : public Object {
            GDCLASS(LibMaszynaUnits, Object)

        protected:
            static void _bind_methods();

        public:
            static constexpr int64_t SECONDS_PER_MINUTE = 60;
            static constexpr int64_t MINUTES_PER_HOUR = 60;
            static constexpr int64_t SECONDS_PER_HOUR = 3600;
            static constexpr int64_t MINUTES_PER_DAY = 1440;
            static constexpr int64_t HOURS_PER_DAY = 24;
            static constexpr int64_t SECONDS_PER_DAY = 86400;
            static constexpr int64_t ARCSECONDS_PER_DEGREE = 3600;
            static constexpr int64_t USEC_PER_MSEC = 1000;
            static constexpr int64_t USEC_PER_SECOND = 1000000;
            static constexpr int64_t METRES_PER_KILOMETRE = 1000;
            static constexpr int64_t NEWTONS_PER_KILONEWTON = 1000;
            static constexpr int64_t KILOGRAMS_PER_TONNE = 1000;
            static constexpr int64_t KILOPASCALS_PER_BAR = 100;
            static constexpr int64_t JOULES_PER_KILOWATT_HOUR = 3600000;
            static constexpr int64_t BYTES_PER_KILOBYTE = 1024;
    };
} // namespace godot
