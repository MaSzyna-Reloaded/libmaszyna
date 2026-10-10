#pragma once
#include "vehicles/base/VehicleController.hpp"
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* What simulates vehicles - registered with VehicleServer under a name a vehicle's controller
     * names (VehicleController::implementation). VehicleServer hands it the step of its own
     * vehicles, all at once: how they are stepped together - in which phases, with what
     * sub-steps - is the implementation's business. */
    class VehicleImplementationServer : public Object {
            GDCLASS(VehicleImplementationServer, Object)

        protected:
            static void _bind_methods() {}

        public:
            /* One frame of p_vehicles, the vehicles VehicleServer holds for this implementation,
             * and p_controllers, theirs at the same index */
            virtual void stepping_advance(
                    const Vector<RID> &p_vehicles, const Vector<Ref<VehicleController>> &p_controllers,
                    double p_delta) {}
    };
} // namespace godot
