#include "VehicleCurvePointItem.hpp"

namespace godot {
    void VehicleCurvePointItem::_bind_methods() {
        BIND_PROPERTY(VehicleCurvePointItem, Variant::FLOAT, x);
        BIND_PROPERTY(VehicleCurvePointItem, Variant::FLOAT, y);
    }
} // namespace godot
