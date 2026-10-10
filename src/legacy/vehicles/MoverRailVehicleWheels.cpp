#include "MoverRailVehicleWheels.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include <godot_cpp/core/math.hpp>

namespace godot {
    namespace {
        /* The diameter TMoverParameters keeps when the data gives none (MOVER.h:1595) */
        constexpr double DEFAULT_WHEEL_DIAMETER = 1.0;
        /* LoadFIZ_Wheels: "approximation formula by youby", an arbitrary constant (Mover.cpp:10388) */
        constexpr double AXLE_INERTIA_APPROXIMATION_FACTOR = 472.0;
    } // namespace

    void MoverRailVehicleWheels::_bind_methods() {}


    void MoverRailVehicleWheels::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        const double resolved_powered_wheel_diameter =
                get_powered_wheel_diameter() > 0.0 ? get_powered_wheel_diameter() : DEFAULT_WHEEL_DIAMETER;
        const double resolved_front_rolling_wheel_diameter = get_front_rolling_wheel_diameter() > 0.0
                                                                     ? get_front_rolling_wheel_diameter()
                                                                     : resolved_powered_wheel_diameter;
        const double resolved_rear_rolling_wheel_diameter = get_rear_rolling_wheel_diameter() > 0.0
                                                                    ? get_rear_rolling_wheel_diameter()
                                                                    : resolved_powered_wheel_diameter;

        p_mover->WheelDiameter = static_cast<float>(resolved_powered_wheel_diameter);
        p_mover->WheelDiameterL = static_cast<float>(resolved_front_rolling_wheel_diameter);
        if (p_mover->WheelDiameterL == 0.0) {
            // original LoadFIZ_Wheels workaround
            p_mover->WheelDiameterL = p_mover->WheelDiameter;
        }
        p_mover->WheelDiameterT = static_cast<float>(resolved_rear_rolling_wheel_diameter);
        if (p_mover->WheelDiameterT == 0.0) {
            // original LoadFIZ_Wheels workaround
            p_mover->WheelDiameterT = p_mover->WheelDiameter;
        }
        p_mover->TrackW = static_cast<float>(get_track_width());
        p_mover->AxleArangement = get_axle_arrangement().ascii().get_data();
        p_mover->NPoweredAxles = Maszyna::s2NPW(p_mover->AxleArangement);
        p_mover->NAxles = p_mover->NPoweredAxles + Maszyna::s2NNW(p_mover->AxleArangement);
        p_mover->BearingType = get_bearing_type() == BEARING_TYPE_SLIDE ? 0 : 1;
        p_mover->ADist = get_bogie_axle_spacing();
        p_mover->BDist = get_bogie_pivot_spacing();

        if (get_axle_inertial_moment() <= 0.0) {
            p_mover->AxleInertialMoment =
                    AXLE_INERTIA_APPROXIMATION_FACTOR / 4.0 * std::pow(p_mover->WheelDiameter, 4.0) * p_mover->NAxles;
            // Original engine: LoadFIZ_Wheels() overwrites the Mred LoadFIZ_Param() read (Mover.cpp:
            // 10248, 10390) - the controller applies its Param: values before the components
            p_mover->Mred = AXLE_INERTIA_APPROXIMATION_FACTOR * std::pow(p_mover->WheelDiameter, 2.0) * p_mover->NAxles;
        } else {
            p_mover->AxleInertialMoment = get_axle_inertial_moment();
        }
    }


    double MoverRailVehicleWheels::get_angle_front_deg() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? wheel_angle_front_deg : 0.0;
    }

    double MoverRailVehicleWheels::get_angle_powered_deg() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? wheel_angle_powered_deg : 0.0;
    }

    double MoverRailVehicleWheels::get_angle_rear_deg() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? wheel_angle_rear_deg : 0.0;
    }

    double MoverRailVehicleWheels::get_rotation_speed_rps() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->nrot : 0.0;
    }

    double MoverRailVehicleWheels::get_rotation_acceleration_rps2() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->nrot_eps : 0.0;
    }

    bool MoverRailVehicleWheels::get_slipping() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SlippingWheels : false;
    }

    double MoverRailVehicleWheels::get_flat() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->WheelFlat : 0.0;
    }

    void MoverRailVehicleWheels::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["wheel_angle_front_deg"] = get_angle_front_deg();
        p_state["wheel_angle_powered_deg"] = get_angle_powered_deg();
        p_state["wheel_angle_rear_deg"] = get_angle_rear_deg();
        p_state["wheel_rotation_speed_rps"] = get_rotation_speed_rps();
        p_state["wheel_rotation_acceleration_rps2"] = get_rotation_acceleration_rps2();
        p_state["slipping_wheels"] = get_slipping();
        p_state["wheel_flat"] = get_flat();
    }

    void MoverRailVehicleWheels::_fill_config_dictionary(Dictionary &p_config) const {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_config["powered_wheel_diameter"] = mover->WheelDiameter;
        p_config["front_rolling_wheel_diameter"] = mover->WheelDiameterL;
        p_config["rear_rolling_wheel_diameter"] = mover->WheelDiameterT;
        p_config["axle_inertial_moment"] = mover->AxleInertialMoment;
        p_config["track_width"] = mover->TrackW;
        p_config["axle_arrangement"] = String(mover->AxleArangement.c_str());
        p_config["bogie_axle_spacing"] = mover->ADist;
        p_config["bogie_pivot_spacing"] = mover->BDist;
        p_config["minimum_curve_radius"] = get_minimum_curve_radius();
        p_config["bearing_type"] = mover->BearingType == 0 ? BEARING_TYPE_SLIDE : BEARING_TYPE_ROLL;
        p_config["axles_powered_count"] = mover->NPoweredAxles;
        p_config["axles_count"] = mover->NAxles;
    }

    // The wheel angles are the vehicle layer's, not the Mover's: DynObj.cpp:3776-3792
    // (TDynamicObject::Update, "krecenie kolami"). From the Mover it reads Vel, V, nrot and
    // WheelDiameterL/T only.
    void MoverRailVehicleWheels::_do_process_component(const double p_delta) {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        if (p_mover->Vel == 0.0) {
            return;
        }

        wheel_angle_front_deg += Math::rad_to_deg(p_mover->V * p_delta / p_mover->WheelDiameterL);
        wheel_angle_front_deg = Math::wrapf(static_cast<real_t>(wheel_angle_front_deg), 0.0, 360.0);
        wheel_angle_powered_deg += p_mover->nrot * p_delta * 360.0;
        wheel_angle_powered_deg = Math::wrapf(static_cast<real_t>(wheel_angle_powered_deg), 0.0, 360.0);
        wheel_angle_rear_deg += Math::rad_to_deg(p_mover->V * p_delta / p_mover->WheelDiameterT);
        wheel_angle_rear_deg = Math::wrapf(static_cast<real_t>(wheel_angle_rear_deg), 0.0, 360.0);
    }
} // namespace godot
