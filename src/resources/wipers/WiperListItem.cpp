#include "WiperListItem.hpp"

namespace godot {
    void WiperListItem::_bind_methods() {
        BIND_PROPERTY(
                Variant::INT, "wiper_mask", "wiper_mask", &WiperListItem::set_wiper_mask,
                &WiperListItem::get_wiper_mask, "wiper_mask");
        BIND_PROPERTY(
                Variant::FLOAT, "transit_time", "transit_time", &WiperListItem::set_transit_time,
                &WiperListItem::get_transit_time, "transit_time");
        BIND_PROPERTY(
                Variant::FLOAT, "period", "period", &WiperListItem::set_period, &WiperListItem::get_period, "period");
        BIND_PROPERTY(
                Variant::FLOAT, "return_delay", "return_delay", &WiperListItem::set_return_delay,
                &WiperListItem::get_return_delay, "return_delay");
    }
} // namespace godot
