#include "MoverRailVehicleHorns.hpp"
#include "legacy/maszyna-mover/utilities.h"
#include "legacy/vehicles/MaszynaMoverVehicleServer.hpp"
#include "legacy/vehicles/MoverBackend.hpp"

namespace godot {
    void MoverRailVehicleHorns::_bind_methods() {}


    void MoverRailVehicleHorns::set_horn_low(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_state) {
            mover->WarningSignal |= MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW;
        } else {
            mover->WarningSignal &= ~MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW;
        }
    }

    void MoverRailVehicleHorns::set_horn_high(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_state) {
            mover->WarningSignal |= MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_HIGH;
        } else {
            mover->WarningSignal &= ~MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_HIGH;
        }
    }

    void MoverRailVehicleHorns::set_whistle(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_state) {
            mover->WarningSignal |= MaszynaMoverVehicleServer::WARNING_SIGNAL_WHISTLE;
        } else {
            mover->WarningSignal &= ~MaszynaMoverVehicleServer::WARNING_SIGNAL_WHISTLE;
        }
    }


    bool MoverRailVehicleHorns::get_low_pressed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(mover->WarningSignal, MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW)
                                : false;
    }

    bool MoverRailVehicleHorns::get_high_pressed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(mover->WarningSignal, MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_HIGH)
                                : false;
    }

    bool MoverRailVehicleHorns::get_whistle_pressed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(mover->WarningSignal, MaszynaMoverVehicleServer::WARNING_SIGNAL_WHISTLE)
                                : false;
    }

    // The combination is the vehicle layer's, not the Mover's: DynObj.cpp:4884-4891. From the
    // Mover it reads Vel, AlarmChainFlag, EmergencyBrakeWarningSignal and WarningSignal.
    int MoverRailVehicleHorns::_get_combined_signal() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0;
        }
        return ((mover->Vel > HORN_EMERGENCY_MIN_SPEED) && mover->AlarmChainFlag ? mover->EmergencyBrakeWarningSignal
                                                                                 : 0) |
               mover->WarningSignal;
    }

    bool MoverRailVehicleHorns::get_low_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(_get_combined_signal(), MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW)
                                : false;
    }

    bool MoverRailVehicleHorns::get_high_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(_get_combined_signal(), MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_HIGH)
                                : false;
    }

    bool MoverRailVehicleHorns::get_whistle_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(_get_combined_signal(), MaszynaMoverVehicleServer::WARNING_SIGNAL_WHISTLE)
                                : false;
    }

    int MoverRailVehicleHorns::get_horn() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0;
        }
        if (TestFlag(mover->WarningSignal, MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_LOW)) {
            return 1;
        }
        return TestFlag(mover->WarningSignal, MaszynaMoverVehicleServer::WARNING_SIGNAL_HORN_HIGH) ? -1 : 0;
    }

    void MoverRailVehicleHorns::_fill_state_dictionary(Dictionary &p_state) const {
        // a component without a backend publishes nothing at all, rather than zeroes
        if (get_mover() == nullptr) {
            return;
        }
        p_state["horn_low_pressed"] = get_low_pressed();
        p_state["horn_high_pressed"] = get_high_pressed();
        p_state["whistle_pressed"] = get_whistle_pressed();
        p_state["horn_low_active"] = get_low_active();
        p_state["horn_high_active"] = get_high_active();
        p_state["whistle_active"] = get_whistle_active();
        p_state["horn"] = get_horn();
    }
} // namespace godot
