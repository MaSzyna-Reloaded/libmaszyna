#include "RailVehicleBuffCoupl.hpp"

namespace godot {
    void RailVehicleBuffCoupl::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                RailVehicleBuffCoupl, Variant::INT, coupler_type, "coupler", PROPERTY_HINT_ENUM,
                "Automatic,Screw,Chain,Bare,Articulated");

        // Buffer properties
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, buffer_stiffness_k, "buffer");
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, buffer_max_compression_tolerance, "buffer");
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, buffer_max_tension_tolerance, "buffer");

        // Coupler properties
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, coupler_stiffness_k, "coupler");
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, coupler_max_compression_tolerance, "coupler");
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, coupler_max_tension_tolerance, "coupler");

        // Damping
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::FLOAT, damping_beta);

        // Coupler capability flags and control
        BIND_PROPERTY_W_HINT(
                RailVehicleBuffCoupl, Variant::INT, allowed_flag, PROPERTY_HINT_FLAGS,
                "Mechanical,Brake pipe,Multiple control,High voltage,Passage,Air 8 bar,Heating,Fixed coupling lock,24V "
                "electric cable,110V electric cable,3+400V electric cable");
        BIND_PROPERTY_W_HINT(
                RailVehicleBuffCoupl, Variant::INT, automatic_flag, PROPERTY_HINT_FLAGS,
                "Mechanical,Brake pipe,Multiple control,High voltage,Passage,Air 8 bar,Heating,Fixed coupling lock,24V "
                "electric cable,110V electric cable,3+400V electric cable");
        BIND_PROPERTY_W_HINT(RailVehicleBuffCoupl, Variant::INT, power_flag, PROPERTY_HINT_FLAGS, "24V,110V,3x400V");
        BIND_PROPERTY_W_HINT(
                RailVehicleBuffCoupl, Variant::INT, power_coupling, PROPERTY_HINT_FLAGS,
                "Mechanical,Brake pipe,Multiple control,High voltage,Passage,Air 8 bar,Heating,Fixed coupling lock,24V "
                "electric cable,110V electric cable,3+400V electric cable");
        BIND_PROPERTY(RailVehicleBuffCoupl, Variant::STRING, control_type);
        BIND_PROPERTY_W_HINT(
                RailVehicleBuffCoupl, Variant::INT, buffer_location, PROPERTY_HINT_ENUM, "Front,Back,Both");

        BIND_ENUM_CONSTANT(COUPLER_TYPE_AUTOMATIC)
        BIND_ENUM_CONSTANT(COUPLER_TYPE_SCREW)
        BIND_ENUM_CONSTANT(COUPLER_TYPE_CHAIN)
        BIND_ENUM_CONSTANT(COUPLER_TYPE_BARE)
        BIND_ENUM_CONSTANT(COUPLER_TYPE_ARTICULATED)

        BIND_ENUM_CONSTANT(BUFFER_LOCATION_FRONT)
        BIND_ENUM_CONSTANT(BUFFER_LOCATION_BACK)
        BIND_ENUM_CONSTANT(BUFFER_LOCATION_BOTH)

        ClassDB::bind_method(D_METHOD("get_coupler_max_force", "end"), &RailVehicleBuffCoupl::get_coupler_max_force);
    }
} // namespace godot
