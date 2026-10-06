#pragma once
#include "vehicles/base/VehicleComponentType.hpp"

namespace godot {
    /* What only a railway vehicle can be asked for, numbered on from the kinds every vehicle has
     * (VehicleComponentType::COMPONENT_TYPE_MAX), so the two never name the same component:
     * `RailVehicleServer.vehicle_component_get(rid, RailVehicleComponentType.COMPONENT_BRAKES)`.
     * A class of its own for the reasons VehicleComponentType is one. */
    class RailVehicleComponentType : public Object {
            GDCLASS(RailVehicleComponentType, Object)

        protected:
            static void _bind_methods();

        public:
            enum Type {
                COMPONENT_BRAKES = VehicleComponentType::COMPONENT_TYPE_MAX,
                COMPONENT_SPRING_BRAKE,
                COMPONENT_EP_ED_BRAKE,
                COMPONENT_BUFFERS,
                COMPONENT_SPEED_CONTROL,
                COMPONENT_SWITCHES,
                COMPONENT_AI_HINTS,
                COMPONENT_SECURITY,
                COMPONENT_UNIVERSAL_CONTROLLER,
                COMPONENT_MASTER_CONTROLLER,
                COMPONENT_POWER_SUPPLY,
                COMPONENT_ENGINE_POWER_SOURCE,
            };
    };
} // namespace godot

VARIANT_ENUM_CAST(RailVehicleComponentType::Type);
