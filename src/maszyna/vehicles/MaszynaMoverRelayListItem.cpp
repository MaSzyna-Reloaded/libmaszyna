#include "MaszynaMoverRelayListItem.hpp"

namespace godot {
    void MaszynaMoverRelayListItem::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverRelayListItem, Variant::INT, relay_position);
        BIND_PROPERTY(MaszynaMoverRelayListItem, Variant::FLOAT, resistance);
        BIND_PROPERTY(MaszynaMoverRelayListItem, Variant::INT, branch_count);
        BIND_PROPERTY(MaszynaMoverRelayListItem, Variant::INT, motors_per_branch);
        BIND_PROPERTY(MaszynaMoverRelayListItem, Variant::BOOL, auto_switch);
        BIND_PROPERTY(MaszynaMoverRelayListItem, Variant::INT, shunt_index);
    }
} // namespace godot
