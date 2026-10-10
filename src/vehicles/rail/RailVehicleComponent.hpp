#pragma once
#include "RailVehicleController.hpp"
#include "vehicles/base/VehicleComponent.hpp"

namespace godot {
    /* A component only a railway vehicle has - a brake system, couplers, an engine of the
     * original's kinds, a master controller, the alerter. It belongs to a RailVehicleController. */
    class RailVehicleComponent : public VehicleComponent {
            GDCLASS(RailVehicleComponent, VehicleComponent);

        protected:
            static void _bind_methods() {}

        public:
            /* The railway vehicle this component belongs to */
            RailVehicleController *get_rail_vehicle_controller() const {
                return Object::cast_to<RailVehicleController>(train_controller_node);
            }
    };
} // namespace godot
