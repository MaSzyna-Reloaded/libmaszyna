#include "RelayListItem.hpp"

namespace godot {
    void RelayListItem::_bind_methods() {
        BIND_PROPERTY(
                Variant::INT, "relay_position", "relay_position", &RelayListItem::set_relay_position,
                &RelayListItem::get_relay_position, "relay_position");
        BIND_PROPERTY(
                Variant::FLOAT, "resistance", "resistance", &RelayListItem::set_resistance,
                &RelayListItem::get_resistance, "resistance");
        BIND_PROPERTY(
                Variant::INT, "branch_count", "branch_count", &RelayListItem::set_branch_count,
                &RelayListItem::get_branch_count, "branch_count");
        BIND_PROPERTY(
                Variant::INT, "motors_per_branch", "motors_per_branch", &RelayListItem::set_motors_per_branch,
                &RelayListItem::get_motors_per_branch, "motors_per_branch");
        BIND_PROPERTY(
                Variant::BOOL, "auto_switch", "auto_switch", &RelayListItem::set_auto_switch,
                &RelayListItem::get_auto_switch, "auto_switch");
        BIND_PROPERTY(
                Variant::INT, "shunt_index", "shunt_index", &RelayListItem::set_shunt_index,
                &RelayListItem::get_shunt_index, "shunt_index");
    }
} // namespace godot
