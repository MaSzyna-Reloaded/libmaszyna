#include "RailVehicleWWListItem.hpp"

namespace godot {
    void RailVehicleWWListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, rpm);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, max_power);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, max_voltage);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, max_current);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::BOOL, has_shunting);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, min_wakeup_voltage);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, max_wakeup_voltage);
        BIND_PROPERTY(RailVehicleWWListItem, Variant::FLOAT, max_wakeup_power);
    }
} // namespace godot
