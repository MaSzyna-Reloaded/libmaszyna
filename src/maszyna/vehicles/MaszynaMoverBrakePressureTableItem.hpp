#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class MaszynaMoverBrakePressureTableItem : public Resource {
            GDCLASS(MaszynaMoverBrakePressureTableItem, Resource);

        public:
            enum BrakeType {
                BRAKE_TYPE_PNEUMATIC,
                BRAKE_TYPE_ELECTRO_PNEUMATIC,
                BRAKE_TYPE_INDIVIDUAL,
            };

            static void _bind_methods();

        private:
            int handle_position = 0;
            double pipe_pressure = 0.0;
            double brake_cylinder_pressure = -1.0;
            double fill_speed = 0.0;
            BrakeType brake_type = BRAKE_TYPE_PNEUMATIC;

        public:
            MAKE_GS(int, handle_position);
            MAKE_GS(double, pipe_pressure);
            MAKE_GS(double, brake_cylinder_pressure);
            MAKE_GS(double, fill_speed);
            MAKE_GS_NR(BrakeType, brake_type);
    };
} // namespace godot
VARIANT_ENUM_CAST(MaszynaMoverBrakePressureTableItem::BrakeType)
