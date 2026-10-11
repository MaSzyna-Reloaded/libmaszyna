#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleWiperListItem : public Resource {
            GDCLASS(RailVehicleWiperListItem, Resource);

        public:
            static void _bind_methods();

        private:
            int wiper_mask = 0;
            double transit_time = 0.0;
            double period = 0.0;
            double return_delay = 0.0;

        public:
            MAKE_GS(int, wiper_mask);
            MAKE_GS(double, transit_time);
            MAKE_GS(double, period);
            MAKE_GS(double, return_delay);
    };
} // namespace godot
