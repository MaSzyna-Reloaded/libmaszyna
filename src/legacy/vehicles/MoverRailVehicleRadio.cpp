#include "MoverRailVehicleRadio.hpp"
#include "legacy/vehicles/MoverBackend.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    void MoverRailVehicleRadio::_bind_methods() {}

    bool MoverRailVehicleRadio::get_enabled() const {
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

    // Original engine: TTrain::OnCommand_radiostopsend (Train.cpp:8149) - on the press, and only a
    // powered radio sends
    void MoverRailVehicleRadio::radio_stop(const bool p_pressed) {
        const VehicleController *controller = train_controller_node;
        RailVehicleServer *server = RailVehicleServer::get_instance();
        if (!p_pressed || !get_powered() || controller == nullptr || server == nullptr) {
            return;
        }
        server->vehicle_emergency_signal_send(controller->get_rid());
    }

    // Original engine: TTrain::OnCommand_radiocall1send/3send (Train.cpp:8209-8236) - on the press,
    // from a powered radio on any channel but the one without calls
    void MoverRailVehicleRadio::radio_call(const bool p_pressed, const RadioCall p_call) {
        const VehicleController *controller = train_controller_node;
        RailVehicleServer *server = RailVehicleServer::get_instance();
        if (!p_pressed || !get_powered() || get_channel() == CHANNEL_NO_CALLS || controller == nullptr ||
            server == nullptr) {
            return;
        }
        server->vehicle_radio_call(controller->get_rid(), p_call);
    }

    // Original engine: TDynamicObject::RadioStop (DynObj.cpp:7229) - a vehicle with somebody
    // driving it, Radio-Stop fitted and the radio on brakes in emergency; the driver's
    // "Emergency_brake" command lands in RadiostopSwitch (Driver.cpp:4487, Mover.cpp:9462)
    bool MoverRailVehicleRadio::radio_stop_receive() {
        TMoverParameters *mover = get_mover();
        const VehicleController *controller = train_controller_node;
        const VehicleServer *vehicles = VehicleServer::get_instance();
        if (mover == nullptr || controller == nullptr || vehicles == nullptr ||
            !vehicles->vehicle_has_person_role(controller->get_rid(), VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER) ||
            !mover->SecuritySystem.radiostop_available() || !mover->Radio) {
            return false;
        }
        mover->RadiostopSwitch(true);
        return true;
    }
} // namespace godot
