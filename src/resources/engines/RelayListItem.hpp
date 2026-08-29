#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RelayListItem : public Resource {
            GDCLASS(RelayListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(int, relay_position, 0);
            MAKE_MEMBER_GS(double, resistance, 0.0);
            MAKE_MEMBER_GS(int, branch_count, 0);
            MAKE_MEMBER_GS(int, motors_per_branch, 0);
            MAKE_MEMBER_GS(bool, auto_switch, false);
            MAKE_MEMBER_GS(int, shunt_index, 0);
    };
} // namespace godot
