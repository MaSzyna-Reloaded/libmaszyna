#include "RailVehicleUniversalController.hpp"
#include <algorithm>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void RailVehicleUniversalController::_bind_methods() {
        BIND_PROPERTY(RailVehicleUniversalController, Variant::BOOL, integrated_brake_pn);
        BIND_PROPERTY(RailVehicleUniversalController, Variant::BOOL, integrated_brake);
        BIND_PROPERTY(RailVehicleUniversalController, Variant::BOOL, integrated_local_brake);
        BIND_PROPERTY(RailVehicleUniversalController, Variant::INT, selector_position);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleUniversalController, Variant::ARRAY, positions, PROPERTY_HINT_TYPE_STRING,
                "RailVehicleUniversalControllerListItem");
    }
} // namespace godot
