#include "RailVehicleCompressorListItem.hpp"

namespace godot {
    void RailVehicleCompressorListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleCompressorListItem, Variant::INT, allow);
        BIND_PROPERTY(RailVehicleCompressorListItem, Variant::INT, speed_factor);
        BIND_PROPERTY(RailVehicleCompressorListItem, Variant::INT, min_pressure_factor);
        BIND_PROPERTY(RailVehicleCompressorListItem, Variant::INT, max_pressure_factor);
    }
} // namespace godot
