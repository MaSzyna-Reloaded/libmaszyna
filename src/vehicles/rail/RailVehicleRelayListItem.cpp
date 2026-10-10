#include "RailVehicleRelayListItem.hpp"

namespace godot {
    void RailVehicleRelayListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleRelayListItem, Variant::INT, relay_position);
        BIND_PROPERTY(RailVehicleRelayListItem, Variant::FLOAT, resistance);
        BIND_PROPERTY(RailVehicleRelayListItem, Variant::INT, branch_count);
        BIND_PROPERTY(RailVehicleRelayListItem, Variant::INT, motors_per_branch);
        BIND_PROPERTY(RailVehicleRelayListItem, Variant::BOOL, auto_switch);
        BIND_PROPERTY(RailVehicleRelayListItem, Variant::INT, shunt_index);
    }
} // namespace godot
