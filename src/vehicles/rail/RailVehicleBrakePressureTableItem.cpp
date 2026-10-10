#include "RailVehicleBrakePressureTableItem.hpp"

namespace godot {
    void RailVehicleBrakePressureTableItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleBrakePressureTableItem, Variant::INT, handle_position);
        BIND_PROPERTY(RailVehicleBrakePressureTableItem, Variant::FLOAT, pipe_pressure);
        BIND_PROPERTY(RailVehicleBrakePressureTableItem, Variant::FLOAT, brake_cylinder_pressure);
        BIND_PROPERTY(RailVehicleBrakePressureTableItem, Variant::FLOAT, fill_speed);
        BIND_PROPERTY_W_HINT(
                RailVehicleBrakePressureTableItem, Variant::INT, brake_type, PROPERTY_HINT_ENUM,
                "Pneumatic,ElectroPneumatic,Individual");

        BIND_ENUM_CONSTANT(BRAKE_TYPE_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_TYPE_ELECTRO_PNEUMATIC);
        BIND_ENUM_CONSTANT(BRAKE_TYPE_INDIVIDUAL);
    }
} // namespace godot
