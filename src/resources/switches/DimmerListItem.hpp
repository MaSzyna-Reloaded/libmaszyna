#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class DimmerListItem : public Resource {
            GDCLASS(DimmerListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(bool, high_beam, false);
            MAKE_MEMBER_GS(bool, dimmed, false);
            MAKE_MEMBER_GS(bool, off, false);
    };
} // namespace godot
