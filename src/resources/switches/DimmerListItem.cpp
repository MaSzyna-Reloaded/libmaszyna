#include "DimmerListItem.hpp"

namespace godot {
    void DimmerListItem::_bind_methods() {
        BIND_PROPERTY(
                Variant::BOOL, "high_beam", "high_beam", &DimmerListItem::set_high_beam, &DimmerListItem::get_high_beam,
                "high_beam");
        BIND_PROPERTY(
                Variant::BOOL, "dimmed", "dimmed", &DimmerListItem::set_dimmed, &DimmerListItem::get_dimmed, "dimmed");
        BIND_PROPERTY(Variant::BOOL, "off", "off", &DimmerListItem::set_off, &DimmerListItem::get_off, "off");
    }
} // namespace godot
