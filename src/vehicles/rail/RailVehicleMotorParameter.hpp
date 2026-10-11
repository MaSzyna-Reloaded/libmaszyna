#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleMotorParameter : public Resource {
            GDCLASS(RailVehicleMotorParameter, Resource);

        public:
            static void _bind_methods();

        private:
            double shunting_up = 0.0;
            double shunting_down = 0.0;
            double voltage_constant_multiplier = 0.0;
            double saturation_current_multiplier = 0.0;
            double initial_voltage_constant_multiplier = 0.0;
            double voltage_constant = 0.0;
            double saturation_current = 0.0;
            double initial_voltage_constant = 0.0;
            bool auto_switch = false;

        public:
            MAKE_GS(double, shunting_up);
            MAKE_GS(double, shunting_down);
            MAKE_GS(double, voltage_constant_multiplier);
            MAKE_GS(double, saturation_current_multiplier);
            MAKE_GS(double, initial_voltage_constant_multiplier);
            MAKE_GS(double, voltage_constant);
            MAKE_GS(double, saturation_current);
            MAKE_GS(double, initial_voltage_constant);
            MAKE_GS(bool, auto_switch);
    };
} // namespace godot
