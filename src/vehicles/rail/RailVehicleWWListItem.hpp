#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleWWListItem : public Resource {
            GDCLASS(RailVehicleWWListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(double, rpm, 0.0);
            MAKE_MEMBER_GS(double, max_power, 0.0);
            MAKE_MEMBER_GS(double, max_voltage, 0.0);
            MAKE_MEMBER_GS(double, max_current, 0.0);
            MAKE_MEMBER_GS(bool, has_shunting, 0.0);
            MAKE_MEMBER_GS(double, min_wakeup_voltage, 0.0);
            MAKE_MEMBER_GS(double, max_wakeup_voltage, 0.0);
            MAKE_MEMBER_GS(double, max_wakeup_power, 0.0);
    };
} // namespace godot
