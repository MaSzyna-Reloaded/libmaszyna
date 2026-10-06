#include "RailVehicleWipers.hpp"
#include <algorithm>

namespace godot {
    void RailVehicleWipers::_bind_methods() {
        ClassDB::bind_method(D_METHOD("switch_increase"), &RailVehicleWipers::switch_increase);
        ClassDB::bind_method(D_METHOD("switch_decrease"), &RailVehicleWipers::switch_decrease);
        BIND_PROPERTY(RailVehicleWipers, Variant::FLOAT, angle);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, default_position);
        BIND_PROPERTY(RailVehicleWipers, Variant::INT, wiper_count);
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleWipers, Variant::ARRAY, positions, PROPERTY_HINT_TYPE_STRING, "RailVehicleWiperListItem");

        ClassDB::bind_method(D_METHOD("get_switch_position"), &RailVehicleWipers::get_switch_position);
        ClassDB::bind_method(D_METHOD("get_sweep_positions"), &RailVehicleWipers::get_sweep_positions);
    }

    void RailVehicleWipers::_register_commands() {
        register_command("wipers_switch_increase", Callable(this, "switch_increase"));
        register_command("wipers_switch_decrease", Callable(this, "switch_decrease"));
    }

    void RailVehicleWipers::_unregister_commands() {
        unregister_command("wipers_switch_increase");
        unregister_command("wipers_switch_decrease");
    }
} // namespace godot
