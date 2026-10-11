#include "RailVehicleWiperListItem.hpp"

namespace godot {
    void RailVehicleWiperListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleWiperListItem, Variant::INT, wiper_mask);
        BIND_PROPERTY(RailVehicleWiperListItem, Variant::FLOAT, transit_time);
        BIND_PROPERTY(RailVehicleWiperListItem, Variant::FLOAT, period);
        BIND_PROPERTY(RailVehicleWiperListItem, Variant::FLOAT, return_delay);
    }
} // namespace godot
