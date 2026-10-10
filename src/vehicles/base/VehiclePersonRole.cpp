#include "VehiclePersonRole.hpp"

namespace godot {
    void VehiclePersonRole::_bind_methods() {
        BIND_ENUM_CONSTANT(VEHICLE_PERSON_ROLE_ANY);
        BIND_ENUM_CONSTANT(VEHICLE_PERSON_ROLE_DRIVER);
        BIND_ENUM_CONSTANT(VEHICLE_PERSON_ROLE_OBSERVER);
    }
} // namespace godot
