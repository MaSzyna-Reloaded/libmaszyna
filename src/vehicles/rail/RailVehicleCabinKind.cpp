#include "RailVehicleCabinKind.hpp"

namespace godot {
    void RailVehicleCabinKind::_bind_methods() {
        BIND_ENUM_CONSTANT(RAIL_VEHICLE_CABIN_NONE);
        BIND_ENUM_CONSTANT(RAIL_VEHICLE_CABIN_FRONT);
        BIND_ENUM_CONSTANT(RAIL_VEHICLE_CABIN_MACHINE);
        BIND_ENUM_CONSTANT(RAIL_VEHICLE_CABIN_REAR);
    }
} // namespace godot
