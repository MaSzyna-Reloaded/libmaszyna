#include "RailVehicleWheels.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/core/math.hpp>

namespace godot {
    /* RailVehicleServer's track-offset distance is rear-relative (see its process_movement()),
     * so a positive distance samples toward the vehicle's rear - the sign here is deliberate and
     * was confirmed live: getting it wrong flips the whole vehicle the moment it starts moving
     * (test_rail_vehicle_idle_orientation_regression.gd). */
    Transform3D RailVehicleWheels::get_bogie_transform(const Bogie p_bogie) const {
        const VehicleController *controller = train_controller_node;
        RailVehicleServer *server = RailVehicleServer::get_instance();
        if (controller == nullptr || server == nullptr) {
            return Transform3D();
        }
        const double half_spacing = 0.5 * get_bogie_pivot_spacing();
        return server->vehicle_get_transform_at_distance(
                controller->get_rid(), p_bogie == BOGIE_FRONT ? -half_spacing : half_spacing);
    }

    void RailVehicleWheels::_bind_methods() {
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, powered_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, front_rolling_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, rear_rolling_wheel_diameter);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, axle_inertial_moment);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, track_width);
        BIND_PROPERTY(RailVehicleWheels, Variant::STRING, axle_arrangement);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_axle_spacing);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, bogie_pivot_spacing);
        BIND_PROPERTY(RailVehicleWheels, Variant::FLOAT, minimum_curve_radius);
        BIND_PROPERTY_W_HINT(RailVehicleWheels, Variant::INT, bearing_type, PROPERTY_HINT_ENUM, "Slide,Roll");

        ClassDB::bind_method(D_METHOD("get_bogie_transform", "bogie"), &RailVehicleWheels::get_bogie_transform);
        BIND_ENUM_CONSTANT(BOGIE_FRONT);
        BIND_ENUM_CONSTANT(BOGIE_REAR);
        BIND_ENUM_CONSTANT(BEARING_TYPE_SLIDE);
        BIND_ENUM_CONSTANT(BEARING_TYPE_ROLL);

        ClassDB::bind_method(D_METHOD("get_angle_front_deg"), &RailVehicleWheels::get_angle_front_deg);
        ClassDB::bind_method(D_METHOD("get_angle_powered_deg"), &RailVehicleWheels::get_angle_powered_deg);
        ClassDB::bind_method(D_METHOD("get_angle_rear_deg"), &RailVehicleWheels::get_angle_rear_deg);
        ClassDB::bind_method(D_METHOD("get_rotation_speed_rps"), &RailVehicleWheels::get_rotation_speed_rps);
        ClassDB::bind_method(
                D_METHOD("get_rotation_acceleration_rps2"), &RailVehicleWheels::get_rotation_acceleration_rps2);
        ClassDB::bind_method(D_METHOD("get_slipping"), &RailVehicleWheels::get_slipping);
        ClassDB::bind_method(D_METHOD("get_flat"), &RailVehicleWheels::get_flat);
    }
} // namespace godot
