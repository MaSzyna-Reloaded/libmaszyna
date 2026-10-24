#include "MaszynaMoverBrakePressureTableItem.hpp"

namespace godot {
    void MaszynaMoverBrakePressureTableItem::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverBrakePressureTableItem, Variant::INT, handle_position);
        BIND_PROPERTY(MaszynaMoverBrakePressureTableItem, Variant::FLOAT, pipe_pressure);
        BIND_PROPERTY(MaszynaMoverBrakePressureTableItem, Variant::FLOAT, brake_cylinder_pressure);
        BIND_PROPERTY(MaszynaMoverBrakePressureTableItem, Variant::FLOAT, fill_speed);
        BIND_PROPERTY_W_HINT(
                MaszynaMoverBrakePressureTableItem, Variant::INT, brake_type, PROPERTY_HINT_ENUM,
                "Pneumatic,ElectroPneumatic,Individual");

        BIND_ENUM_CONSTANT(BRAKE_TYPE_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_TYPE_ELECTRO_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_TYPE_INDIVIDUAL);
    }
} // namespace godot
