#include "RailVehicleHorns.hpp"

namespace godot {
    void RailVehicleHorns::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_horn_low", "state"), &RailVehicleHorns::set_horn_low);
        ClassDB::bind_method(D_METHOD("set_horn_high", "state"), &RailVehicleHorns::set_horn_high);
        ClassDB::bind_method(D_METHOD("set_whistle", "state"), &RailVehicleHorns::set_whistle);

        ClassDB::bind_method(D_METHOD("get_low_pressed"), &RailVehicleHorns::get_low_pressed);
        ClassDB::bind_method(D_METHOD("get_high_pressed"), &RailVehicleHorns::get_high_pressed);
        ClassDB::bind_method(D_METHOD("get_whistle_pressed"), &RailVehicleHorns::get_whistle_pressed);
        ClassDB::bind_method(D_METHOD("get_low_active"), &RailVehicleHorns::get_low_active);
        ClassDB::bind_method(D_METHOD("get_high_active"), &RailVehicleHorns::get_high_active);
        ClassDB::bind_method(D_METHOD("get_whistle_active"), &RailVehicleHorns::get_whistle_active);
        ClassDB::bind_method(D_METHOD("get_horn"), &RailVehicleHorns::get_horn);
    }

    void RailVehicleHorns::_register_commands() {
        register_command("horn_low", Callable(this, "set_horn_low"));
        register_command("horn_high", Callable(this, "set_horn_high"));
        register_command("whistle", Callable(this, "set_whistle"));
    }

    void RailVehicleHorns::_unregister_commands() {
        unregister_command("horn_low");
        unregister_command("horn_high");
        unregister_command("whistle");
    }
} // namespace godot
