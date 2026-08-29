#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class WiperListItem : public Resource {
            GDCLASS(WiperListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(int, wiper_mask, 0);
            MAKE_MEMBER_GS(double, transit_time, 0.0);
            MAKE_MEMBER_GS(double, period, 0.0);
            MAKE_MEMBER_GS(double, return_delay, 0.0);
    };
} // namespace godot
