#include "MaszynaMoverWWListItem.hpp"

namespace godot {
    void MaszynaMoverWWListItem::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, rpm);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, max_power);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, max_voltage);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, max_current);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::BOOL, has_shunting);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, min_wakeup_voltage);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, max_wakeup_voltage);
        BIND_PROPERTY(MaszynaMoverWWListItem, Variant::FLOAT, max_wakeup_power);
    }
} // namespace godot
