#include "MaszynaVehicleCommandAction.hpp"
#include "driver/DriverServer.hpp"
#include "maszyna/scenario/MaszynaScenarioEventServer.hpp"
#include "maszyna/vehicles/MaszynaCarDriver.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleLoad.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    void MaszynaVehicleCommandAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_command", "command"), &MaszynaVehicleCommandAction::set_command);
        ClassDB::bind_method(D_METHOD("get_command"), &MaszynaVehicleCommandAction::get_command);
        ClassDB::bind_method(D_METHOD("set_value1", "value1"), &MaszynaVehicleCommandAction::set_value1);
        ClassDB::bind_method(D_METHOD("get_value1"), &MaszynaVehicleCommandAction::get_value1);
        ClassDB::bind_method(D_METHOD("set_value2", "value2"), &MaszynaVehicleCommandAction::set_value2);
        ClassDB::bind_method(D_METHOD("get_value2"), &MaszynaVehicleCommandAction::get_value2);
        ClassDB::bind_method(D_METHOD("set_source", "source"), &MaszynaVehicleCommandAction::set_source);
        ClassDB::bind_method(D_METHOD("get_source"), &MaszynaVehicleCommandAction::get_source);
        ClassDB::bind_method(D_METHOD("set_position", "position"), &MaszynaVehicleCommandAction::set_position);
        ClassDB::bind_method(D_METHOD("get_position"), &MaszynaVehicleCommandAction::get_position);
        ClassDB::bind_method(D_METHOD("set_radio", "radio"), &MaszynaVehicleCommandAction::set_radio);
        ClassDB::bind_method(D_METHOD("get_radio"), &MaszynaVehicleCommandAction::get_radio);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "command"), "set_command", "get_command");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value1"), "set_value1", "get_value1");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value2"), "set_value2", "get_value2");
        ADD_PROPERTY(PropertyInfo(Variant::RID, "source"), "set_source", "get_source");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "radio", PROPERTY_HINT_RESOURCE_TYPE, "SceneryVirtualRadio"), "set_radio",
                "get_radio");
    }

    /// getvalues_event::run_() / putvalues_event::run_() (Event.cpp:615-635, 771-844)
    void MaszynaVehicleCommandAction::run(const RID &p_event, const RID &p_activator) {
        MaszynaScenarioEventServer *events = MaszynaScenarioEventServer::get_instance();
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        ERR_FAIL_NULL(events);
        ERR_FAIL_NULL(vehicle_server);
        const bool read = source.is_valid();
        const String text = read ? events->memory_get_text(source) : command;
        // a getvalues goes to the activator's own driver alone, nothing happens without one
        // (getvalues_event::run_(), Event.cpp:629-631; TMemCell::PutCommand(), MemCell.cpp:124-128)
        const DriverServer *own_drivers = DriverServer::get_instance();
        const MaszynaCarDriver *car_drivers = MaszynaCarDriver::get_instance();
        if (read && (own_drivers == nullptr || !own_drivers->vehicle_get_driver(p_activator).is_valid()) &&
            (car_drivers == nullptr || !car_drivers->vehicle_is_attached(p_activator))) {
            return;
        }
        if (text == CAB_SIGNAL && p_activator.is_valid()) {
            vehicle_server->vehicle_send_command(p_activator, "security_cabsignal_trigger");
            return;
        }
        // the original switches Radio-Stop on for 1 (Mover.cpp:12624-12630); sent, it can only
        // switch it on
        const double command_value1 = read ? events->memory_get_value1(source) : value1;
        if (text == EMERGENCY_BRAKE) {
            if (Math::floor(command_value1) == 1.0) {
                ERR_FAIL_COND_MSG(radio.is_null(), "Emergency_brake needs the scenery's radio.");
                radio->transmit(
                        position, RadioServer::CHANNEL_ALL, RadioServer::RADIO_STOP_REACH,
                        RadioServer::KIND_RADIO_STOP);
            }
            return;
        }
        // the rest is an order for the driver of the vehicle that queued the event - a car's is
        // the cars' driver
        const double command_value2 = read ? events->memory_get_value2(source) : value2;
        if (MaszynaCarDriver *cars = MaszynaCarDriver::get_instance();
            cars != nullptr && cars->vehicle_is_attached(p_activator)) {
            cars->vehicle_handle_command(p_activator, text, command_value1, command_value2);
            return;
        }
        DriverServer *drivers = DriverServer::get_instance();
        ERR_FAIL_NULL(drivers);
        if (text.is_empty()) {
            return;
        }
        RID driver = drivers->vehicle_get_driver(p_activator);
        if (driver.is_valid()) {
            drivers->driver_send_command(driver, text, command_value1, command_value2, position);
            return;
        }
        // nobody aboard: a load is the vehicle's own, anything else goes to the driver of its trainset
        // (ctOwner, is_command_for_owner(), Event.cpp:796-806, 836-845) - and with none there, to the
        // vehicle itself (Event.cpp:808-816)
        if (!text.begins_with(MaszynaScenarioEventServer::LOAD_COMMAND) &&
            !text.begins_with(MaszynaScenarioEventServer::UNLOAD_COMMAND)) {
            RailVehicleServer *rail_vehicles = RailVehicleServer::get_instance();
            ERR_FAIL_NULL(rail_vehicles);
            const RID trainset = rail_vehicles->vehicle_get_trainset(p_activator);
            const TypedArray<RID> vehicles =
                    trainset.is_valid() ? rail_vehicles->trainset_get_vehicles(trainset) : TypedArray<RID>();
            for (int i = 0; i < vehicles.size() && !driver.is_valid(); i++) {
                driver = drivers->vehicle_get_driver(vehicles[i]);
            }
            if (driver.is_valid()) {
                drivers->driver_send_command(driver, text, command_value1, command_value2, position);
                return;
            }
        }
        events->vehicle_put_command(p_activator, text, command_value1, command_value2, position);
    }

    void MaszynaVehicleCommandAction::set_command(const String &p_command) {
        command = p_command;
    }

    String MaszynaVehicleCommandAction::get_command() const {
        return command;
    }

    void MaszynaVehicleCommandAction::set_value1(const double p_value1) {
        value1 = p_value1;
    }

    double MaszynaVehicleCommandAction::get_value1() const {
        return value1;
    }

    void MaszynaVehicleCommandAction::set_value2(const double p_value2) {
        value2 = p_value2;
    }

    double MaszynaVehicleCommandAction::get_value2() const {
        return value2;
    }

    void MaszynaVehicleCommandAction::set_source(const RID &p_source) {
        source = p_source;
    }

    RID MaszynaVehicleCommandAction::get_source() const {
        return source;
    }

    void MaszynaVehicleCommandAction::set_position(const Vector3 &p_position) {
        position = p_position;
    }

    Vector3 MaszynaVehicleCommandAction::get_position() const {
        return position;
    }

    void MaszynaVehicleCommandAction::set_radio(const Ref<SceneryVirtualRadio> &p_radio) {
        radio = p_radio;
    }

    Ref<SceneryVirtualRadio> MaszynaVehicleCommandAction::get_radio() const {
        return radio;
    }
} // namespace godot
