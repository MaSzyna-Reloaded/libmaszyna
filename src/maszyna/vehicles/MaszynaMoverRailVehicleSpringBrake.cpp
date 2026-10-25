#include "MaszynaMoverRailVehicleSpringBrake.hpp"
#include "maszyna/vehicles/MaszynaMoverBackend.hpp"
#include "vehicles/rail/RailVehicleSpringBrake.hpp"

namespace godot {
    void MaszynaMoverRailVehicleSpringBrake::_bind_methods() {
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, spring_actuator_chamber_volume, "spring/actuator")
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, spring_actuator_max_filling_force,
                "spring/actuator")
        BIND_PROPERTY(MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, pressure_force_coefficient)
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, spring_actuator_preload_pressure, "spring/actuator")
        BIND_PROPERTY(MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, spring_full_balance_pressure, "spring")
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, brake_signal_released_state_pressure,
                "brake_signal")
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, brake_signal_braked_state_pressure, "brake_signal")
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, valve_cross_section_actuator_discharge,
                "valve_cross_section")
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, valve_cross_section_actuator_charge,
                "valve_cross_section")
        BIND_PROPERTY(
                MaszynaMoverRailVehicleSpringBrake, Variant::FLOAT, valve_cross_section_pneumatic_brake,
                "valve_cross_section")
        BIND_PROPERTY(MaszynaMoverRailVehicleSpringBrake, Variant::INT, required_coupler_connection_method)
    }


    void MaszynaMoverRailVehicleSpringBrake::spring_brake_activate(const bool p_active) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpringBrakeActivate(p_active);
    }

    void MaszynaMoverRailVehicleSpringBrake::spring_brake_shut_off(const bool p_shut_off) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpringBrakeShutOff(p_shut_off);
    }

    void MaszynaMoverRailVehicleSpringBrake::spring_brake_release() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SpringBrakeRelease();
    }

    void MaszynaMoverRailVehicleSpringBrake::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        if (!p_mover->SpringBrake.Cylinder) {
            p_mover->SpringBrake.Cylinder = std::make_shared<TReservoir>();
        }
        p_mover->SpringBrake.Cylinder->CreateCap(get_spring_actuator_chamber_volume());
        p_mover->SpringBrake.MaxBrakeForce = get_pressure_force_coefficient(); // It is equal to brake force
        p_mover->SpringBrake.MaxSetPressure = get_spring_actuator_preload_pressure();
        p_mover->SpringBrake.ResetPressure = get_spring_actuator_max_filling_force();
        p_mover->SpringBrake.MinForcePressure = get_spring_full_balance_pressure();
        p_mover->SpringBrake.PressureOff = get_brake_signal_released_state_pressure();
        p_mover->SpringBrake.PressureOn = get_brake_signal_braked_state_pressure();
        // Mover.cpp:11025 - the original reads FIZ ValveOnArea into ValveOffArea and vice versa
        p_mover->SpringBrake.ValveOffArea = get_valve_cross_section_actuator_discharge();
        p_mover->SpringBrake.ValveOnArea = get_valve_cross_section_actuator_charge();
        p_mover->SpringBrake.ValvePNBrakeArea = get_valve_cross_section_pneumatic_brake();
        p_mover->SpringBrake.PNBrakeConnection = p_mover->SpringBrake.ValvePNBrakeArea > 0;
        // defaults to spring_brake::MultiTractionCoupler{127} (MOVER.h) when the FIZ has no MTC=
        p_mover->SpringBrake.MultiTractionCoupler = get_required_coupler_connection_method();
        // Mover.cpp:11028 - loading the section leaves the brake armed, not shut off and released; the
        // struct defaults (ShuttOff{true}, IsReady{false}) describe a vehicle without one
        p_mover->SpringBrake.ShuttOff = false;
        p_mover->SpringBrake.Activate = false;
        p_mover->SpringBrake.IsReady = true;

        //@TODO: There might be a need to update Spring Brake in the mover internally but it seems to be working as for
        // now
        VehicleComponent::_apply_configuration();
    }


    bool MaszynaMoverRailVehicleSpringBrake::get_ready() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpringBrake.IsReady : false;
    }

    bool MaszynaMoverRailVehicleSpringBrake::get_shut_off() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpringBrake.ShuttOff : false;
    }

    bool MaszynaMoverRailVehicleSpringBrake::get_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpringBrake.Activate : false;
    }

    bool MaszynaMoverRailVehicleSpringBrake::get_braking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpringBrake.IsActive : false;
    }

    double MaszynaMoverRailVehicleSpringBrake::get_cylinder_pressure() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SpringBrake.SBP : 0.0;
    }

    void MaszynaMoverRailVehicleSpringBrake::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["spring_brake_is_ready"] = get_ready();
        p_state["spring_brake_shut_off"] = get_shut_off();
        p_state["spring_brake_active"] = get_active();
        p_state["spring_brake_braking"] = get_braking();
        p_state["spring_brake_cylinder_pressure"] = get_cylinder_pressure();
    }


} // namespace godot
