#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"

#include <godot_cpp/core/object_id.hpp>
#include <godot_cpp/variant/rid.hpp>

namespace godot {
    /* What every Mover* component shares: the Mover of the vehicle it belongs to. Not a Godot
     * class - a Mover* component inherits it next to its RailVehicle* interface, and takes the
     * Mover in its own _implementation_changed() (VehicleComponent::attach_implementation()):
     *
     *     void _implementation_changed() override {
     *         take_mover(get_implementation(), ...the vehicle's RID...);
     *     }
     *
     * The pointer has one lifetime: the implementation hands the implementation over once the
     * vehicle's Mover exists (MoverRailVehicleController::_initialize_simulation()) and takes it
     * back before the Mover is freed (release()), and take_mover() follows both. Between the two
     * the Mover is read straight, with no lookup. */
    class MoverComponent {
        private:
            TMoverParameters *mover = nullptr;

        public:
            virtual ~MoverComponent() = default;

            /* The Mover of p_vehicle from the MaszynaMoverVehicleServer p_implementation, or none
             * when there is no implementation any more */
            void take_mover(const ObjectID &p_implementation, const RID &p_vehicle);

            /* The vehicle's Mover, or null while there is none. */
            TMoverParameters *get_mover() const {
                return mover;
            }
    };
} // namespace godot
