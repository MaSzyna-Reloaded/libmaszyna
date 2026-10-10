#include "RailVehicleDoors.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *RailVehicleDoors::doors_opened_signal = "doors_opened";
    const char *RailVehicleDoors::doors_closed_signal = "doors_closed";

    void RailVehicleDoors::_bind_methods() {
        ADD_SIGNAL(MethodInfo(doors_opened_signal, PropertyInfo(Variant::INT, "side")));
        ADD_SIGNAL(MethodInfo(doors_closed_signal, PropertyInfo(Variant::INT, "side")));
        BIND_PROPERTY_W_HINT(RailVehicleDoors, Variant::INT, type, PROPERTY_HINT_ENUM, "Shift,Rotate,Fold,Plug");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, open_time, "open");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, open_speed, "open");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, close_speed, "close");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, max_shift);
        BIND_PROPERTY_W_HINT(
                RailVehicleDoors, Variant::INT, open_method, "open", PROPERTY_HINT_ENUM,
                "Passenger,Automatic,Driver,Conductor,Mixed");
        BIND_PROPERTY_W_HINT(
                RailVehicleDoors, Variant::INT, close_method, "close", PROPERTY_HINT_ENUM,
                "Passenger,Automatic,Driver,Conductor,Mixed");
        BIND_PROPERTY_W_HINT(RailVehicleDoors, Variant::INT, voltage, PROPERTY_HINT_ENUM, "Automatic,0V,12V,24V,110V");
        BIND_PROPERTY(RailVehicleDoors, Variant::BOOL, close_warning, "close");
        BIND_PROPERTY(RailVehicleDoors, Variant::BOOL, close_auto_close_warning, "close");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, open_delay, "open");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, close_delay, "close");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, open_with_permit, "open");
        BIND_PROPERTY(RailVehicleDoors, Variant::BOOL, has_lock);
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, max_shift_plug);
        BIND_PROPERTY_ARRAY(RailVehicleDoors, permit_list, "permit");
        BIND_PROPERTY(RailVehicleDoors, Variant::INT, permit_default, "permit");
        BIND_PROPERTY(RailVehicleDoors, Variant::BOOL, close_auto_close_remote, "close");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, close_auto_close_velocity, "close");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, platform_max_speed, "platform");
        BIND_PROPERTY_W_HINT(
                RailVehicleDoors, Variant::INT, platform_type, "platform", PROPERTY_HINT_ENUM, "Shift,Rotate");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, platform_max_shift, "platform");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, platform_speed, "platform");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, mirror_max_shift, "mirror");
        BIND_PROPERTY(RailVehicleDoors, Variant::FLOAT, mirror_close_velocity, "mirror");
        BIND_PROPERTY(RailVehicleDoors, Variant::BOOL, permit_required, "permit");
        BIND_PROPERTY_W_HINT(
                RailVehicleDoors, Variant::INT, permit_light_blinking, "permit", PROPERTY_HINT_ENUM,
                "Continuous light,Flashing on permission w/step,Flashing on permission,Flashing always");
        ClassDB::bind_method(D_METHOD("next_permit_preset"), &RailVehicleDoors::next_permit_preset);
        ClassDB::bind_method(D_METHOD("previous_permit_preset"), &RailVehicleDoors::previous_permit_preset);
        ClassDB::bind_method(D_METHOD("permit_step", "state"), &RailVehicleDoors::permit_step);
        ClassDB::bind_method(D_METHOD("permit_doors", "state", "side"), &RailVehicleDoors::permit_doors);
        ClassDB::bind_method(D_METHOD("operate_doors", "state", "side"), &RailVehicleDoors::operate_doors);
        ClassDB::bind_method(
                D_METHOD("operate_doors_locally", "state", "side"), &RailVehicleDoors::operate_doors_locally);
        ClassDB::bind_method(D_METHOD("door_lock", "state"), &RailVehicleDoors::door_lock);
        ClassDB::bind_method(D_METHOD("door_remote_control", "state"), &RailVehicleDoors::door_remote_control);
        ClassDB::bind_method(D_METHOD("signal_departure", "state"), &RailVehicleDoors::signal_departure);
        ClassDB::bind_method(D_METHOD("forbid_mirrors", "state"), &RailVehicleDoors::forbid_mirrors);


        BIND_ENUM_CONSTANT(PERMIT_LIGHT_CONTINUOUS);
        BIND_ENUM_CONSTANT(PERMIT_LIGHT_FLASHING_ON_PERMISSION_WITH_STEP);
        BIND_ENUM_CONSTANT(PERMIT_LIGHT_FLASHING_ON_PERMISSION);
        BIND_ENUM_CONSTANT(PERMIT_LIGHT_FLASHING_ALWAYS);

        BIND_ENUM_CONSTANT(PLATFORM_TYPE_SHIFT);
        BIND_ENUM_CONSTANT(PLATFORM_TYPE_ROTATE);

        BIND_ENUM_CONSTANT(SIDE_RIGHT)
        BIND_ENUM_CONSTANT(SIDE_LEFT)

        BIND_ENUM_CONSTANT(CONTROLS_PASSENGER)
        BIND_ENUM_CONSTANT(CONTROLS_AUTOMATIC)
        BIND_ENUM_CONSTANT(CONTROLS_DRIVER)
        BIND_ENUM_CONSTANT(CONTROLS_CONDUCTOR)
        BIND_ENUM_CONSTANT(CONTROLS_MIXED)

        BIND_ENUM_CONSTANT(VOLTAGE_AUTO)
        BIND_ENUM_CONSTANT(VOLTAGE_0)
        BIND_ENUM_CONSTANT(VOLTAGE_12)
        BIND_ENUM_CONSTANT(VOLTAGE_24)
        BIND_ENUM_CONSTANT(VOLTAGE_110)

        BIND_ENUM_CONSTANT(TYPE_SHIFT)
        BIND_ENUM_CONSTANT(TYPE_ROTATE)
        BIND_ENUM_CONSTANT(TYPE_FOLD)
        BIND_ENUM_CONSTANT(TYPE_PLUG)

        ClassDB::bind_method(D_METHOD("get_permit_preset"), &RailVehicleDoors::get_permit_preset);
        ClassDB::bind_method(D_METHOD("get_locked"), &RailVehicleDoors::get_locked);
        ClassDB::bind_method(D_METHOD("get_lock_enabled"), &RailVehicleDoors::get_lock_enabled);
        ClassDB::bind_method(D_METHOD("get_step_enabled"), &RailVehicleDoors::get_step_enabled);
        ClassDB::bind_method(D_METHOD("get_open_control"), &RailVehicleDoors::get_open_control);
        ClassDB::bind_method(D_METHOD("get_left_open"), &RailVehicleDoors::get_left_open);
        ClassDB::bind_method(D_METHOD("get_left_closed"), &RailVehicleDoors::get_left_closed);
        ClassDB::bind_method(D_METHOD("get_left_door_closed"), &RailVehicleDoors::get_left_door_closed);
        ClassDB::bind_method(D_METHOD("get_left_open_permit"), &RailVehicleDoors::get_left_open_permit);
        ClassDB::bind_method(D_METHOD("get_left_local_open"), &RailVehicleDoors::get_left_local_open);
        ClassDB::bind_method(D_METHOD("get_left_remote_open"), &RailVehicleDoors::get_left_remote_open);
        ClassDB::bind_method(D_METHOD("get_left_position"), &RailVehicleDoors::get_left_position);
        ClassDB::bind_method(D_METHOD("get_left_position_normalized"), &RailVehicleDoors::get_left_position_normalized);
        ClassDB::bind_method(D_METHOD("get_left_operating"), &RailVehicleDoors::get_left_operating);
        ClassDB::bind_method(D_METHOD("get_left_step_position"), &RailVehicleDoors::get_left_step_position);
        ClassDB::bind_method(D_METHOD("get_left_step_operating"), &RailVehicleDoors::get_left_step_operating);
        ClassDB::bind_method(D_METHOD("get_right_open"), &RailVehicleDoors::get_right_open);
        ClassDB::bind_method(D_METHOD("get_right_closed"), &RailVehicleDoors::get_right_closed);
        ClassDB::bind_method(D_METHOD("get_right_door_closed"), &RailVehicleDoors::get_right_door_closed);
        ClassDB::bind_method(D_METHOD("get_right_open_permit"), &RailVehicleDoors::get_right_open_permit);
        ClassDB::bind_method(D_METHOD("get_right_local_open"), &RailVehicleDoors::get_right_local_open);
        ClassDB::bind_method(D_METHOD("get_right_remote_open"), &RailVehicleDoors::get_right_remote_open);
        ClassDB::bind_method(D_METHOD("get_right_position"), &RailVehicleDoors::get_right_position);
        ClassDB::bind_method(
                D_METHOD("get_right_position_normalized"), &RailVehicleDoors::get_right_position_normalized);
        ClassDB::bind_method(D_METHOD("get_right_operating"), &RailVehicleDoors::get_right_operating);
        ClassDB::bind_method(D_METHOD("get_right_step_position"), &RailVehicleDoors::get_right_step_position);
        ClassDB::bind_method(D_METHOD("get_right_step_operating"), &RailVehicleDoors::get_right_step_operating);
        ClassDB::bind_method(D_METHOD("get_mirror_left_position"), &RailVehicleDoors::get_mirror_left_position);
        ClassDB::bind_method(D_METHOD("get_mirror_right_position"), &RailVehicleDoors::get_mirror_right_position);
        ClassDB::bind_method(D_METHOD("get_mirrors_forbidden"), &RailVehicleDoors::get_mirrors_forbidden);
        ClassDB::bind_method(D_METHOD("get_departure_signal"), &RailVehicleDoors::get_departure_signal);
        ClassDB::bind_method(
                D_METHOD("get_departure_signal_sounding"), &RailVehicleDoors::get_departure_signal_sounding);
        ClassDB::bind_method(D_METHOD("get_remote_only"), &RailVehicleDoors::get_remote_only);
    }

    void RailVehicleDoors::_register_commands() {
        register_command("doors_next_permit_preset", Callable(this, "next_permit_preset"));
        register_command("doors_previous_permit_preset", Callable(this, "previous_permit_preset"));
        register_command("doors_permit_step", Callable(this, "permit_step"));
        register_command("doors_left_permit", Callable(this, "permit_doors").bind(SIDE_LEFT));
        register_command("doors_right_permit", Callable(this, "permit_doors").bind(SIDE_RIGHT));
        register_command("doors_left", Callable(this, "operate_doors").bind(SIDE_LEFT));
        register_command("doors_right", Callable(this, "operate_doors").bind(SIDE_RIGHT));
        register_command("doors_left_local", Callable(this, "operate_doors_locally").bind(SIDE_LEFT));
        register_command("doors_right_local", Callable(this, "operate_doors_locally").bind(SIDE_RIGHT));
        register_command("doors_lock", Callable(this, "door_lock"));
        register_command("doors_remote_control", Callable(this, "door_remote_control"));
        register_command("doors_departure_signal", Callable(this, "signal_departure"));
        register_command("mirrors_forbid", Callable(this, "forbid_mirrors"));
    }

    void RailVehicleDoors::_unregister_commands() {
        unregister_command("doors_next_permit_preset");
        unregister_command("doors_previous_permit_preset");
        unregister_command("doors_permit_step");
        unregister_command("doors_left_permit");
        unregister_command("doors_right_permit");
        unregister_command("doors_left");
        unregister_command("doors_right");
        unregister_command("doors_left_local");
        unregister_command("doors_right_local");
        unregister_command("doors_lock");
        unregister_command("doors_remote_control");
        unregister_command("doors_departure_signal");
        unregister_command("mirrors_forbid");
    }
} // namespace godot
