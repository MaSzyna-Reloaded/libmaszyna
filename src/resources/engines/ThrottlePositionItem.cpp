#include "ThrottlePositionItem.hpp"

namespace godot {
    void ThrottlePositionItem::_bind_methods() {
        BIND_PROPERTY(
                Variant::INT, "throttle_position", "throttle_position", &ThrottlePositionItem::set_throttle_position,
                &ThrottlePositionItem::get_throttle_position, "throttle_position");
        BIND_PROPERTY(
                Variant::FLOAT, "fuel_dose", "fuel_dose", &ThrottlePositionItem::set_fuel_dose,
                &ThrottlePositionItem::get_fuel_dose, "fuel_dose");
        BIND_PROPERTY_W_HINT(
                Variant::INT, "clutch_behavior", "clutch_behavior", &ThrottlePositionItem::set_clutch_behavior,
                &ThrottlePositionItem::get_clutch_behavior, "clutch_behavior", PROPERTY_HINT_ENUM,
                "None,HalfClutchMinRpm,FullClutchMaxRpm,HalfClutchThenFull,TwoThirdsClutch");

        BIND_ENUM_CONSTANT(CLUTCH_BEHAVIOR_NONE);
        BIND_ENUM_CONSTANT(CLUTCH_BEHAVIOR_HALF_CLUTCH_MIN_RPM);
        BIND_ENUM_CONSTANT(CLUTCH_BEHAVIOR_FULL_CLUTCH_MAX_RPM);
        BIND_ENUM_CONSTANT(CLUTCH_BEHAVIOR_HALF_CLUTCH_THEN_FULL);
        BIND_ENUM_CONSTANT(CLUTCH_BEHAVIOR_TWO_THIRDS_CLUTCH);
    }
} // namespace godot
