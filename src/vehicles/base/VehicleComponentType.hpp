#pragma once
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>

namespace godot {
    /* What any vehicle can be asked for - a kind of vehicle adds its own kinds after
     * COMPONENT_TYPE_MAX (RailVehicleComponentType).
     *
     * A consumer names the kind, not the implementation:
     * `VehicleServer.vehicle_component_get(rid, VehicleComponentType.COMPONENT_ENGINE)` answers
     * with a RailVehicleEngine whether the vehicle is diesel or electric.
     *
     * It is a class of its own, holding nothing, for two reasons: GDScript only sees an enum's
     * constants as members of a registered class, and the component and the vehicle include each
     * other, so neither of them can own it. */
    class VehicleComponentType : public Object {
            GDCLASS(VehicleComponentType, Object)

        protected:
            static void _bind_methods();

        public:
            enum Type {
                COMPONENT_NONE,
                COMPONENT_GENERIC,
                COMPONENT_ENGINE,
                COMPONENT_HEATING,
                COMPONENT_LIGHTING,
                COMPONENT_LOAD,
                COMPONENT_DOORS,
                COMPONENT_HORNS,
                COMPONENT_WIPERS,
                COMPONENT_RADIO,
                COMPONENT_WHEELS,
                /* Where a kind of vehicle's own kinds start */
                COMPONENT_TYPE_MAX,
            };
    };
} // namespace godot

VARIANT_ENUM_CAST(VehicleComponentType::Type);
