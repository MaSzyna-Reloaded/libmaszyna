#include "CompressorListItem.hpp"

namespace godot {
    void CompressorListItem::_bind_methods() {
        BIND_PROPERTY(
                Variant::INT, "allow", "allow", &CompressorListItem::set_allow, &CompressorListItem::get_allow,
                "allow");
        BIND_PROPERTY(
                Variant::INT, "speed_factor", "speed_factor", &CompressorListItem::set_speed_factor,
                &CompressorListItem::get_speed_factor, "speed_factor");
        BIND_PROPERTY(
                Variant::INT, "min_pressure_factor", "min_pressure_factor", &CompressorListItem::set_min_pressure_factor,
                &CompressorListItem::get_min_pressure_factor, "min_pressure_factor");
        BIND_PROPERTY(
                Variant::INT, "max_pressure_factor", "max_pressure_factor", &CompressorListItem::set_max_pressure_factor,
                &CompressorListItem::get_max_pressure_factor, "max_pressure_factor");
    }
} // namespace godot
