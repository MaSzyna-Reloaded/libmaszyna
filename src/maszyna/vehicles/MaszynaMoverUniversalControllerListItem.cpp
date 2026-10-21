#include "MaszynaMoverUniversalControllerListItem.hpp"
#include "macros.hpp"

namespace godot {
    void MaszynaMoverUniversalControllerListItem::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::INT, pneumatic_brake_position);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::FLOAT, min_percentage);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::FLOAT, max_percentage);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::FLOAT, target_value);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::FLOAT, increase_speed);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::FLOAT, decrease_speed);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::INT, bounce_back_position);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::INT, nearest_stable_down);
        BIND_PROPERTY(MaszynaMoverUniversalControllerListItem, Variant::INT, nearest_stable_up);
    }
} // namespace godot
