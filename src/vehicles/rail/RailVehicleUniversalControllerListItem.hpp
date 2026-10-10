#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleUniversalControllerListItem : public Resource {
            GDCLASS(RailVehicleUniversalControllerListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(int, pneumatic_brake_position, -1);
            MAKE_MEMBER_GS(double, min_percentage, 0.0);
            MAKE_MEMBER_GS(double, max_percentage, 0.0);
            MAKE_MEMBER_GS(double, target_value, 0.0);
            MAKE_MEMBER_GS(double, increase_speed, 0.0);
            MAKE_MEMBER_GS(double, decrease_speed, 0.0);
            MAKE_MEMBER_GS(int, bounce_back_position, 0);
            MAKE_MEMBER_GS(int, nearest_stable_down, 0);
            MAKE_MEMBER_GS(int, nearest_stable_up, 0);
    };
} // namespace godot
