#include "RailVehicleLoadListItem.hpp"
#include "macros.hpp"

namespace godot {
    void RailVehicleLoadListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleLoadListItem, Variant::FLOAT, max_load)
        BIND_PROPERTY_W_HINT(
                RailVehicleLoadListItem, Variant::INT, load_type, PROPERTY_HINT_ENUM, "None,Passenger,Cargo")
        BIND_PROPERTY(RailVehicleLoadListItem, Variant::FLOAT, load_offset)

        BIND_ENUM_CONSTANT(LOAD_TYPE_NONE);
        BIND_ENUM_CONSTANT(LOAD_TYPE_PASSENGER);
        BIND_ENUM_CONSTANT(LOAD_TYPE_CARGO);
    }
} // namespace godot
