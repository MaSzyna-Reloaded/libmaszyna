#include "RailVehicleUniversalControllerListItem.hpp"
#include "macros.hpp"

namespace godot {
    void RailVehicleUniversalControllerListItem::_bind_methods() {
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::INT, pneumatic_brake_position);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::FLOAT, min_percentage);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::FLOAT, max_percentage);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::FLOAT, target_value);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::FLOAT, increase_speed);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::FLOAT, decrease_speed);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::INT, bounce_back_position);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::INT, nearest_stable_down);
        BIND_PROPERTY(RailVehicleUniversalControllerListItem, Variant::INT, nearest_stable_up);
    }
} // namespace godot
