#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleDimmerListItem : public Resource {
            GDCLASS(RailVehicleDimmerListItem, Resource);

        public:
            static void _bind_methods();

        private:
            bool high_beam = false;
            bool dimmed = false;
            bool off = false;

        public:
            MAKE_GS(bool, high_beam);
            MAKE_GS(bool, dimmed);
            MAKE_GS(bool, off);
    };
} // namespace godot
