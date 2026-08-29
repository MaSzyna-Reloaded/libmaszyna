#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class CompressorListItem : public Resource {
            GDCLASS(CompressorListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(int, allow, 0);
            MAKE_MEMBER_GS(int, speed_factor, 1);
            MAKE_MEMBER_GS(int, min_pressure_factor, 1);
            MAKE_MEMBER_GS(int, max_pressure_factor, 1);
    };
} // namespace godot
