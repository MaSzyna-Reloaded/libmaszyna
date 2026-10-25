#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class MaszynaMoverUniversalControllerListItem : public Resource {
            GDCLASS(MaszynaMoverUniversalControllerListItem, Resource);

        public:
            static void _bind_methods();

        private:
            int pneumatic_brake_position = -1;
            double min_percentage = 0.0;
            double max_percentage = 0.0;
            double target_value = 0.0;
            double increase_speed = 0.0;
            double decrease_speed = 0.0;
            int bounce_back_position = 0;
            int nearest_stable_down = 0;
            int nearest_stable_up = 0;

        public:
            MAKE_GS(int, pneumatic_brake_position);
            MAKE_GS(double, min_percentage);
            MAKE_GS(double, max_percentage);
            MAKE_GS(double, target_value);
            MAKE_GS(double, increase_speed);
            MAKE_GS(double, decrease_speed);
            MAKE_GS(int, bounce_back_position);
            MAKE_GS(int, nearest_stable_down);
            MAKE_GS(int, nearest_stable_up);
    };
} // namespace godot
