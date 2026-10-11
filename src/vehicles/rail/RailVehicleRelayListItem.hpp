#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleRelayListItem : public Resource {
            GDCLASS(RailVehicleRelayListItem, Resource);

        public:
            static void _bind_methods();

        private:
            int relay_position = 0;
            double resistance = 0.0;
            int branch_count = 0;
            int motors_per_branch = 0;
            bool auto_switch = false;
            int shunt_index = 0;

        public:
            MAKE_GS(int, relay_position);
            MAKE_GS(double, resistance);
            MAKE_GS(int, branch_count);
            MAKE_GS(int, motors_per_branch);
            MAKE_GS(bool, auto_switch);
            MAKE_GS(int, shunt_index);
    };
} // namespace godot
