#include "MaszynaLegacyVehicleCommandAction.hpp"
#include "driver/DriverServer.hpp"
#include "scenario/ScenarioEventServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"

namespace godot {
    void MaszynaLegacyVehicleCommandAction::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_command", "command"), &MaszynaLegacyVehicleCommandAction::set_command);
        ClassDB::bind_method(D_METHOD("get_command"), &MaszynaLegacyVehicleCommandAction::get_command);
        ClassDB::bind_method(D_METHOD("set_value1", "value1"), &MaszynaLegacyVehicleCommandAction::set_value1);
        ClassDB::bind_method(D_METHOD("get_value1"), &MaszynaLegacyVehicleCommandAction::get_value1);
        ClassDB::bind_method(D_METHOD("set_value2", "value2"), &MaszynaLegacyVehicleCommandAction::set_value2);
        ClassDB::bind_method(D_METHOD("get_value2"), &MaszynaLegacyVehicleCommandAction::get_value2);
        ClassDB::bind_method(D_METHOD("set_source", "source"), &MaszynaLegacyVehicleCommandAction::set_source);
        ClassDB::bind_method(D_METHOD("get_source"), &MaszynaLegacyVehicleCommandAction::get_source);
        ClassDB::bind_method(D_METHOD("set_position", "position"), &MaszynaLegacyVehicleCommandAction::set_position);
        ClassDB::bind_method(D_METHOD("get_position"), &MaszynaLegacyVehicleCommandAction::get_position);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "command"), "set_command", "get_command");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value1"), "set_value1", "get_value1");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "value2"), "set_value2", "get_value2");
        ADD_PROPERTY(PropertyInfo(Variant::RID, "source"), "set_source", "get_source");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "position"), "set_position", "get_position");
    }

    /// getvalues_event::run_() / putvalues_event::run_() (Event.cpp:615-635, 771-844)
    void MaszynaLegacyVehicleCommandAction::run(const RID &p_event, const RID &p_activator) {
        const ScenarioEventServer *events = ScenarioEventServer::get_instance();
        VehicleServer *vehicle_server = VehicleServer::get_instance();
        RailVehicleServer *vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL(events);
        ERR_FAIL_NULL(vehicle_server);
        ERR_FAIL_NULL(vehicles);
        const bool read = source.is_valid();
        const String text = read ? events->memory_get_text(source) : command;
        if (text == CAB_SIGNAL && p_activator.is_valid()) {
            vehicle_server->vehicle_send_command(p_activator, "security_cabsignal_trigger");
            return;
        }
        // the original switches Radio-Stop on for 1 (Mover.cpp:12624-12630); sent, it can only
        // switch it on
        const double command_value1 = read ? events->memory_get_value1(source) : value1;
        if (text == EMERGENCY_BRAKE) {
            if (Math::floor(command_value1) == 1.0) {
                vehicles->emergency_signal_send(position);
            }
            return;
        }
        // the rest is an order for the driver of the vehicle that queued the event
        DriverServer *drivers = DriverServer::get_instance();
        ERR_FAIL_NULL(drivers);
        const RID driver = drivers->vehicle_get_driver(p_activator);
        if (text.is_empty() || !driver.is_valid()) {
            return;
        }
        drivers->driver_send_command(
                driver, text, command_value1, read ? events->memory_get_value2(source) : value2, position);
    }

    void MaszynaLegacyVehicleCommandAction::set_command(const String &p_command) {
        command = p_command;
    }

    String MaszynaLegacyVehicleCommandAction::get_command() const {
        return command;
    }

    void MaszynaLegacyVehicleCommandAction::set_value1(const double p_value1) {
        value1 = p_value1;
    }

    double MaszynaLegacyVehicleCommandAction::get_value1() const {
        return value1;
    }

    void MaszynaLegacyVehicleCommandAction::set_value2(const double p_value2) {
        value2 = p_value2;
    }

    double MaszynaLegacyVehicleCommandAction::get_value2() const {
        return value2;
    }

    void MaszynaLegacyVehicleCommandAction::set_source(const RID &p_source) {
        source = p_source;
    }

    RID MaszynaLegacyVehicleCommandAction::get_source() const {
        return source;
    }

    void MaszynaLegacyVehicleCommandAction::set_position(const Vector3 &p_position) {
        position = p_position;
    }

    Vector3 MaszynaLegacyVehicleCommandAction::get_position() const {
        return position;
    }
} // namespace godot
