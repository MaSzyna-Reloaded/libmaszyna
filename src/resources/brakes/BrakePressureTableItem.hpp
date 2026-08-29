#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class BrakePressureTableItem : public Resource {
            GDCLASS(BrakePressureTableItem, Resource);

        public:
            enum BrakeType {
                BRAKE_TYPE_PNEUMATIC,
                BRAKE_TYPE_ELECTRO_PNEUMATIC,
                BRAKE_TYPE_INDIVIDUAL,
            };

            static void _bind_methods();
            MAKE_MEMBER_GS(int, handle_position, 0);
            MAKE_MEMBER_GS(double, pipe_pressure, 0.0);
            MAKE_MEMBER_GS(double, brake_cylinder_pressure, -1.0);
            MAKE_MEMBER_GS(double, fill_speed, 0.0);
            MAKE_MEMBER_GS_NR(BrakeType, brake_type, BRAKE_TYPE_PNEUMATIC);
    };
} // namespace godot
VARIANT_ENUM_CAST(BrakePressureTableItem::BrakeType)
