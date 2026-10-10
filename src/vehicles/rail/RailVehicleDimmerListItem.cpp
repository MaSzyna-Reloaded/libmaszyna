#include "RailVehicleDimmerListItem.hpp"

namespace godot {
    void RailVehicleDimmerListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleDimmerListItem, Variant::BOOL, high_beam);
        BIND_PROPERTY(RailVehicleDimmerListItem, Variant::BOOL, dimmed);
        BIND_PROPERTY(RailVehicleDimmerListItem, Variant::BOOL, off);
    }
} // namespace godot
