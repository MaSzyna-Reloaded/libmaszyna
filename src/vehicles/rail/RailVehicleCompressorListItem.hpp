#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleCompressorListItem : public Resource {
            GDCLASS(RailVehicleCompressorListItem, Resource);

        public:
            static void _bind_methods();

        private:
            int allow = 0;
            int speed_factor = 1;
            int min_pressure_factor = 1;
            int max_pressure_factor = 1;

        public:
            MAKE_GS(int, allow);
            MAKE_GS(int, speed_factor);
            MAKE_GS(int, min_pressure_factor);
            MAKE_GS(int, max_pressure_factor);
    };
} // namespace godot
