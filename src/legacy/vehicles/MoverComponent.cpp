#include "MoverComponent.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"

namespace godot {
    void MoverComponent::take_mover(const ObjectID &p_implementation, const RID &p_vehicle) {
        const MaszynaMoverVehicleServer *implementation =
                Object::cast_to<MaszynaMoverVehicleServer>(ObjectDB::get_instance(p_implementation));
        mover = implementation != nullptr ? implementation->mover_get(p_vehicle) : nullptr;
    }
} // namespace godot
