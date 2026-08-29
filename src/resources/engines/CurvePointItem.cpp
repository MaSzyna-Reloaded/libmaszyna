#include "CurvePointItem.hpp"

namespace godot {
    void CurvePointItem::_bind_methods() {
        BIND_PROPERTY(Variant::FLOAT, "x", "x", &CurvePointItem::set_x, &CurvePointItem::get_x, "x");
        BIND_PROPERTY(Variant::FLOAT, "y", "y", &CurvePointItem::set_y, &CurvePointItem::get_y, "y");
    }
} // namespace godot
