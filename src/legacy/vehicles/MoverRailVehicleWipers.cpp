#include "MoverRailVehicleWipers.hpp"
#include "legacy/vehicles/MoverBackend.hpp"

namespace godot {
    void MoverRailVehicleWipers::_bind_methods() {}

    bool MoverRailVehicleWipers::_get_battery_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->Battery;
    }
} // namespace godot
