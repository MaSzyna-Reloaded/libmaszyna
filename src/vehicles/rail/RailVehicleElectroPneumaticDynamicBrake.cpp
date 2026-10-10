#include "RailVehicleElectroPneumaticDynamicBrake.hpp"

namespace godot {
    void RailVehicleElectroPneumaticDynamicBrake::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("set_ep_brake_force", "value"), &RailVehicleElectroPneumaticDynamicBrake::set_ep_brake_force);
        ClassDB::bind_method(
                D_METHOD("switch_ep_fuse", "value"), &RailVehicleElectroPneumaticDynamicBrake::switch_ep_fuse);

        BIND_PROPERTY_W_HINT(
                RailVehicleElectroPneumaticDynamicBrake, Variant::INT, coupler_check, PROPERTY_HINT_ENUM,
                "None,Front,Back");
        BIND_PROPERTY(
                RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, electro_pneumatic_min_regenerative_braking,
                "electro_pneumatic");
        BIND_PROPERTY(
                RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT,
                electro_pneumatic_max_ep_brake_engagement_speed, "electro_pneumatic");
        BIND_PROPERTY(
                RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, electro_pneumatic_brake_delay,
                "electro_pneumatic");
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::BOOL, ep_brake_fuse);
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, blending_max_velocity, "blending");
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, blending_min_velocity, "blending");
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, blending_reference_velocity, "blending");
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, blending_max_deceleration, "blending");
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::BOOL, blending_velocity_correction, "blending");
        BIND_PROPERTY(RailVehicleElectroPneumaticDynamicBrake, Variant::BOOL, blending_load_correction, "blending");
        BIND_PROPERTY(
                RailVehicleElectroPneumaticDynamicBrake, Variant::FLOAT, blending_min_ed_brake_request, "blending");

        BIND_ENUM_CONSTANT(NONE)
        BIND_ENUM_CONSTANT(FRONT)
        BIND_ENUM_CONSTANT(BACK)

        ClassDB::bind_method(
                D_METHOD("get_ed_braking_ep_delay"), &RailVehicleElectroPneumaticDynamicBrake::get_ed_braking_ep_delay);
        ClassDB::bind_method(
                D_METHOD("get_ep_max_brake_engagement_speed"),
                &RailVehicleElectroPneumaticDynamicBrake::get_ep_max_brake_engagement_speed);
        ClassDB::bind_method(
                D_METHOD("get_ep_min_regenerative_braking"),
                &RailVehicleElectroPneumaticDynamicBrake::get_ep_min_regenerative_braking);
        ClassDB::bind_method(D_METHOD("get_ep_force"), &RailVehicleElectroPneumaticDynamicBrake::get_ep_force);
        ClassDB::bind_method(D_METHOD("get_ep_fuse"), &RailVehicleElectroPneumaticDynamicBrake::get_ep_fuse);
    }

    void RailVehicleElectroPneumaticDynamicBrake::_register_commands() {
        register_command("set_ep_brake_force", Callable(this, "set_ep_brake_force"));
        register_command("switch_ep_fuse", Callable(this, "switch_ep_fuse"));
        VehicleComponent::_register_commands();
    }

    void RailVehicleElectroPneumaticDynamicBrake::_unregister_commands() {
        unregister_command("set_ep_brake_force");
        unregister_command("switch_ep_fuse");
        VehicleComponent::_unregister_commands();
    }
} // namespace godot
