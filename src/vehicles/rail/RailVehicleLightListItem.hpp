#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleLightListItem : public Resource {
            GDCLASS(RailVehicleLightListItem, Resource);

        public:
            static void _bind_methods();

        private:
            bool cabin_a_head_light = false;
            bool cabin_a_left_white_signal = false;
            bool cabin_a_left_red_signal = false;
            bool cabin_a_right_white_signal = false;
            bool cabin_a_right_red_signal = false;
            bool cabin_a_end_signals = false;
            bool cabin_a_left_auxiliary_light = false;
            bool cabin_a_right_auxiliary_light = false;
            bool cabin_b_head_light = false;
            bool cabin_b_left_white_signal = false;
            bool cabin_b_left_red_signal = false;
            bool cabin_b_right_white_signal = false;
            bool cabin_b_right_red_signal = false;
            bool cabin_b_end_signals = false;
            bool cabin_b_left_auxiliary_light = false;
            bool cabin_b_right_auxiliary_light = false;

        public:
            MAKE_GS(bool, cabin_a_head_light);
            MAKE_GS(bool, cabin_a_left_white_signal);
            MAKE_GS(bool, cabin_a_left_red_signal);
            MAKE_GS(bool, cabin_a_right_white_signal);
            MAKE_GS(bool, cabin_a_right_red_signal);
            MAKE_GS(bool, cabin_a_end_signals);
            MAKE_GS(bool, cabin_a_left_auxiliary_light);
            MAKE_GS(bool, cabin_a_right_auxiliary_light);
            MAKE_GS(bool, cabin_b_head_light);
            MAKE_GS(bool, cabin_b_left_white_signal);
            MAKE_GS(bool, cabin_b_left_red_signal);
            MAKE_GS(bool, cabin_b_right_white_signal);
            MAKE_GS(bool, cabin_b_right_red_signal);
            MAKE_GS(bool, cabin_b_end_signals);
            MAKE_GS(bool, cabin_b_left_auxiliary_light);
            MAKE_GS(bool, cabin_b_right_auxiliary_light);
    };
} // namespace godot
