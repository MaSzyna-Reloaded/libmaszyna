#include "RailVehicleLightListItem.hpp"
#include "macros.hpp"
namespace godot {
    void RailVehicleLightListItem::_bind_methods() { // Cabin A
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_head_light, "cabin_a");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_left_white_signal, "cabin_a/left");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_left_red_signal, "cabin_a/left");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_right_white_signal, "cabin_a/right");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_right_red_signal, "cabin_a/right");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_end_signals, "cabin_a");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_left_auxiliary_light, "cabin_a/left");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_a_right_auxiliary_light, "cabin_a/right");
        // Cabin B
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_head_light, "cabin_b");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_left_white_signal, "cabin_b/left");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_left_red_signal, "cabin_b/left");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_right_white_signal, "cabin_b/right");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_right_red_signal, "cabin_b/right");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_end_signals, "cabin_b");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_left_auxiliary_light, "cabin_b/left");
        BIND_PROPERTY(RailVehicleLightListItem, Variant::BOOL, cabin_b_right_auxiliary_light, "cabin_b/right");
    }
} // namespace godot
