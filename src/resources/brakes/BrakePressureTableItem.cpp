#include "BrakePressureTableItem.hpp"

namespace godot {
    void BrakePressureTableItem::_bind_methods() {
        BIND_PROPERTY(
                Variant::INT, "handle_position", "handle_position", &BrakePressureTableItem::set_handle_position,
                &BrakePressureTableItem::get_handle_position, "handle_position");
        BIND_PROPERTY(
                Variant::FLOAT, "pipe_pressure", "pipe_pressure", &BrakePressureTableItem::set_pipe_pressure,
                &BrakePressureTableItem::get_pipe_pressure, "pipe_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "brake_cylinder_pressure", "brake_cylinder_pressure",
                &BrakePressureTableItem::set_brake_cylinder_pressure,
                &BrakePressureTableItem::get_brake_cylinder_pressure, "brake_cylinder_pressure");
        BIND_PROPERTY(
                Variant::FLOAT, "fill_speed", "fill_speed", &BrakePressureTableItem::set_fill_speed,
                &BrakePressureTableItem::get_fill_speed, "fill_speed");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "brake_type", "brake_type", &BrakePressureTableItem::set_brake_type,
                &BrakePressureTableItem::get_brake_type, "brake_type", PROPERTY_HINT_ENUM,
                "Pneumatic,ElectroPneumatic,Individual");

        BIND_ENUM_CONSTANT(BRAKE_TYPE_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_TYPE_ELECTRO_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_TYPE_INDIVIDUAL);
    }
} // namespace godot
