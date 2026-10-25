#include "MaszynaMoverCompressorListItem.hpp"

namespace godot {
    void MaszynaMoverCompressorListItem::_bind_methods() {
        BIND_PROPERTY(MaszynaMoverCompressorListItem, Variant::INT, allow);
        BIND_PROPERTY(MaszynaMoverCompressorListItem, Variant::INT, speed_factor);
        BIND_PROPERTY(MaszynaMoverCompressorListItem, Variant::INT, min_pressure_factor);
        BIND_PROPERTY(MaszynaMoverCompressorListItem, Variant::INT, max_pressure_factor);
    }
} // namespace godot
