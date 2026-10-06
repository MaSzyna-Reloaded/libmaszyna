#include "MoverRailVehicleHorns.hpp"
#include "legacy/maszyna-mover/utilities.h"
#include "legacy/vehicles/MoverBackend.hpp"

namespace godot {
    void MoverRailVehicleHorns::_bind_methods() {}


    void MoverRailVehicleHorns::set_horn_low(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_state) {
            mover->WarningSignal |= 1;
        } else {
            mover->WarningSignal &= ~1;
        }
    }

    void MoverRailVehicleHorns::set_horn_high(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_state) {
            mover->WarningSignal |= 2;
        } else {
            mover->WarningSignal &= ~2;
        }
    }

    void MoverRailVehicleHorns::set_whistle(const bool p_state) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        if (p_state) {
            mover->WarningSignal |= 4;
        } else {
            mover->WarningSignal &= ~4;
        }
    }


    bool MoverRailVehicleHorns::get_low_pressed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(mover->WarningSignal, 1) : false;
    }

    bool MoverRailVehicleHorns::get_high_pressed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(mover->WarningSignal, 2) : false;
    }

    bool MoverRailVehicleHorns::get_whistle_pressed() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(mover->WarningSignal, 4) : false;
    }

    // The combination is the vehicle layer's, not the Mover's: DynObj.cpp:4884-4891. From the
    // Mover it reads Vel, AlarmChainFlag, EmergencyBrakeWarningSignal and WarningSignal.
    int MoverRailVehicleHorns::get_combined_signal() const {
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
        return mover != nullptr ? TestFlag(get_combined_signal(), 1) : false;
    }

    bool MoverRailVehicleHorns::get_high_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(get_combined_signal(), 2) : false;
    }

    bool MoverRailVehicleHorns::get_whistle_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? TestFlag(get_combined_signal(), 4) : false;
    }

    int MoverRailVehicleHorns::get_horn() const {
        const TMoverParameters *mover = get_mover();
        if (mover == nullptr) {
            return 0;
        }
        if (TestFlag(mover->WarningSignal, 1)) {
            return 1;
        }
        return TestFlag(mover->WarningSignal, 2) ? -1 : 0;
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
