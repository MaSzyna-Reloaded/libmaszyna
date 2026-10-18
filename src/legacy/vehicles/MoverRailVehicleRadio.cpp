#include "MoverRailVehicleRadio.hpp"
#include "legacy/vehicles/MoverBackend.hpp"

namespace godot {
    void MoverRailVehicleRadio::_bind_methods() {}

    bool MoverRailVehicleRadio::get_radio_enabled() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Radio : false;
    }

    bool MoverRailVehicleRadio::get_powered() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr ? mover->Radio && (mover->Power24vIsAvailable || mover->Power110vIsAvailable) : false;
    }

    bool MoverRailVehicleRadio::get_radio_stop_active() const {
        const TMoverParameters *mover = get_mover();
        return mover != nullptr && mover->RadioStopFlag;
    }

    void MoverRailVehicleRadio::_do_process_component(const double p_delta) {
        if (const bool powered = get_powered(); powered != previous_powered) {
            previous_powered = powered;
            emit_signal(radio_toggled_signal, powered);
        }
    }

    void MoverRailVehicleRadio::radio(const bool p_enabled) {
        TMoverParameters *mover = get_mover();
        ASSERT_MOVER(mover);
        mover->Radio = p_enabled;
    }

    // Original engine: TDynamicObject::RadioStop (DynObj.cpp:7229) - Radio-Stop fitted and the
    // radio on brakes in emergency, the driver's "Emergency_brake" command landing in
    // RadiostopSwitch (Driver.cpp:4487, Mover.cpp:9462). Kept as in the original: it acts only on
    // a vehicle somebody drives - the original needs a crew in the vehicle for the brakes to act.
    bool MoverRailVehicleRadio::_radio_stop_receive(const bool p_driven) {
        TMoverParameters *mover = get_mover();
        if (mover == nullptr || !p_driven || !mover->SecuritySystem.radiostop_available() || !mover->Radio) {
            return false;
        }
        mover->RadiostopSwitch(true);
        return true;
    }
} // namespace godot
