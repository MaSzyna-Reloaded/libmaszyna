#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class RailVehicleWWListItem : public Resource {
            GDCLASS(RailVehicleWWListItem, Resource);

        public:
            static void _bind_methods();

        private:
            double rpm = 0.0;
            double max_power = 0.0;
            double max_voltage = 0.0;
            double max_current = 0.0;
            bool has_shunting = false;
            double min_wakeup_voltage = 0.0;
            double max_wakeup_voltage = 0.0;
            double max_wakeup_power = 0.0;

        public:
            MAKE_GS(double, rpm);
            MAKE_GS(double, max_power);
            MAKE_GS(double, max_voltage);
            MAKE_GS(double, max_current);
            MAKE_GS(bool, has_shunting);
            MAKE_GS(double, min_wakeup_voltage);
            MAKE_GS(double, max_wakeup_voltage);
            MAKE_GS(double, max_wakeup_power);
    };
} // namespace godot
