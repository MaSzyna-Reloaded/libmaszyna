#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleLightListItem : public Resource {
            GDCLASS(RailVehicleLightListItem, Resource);

        public:
            static void _bind_methods();
            MAKE_MEMBER_GS(bool, cabin_a_head_light, false);
            MAKE_MEMBER_GS(bool, cabin_a_left_white_signal, false);
            MAKE_MEMBER_GS(bool, cabin_a_left_red_signal, false);
            MAKE_MEMBER_GS(bool, cabin_a_right_white_signal, false);
            MAKE_MEMBER_GS(bool, cabin_a_right_red_signal, false);
            MAKE_MEMBER_GS(bool, cabin_a_end_signals, false);
            MAKE_MEMBER_GS(bool, cabin_a_left_auxiliary_light, false);
            MAKE_MEMBER_GS(bool, cabin_a_right_auxiliary_light, false);
            MAKE_MEMBER_GS(bool, cabin_b_head_light, false);
            MAKE_MEMBER_GS(bool, cabin_b_left_white_signal, false);
            MAKE_MEMBER_GS(bool, cabin_b_left_red_signal, false);
            MAKE_MEMBER_GS(bool, cabin_b_right_white_signal, false);
            MAKE_MEMBER_GS(bool, cabin_b_right_red_signal, false);
            MAKE_MEMBER_GS(bool, cabin_b_end_signals, false);
            MAKE_MEMBER_GS(bool, cabin_b_left_auxiliary_light, false);
            MAKE_MEMBER_GS(bool, cabin_b_right_auxiliary_light, false);
    };
} // namespace godot
