#include "MoverRailVehicleSecuritySystem.hpp"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "macros.hpp"
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    void MoverRailVehicleSecuritySystem::_bind_methods() {}


    // Detected once per tick against this part's own members - these used to be compared against
    // the state dictionary while that dictionary was being filled, so the signals fired on a read
    // rather than on a change.
    void MoverRailVehicleSecuritySystem::_do_process_component(const double p_delta) {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        if (const bool blinking = p_mover->SecuritySystem.is_blinking(); previous_blinking != blinking) {
            previous_blinking = blinking;
            emit_signal("blinking_changed", blinking);
        }
        if (const bool beeping = p_mover->SecuritySystem.is_beeping(); previous_beeping != beeping) {
            previous_beeping = beeping;
            emit_signal("beeping_changed", beeping);
        }
    }


    bool MoverRailVehicleSecuritySystem::get_beeping() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_beeping() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_blinking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_blinking() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_radiostop_available() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.radiostop_available() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_vigilance_blinking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_vigilance_blinking() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_cabsignal_blinking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_cabsignal_blinking() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_cabsignal_beeping() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_cabsignal_beeping() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_braking() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_braking() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_engine_blocked() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.is_engine_blocked() : false;
    }

    bool MoverRailVehicleSecuritySystem::get_separate_acknowledge() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->SecuritySystem.has_separate_acknowledge() : false;
    }

    void MoverRailVehicleSecuritySystem::_fill_state_dictionary(Dictionary &p_state) const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return;
        }
        p_state["beeping"] = get_beeping();
        p_state["blinking"] = get_blinking();
        p_state["radiostop_available"] = get_radiostop_available();
        p_state["vigilance_blinking"] = get_vigilance_blinking();
        p_state["cabsignal_blinking"] = get_cabsignal_blinking();
        p_state["cabsignal_beeping"] = get_cabsignal_beeping();
        p_state["braking"] = get_braking();
        p_state["engine_blocked"] = get_engine_blocked();
        p_state["separate_acknowledge"] = get_separate_acknowledge();
    }

    void MoverRailVehicleSecuritySystem::_apply_configuration() {
        TMoverParameters *p_mover = get_mover();
        ASSERT_MOVER(p_mover);
        // the alerter is switched on by the cab's activation and off by its deactivation
        // (CabActivisation(), Mover.cpp:2905, 2956) - an inactive cab car's must not run

        p_mover->SecuritySystem.vigilance_enabled = get_aware_system_active();
        p_mover->SecuritySystem.cabsignal_enabled = get_aware_system_cabsignal();
        p_mover->SecuritySystem.separate_acknowledge = get_aware_system_separate_acknowledge();
        p_mover->SecuritySystem.is_sifa = get_aware_system_sifa();

        p_mover->SecuritySystem.AwareDelay = get_aware_delay();
        p_mover->SecuritySystem.AwareMinSpeed = get_aware_min_speed() == AWARE_MIN_SPEED_FROM_MAX_VELOCITY
                                                        ? AWARE_MIN_SPEED_MAX_VELOCITY_SHARE * p_mover->Vmax
                                                        : get_aware_min_speed();
        p_mover->SecuritySystem.CabDependent = get_cab_dependent();
        p_mover->SecuritySystem.EmergencyBrakeDelay = get_emergency_brake_delay();
        p_mover->SecuritySystem.radiostop_enabled = get_radio_stop_enabled();
        p_mover->SecuritySystem.SoundSignalDelay = get_sound_signal_delay();
        p_mover->SecuritySystem.MagnetLocation = get_shp_magnet_distance();
        p_mover->SecuritySystem.MaxHoldTime = get_ca_max_hold_time();

        switch (get_emergency_signal()) {
            case EMERGENCY_SIGNAL_SIREN_LOW_TONE:
                p_mover->EmergencyBrakeWarningSignal = MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW;
                break;
            case EMERGENCY_SIGNAL_SIREN_HIGH_TONE:
                p_mover->EmergencyBrakeWarningSignal = MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_HIGH;
                break;
            case EMERGENCY_SIGNAL_WHISTLE:
                p_mover->EmergencyBrakeWarningSignal = MaszynaMoverVehicleServer::WARNING_SIGNAL_WHISTLE;
                break;
            default:
                p_mover->EmergencyBrakeWarningSignal = MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW;
                break;
        }
    }


    // Train.cpp:2876 OnCommand_cabsignalacknowledge - the cab signalling of a vehicle with a separate
    // acknowledge button (FIZ SeparateAcknowledge) is not reset by the vigilance button
    void MoverRailVehicleSecuritySystem::security_cabsignal_acknowledge() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (mover->SecuritySystem.has_separate_acknowledge()) {
            mover->SecuritySystem.cabsignal_reset();
        }
    }

    void MoverRailVehicleSecuritySystem::security_cabsignal_trigger() {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->SecuritySystem.set_cabsignal();
    }

    void MoverRailVehicleSecuritySystem::security_radiostop(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->RadiostopSwitch(p_enabled);
    }

    void MoverRailVehicleSecuritySystem::security_acknowledge(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_enabled) {
            mover->SecuritySystem.acknowledge_press();
        } else {
            mover->SecuritySystem.acknowledge_release();
        }
    }
} // namespace godot
