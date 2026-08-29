#pragma once
#include "macros.hpp"
#include <godot_cpp/classes/resource.hpp>

namespace godot {
    class ThrottlePositionItem : public Resource {
            GDCLASS(ThrottlePositionItem, Resource);

        public:
            /* C: Zachowanie sprzegla i regulatora */
            enum ClutchBehavior {
                CLUTCH_BEHAVIOR_NONE,
                CLUTCH_BEHAVIOR_HALF_CLUTCH_MIN_RPM,
                CLUTCH_BEHAVIOR_FULL_CLUTCH_MAX_RPM,
                CLUTCH_BEHAVIOR_HALF_CLUTCH_THEN_FULL,
                CLUTCH_BEHAVIOR_TWO_THIRDS_CLUTCH,
            };

            static void _bind_methods();
            MAKE_MEMBER_GS(int, throttle_position, 0);
            MAKE_MEMBER_GS(double, fuel_dose, 0.0);
            MAKE_MEMBER_GS_NR(ClutchBehavior, clutch_behavior, CLUTCH_BEHAVIOR_NONE);
    };
} // namespace godot
VARIANT_ENUM_CAST(ThrottlePositionItem::ClutchBehavior)
