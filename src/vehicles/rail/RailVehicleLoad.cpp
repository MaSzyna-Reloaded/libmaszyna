#include "RailVehicleLoad.hpp"

namespace godot {
    const char *RailVehicleLoad::load_exchange_finished_signal = "load_exchange_finished";

    void RailVehicleLoad::_bind_methods() {
        ADD_SIGNAL(MethodInfo(load_exchange_finished_signal));
        BIND_PROPERTY_W_HINT_RES_ARRAY(
                RailVehicleLoad, Variant::ARRAY, load_list, PROPERTY_HINT_ARRAY_TYPE, "RailVehicleLoadListItem")
        BIND_PROPERTY_W_HINT(RailVehicleLoad, Variant::INT, load_unit, PROPERTY_HINT_ENUM, "Tons,Pieces");
        BIND_PROPERTY(RailVehicleLoad, Variant::FLOAT, overload_factor);
        BIND_PROPERTY(RailVehicleLoad, Variant::FLOAT, load_speed);
        BIND_PROPERTY(RailVehicleLoad, Variant::FLOAT, unload_speed);
        BIND_PROPERTY(RailVehicleLoad, Variant::FLOAT, max_load);
        BIND_PROPERTY_ARRAY(RailVehicleLoad, minimum_load_offsets);
        BIND_PROPERTY_ARRAY(RailVehicleLoad, accepted_loads);
        BIND_ENUM_CONSTANT(LOAD_UNIT_TONS);
        BIND_ENUM_CONSTANT(LOAD_UNIT_PIECES);
        BIND_ENUM_CONSTANT(PLATFORM_SIDE_LEFT);
        BIND_ENUM_CONSTANT(PLATFORM_SIDE_RIGHT);
        BIND_ENUM_CONSTANT(PLATFORM_SIDE_BOTH);
        ClassDB::bind_method(D_METHOD("load_add", "amount", "side", "load_name"), &RailVehicleLoad::load_add);
        ClassDB::bind_method(D_METHOD("load_remove", "amount", "side"), &RailVehicleLoad::load_remove);
        ClassDB::bind_method(D_METHOD("get_load_exchange_time"), &RailVehicleLoad::get_load_exchange_time);
        ClassDB::bind_method(D_METHOD("get_load_exchange_speed"), &RailVehicleLoad::get_load_exchange_speed);
        ClassDB::bind_method(D_METHOD("get_load_name"), &RailVehicleLoad::get_load_name);
        ClassDB::bind_method(D_METHOD("get_load_amount"), &RailVehicleLoad::get_load_amount);
    }
} // namespace godot
