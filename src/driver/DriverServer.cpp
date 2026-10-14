#include "DriverServer.hpp"
#include "person/PersonServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include "vehicles/rail/RailVehicleServer.hpp"
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/callable_method_pointer.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *DriverServer::driver_timetable_changed_signal = "driver_timetable_changed";
    const char *DriverServer::driver_order_changed_signal = "driver_order_changed";
    const char *DriverServer::driver_freed_signal = "driver_freed";
    const char *DriverServer::driver_attached_signal = "driver_attached";

    void DriverServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("implementation_register", "name", "implementation"), &DriverServer::implementation_register);
        ClassDB::bind_method(D_METHOD("implementation_unregister", "name"), &DriverServer::implementation_unregister);
        ClassDB::bind_method(D_METHOD("driver_get_rids"), &DriverServer::driver_get_rids);
        ClassDB::bind_method(
                D_METHOD("driver_attach_implementation", "driver", "implementation"),
                &DriverServer::driver_attach_implementation);
        ClassDB::bind_method(D_METHOD("driver_get_implementation", "driver"), &DriverServer::driver_get_implementation);
        ClassDB::bind_method(D_METHOD("vehicle_get_driver", "vehicle"), &DriverServer::vehicle_get_driver);
        ClassDB::bind_method(
                D_METHOD("driver_send_command", "driver", "command", "value1", "value2", "position"),
                &DriverServer::driver_send_command, DEFVAL(Vector3()));
        ClassDB::bind_method(
                D_METHOD("driver_schedule_update", "driver", "seconds"), &DriverServer::driver_schedule_update);
        ClassDB::bind_method(
                D_METHOD("vehicle_is_control_active", "vehicle"), &DriverServer::vehicle_is_control_active);
        ClassDB::bind_method(
                D_METHOD("driver_get_timetable_state", "driver"), &DriverServer::driver_get_timetable_state);
        ClassDB::bind_method(
                D_METHOD("vehicle_get_seconds_until_departure", "vehicle", "hours"),
                &DriverServer::vehicle_get_seconds_until_departure);
        ClassDB::bind_method(
                D_METHOD("driver_report_timetable_changed", "driver"), &DriverServer::driver_report_timetable_changed);
        ClassDB::bind_method(D_METHOD("driver_get_state", "driver"), &DriverServer::driver_get_state);
        ADD_SIGNAL(MethodInfo(driver_timetable_changed_signal, PropertyInfo(Variant::RID, "driver")));
        ClassDB::bind_method(
                D_METHOD("driver_report_order_changed", "driver"), &DriverServer::driver_report_order_changed);
        ADD_SIGNAL(MethodInfo(driver_order_changed_signal, PropertyInfo(Variant::RID, "driver")));
        ADD_SIGNAL(MethodInfo(driver_freed_signal, PropertyInfo(Variant::RID, "driver")));
        ADD_SIGNAL(MethodInfo(driver_attached_signal, PropertyInfo(Variant::RID, "driver")));
    }

    /// A freed person is no driver; one sitting down at the controls again takes them back. No
    /// explicit disconnect: callable_mp reports this instance as the callable's object, so the
    /// engine drops the connection when it dies.
    DriverServer::DriverServer() {
        PersonServer *persons = PersonServer::get_instance();
        VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL(persons);
        ERR_FAIL_NULL(vehicles);
        persons->connect(PersonServer::person_freed_signal, callable_mp(this, &DriverServer::_on_person_freed));
        vehicles->connect(
                VehicleServer::cabin_person_role_changed_signal,
                callable_mp(this, &DriverServer::_on_cabin_person_role_changed));
    }

    DriverServer::~DriverServer() {
        _set_processing(false);
    }

    /// Scheduled, it holds the runtime's clock, so the time it waits for passes
    void DriverServer::_set_processing(const bool p_processing) {
        if (processing == p_processing) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        processing = p_processing;
        if (p_processing) {
            runtime->clock_subscribe(callable_mp(this, &DriverServer::_process_updates));
            return;
        }
        runtime->clock_unsubscribe(callable_mp(this, &DriverServer::_process_updates));
    }

    /// Only the updates whose time has come are taken, and only those scheduled before the pass:
    /// what an update schedules, even for now, runs on the next frame
    void DriverServer::_process_updates(double /* p_seconds */) {
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        const double time = runtime->simulation_get_time();
        const uint64_t pass_end = next_sequence;
        while (!updates.empty() && updates.top().time <= time && updates.top().sequence < pass_end) {
            const UpdateEntry entry = updates.top();
            updates.pop();
            DriverData *data = drivers.getptr(entry.driver);
            if (data == nullptr || !(data->update_sequence == entry.sequence)) {
                continue;
            }
            data->update_sequence = 0;
            // taken before it runs: the update may create or free drivers, which rehashes the table
            const Ref<DriverImplementation> implementation = data->implementation;
            if (implementation.is_valid()) {
                implementation->update(entry.driver);
            }
        }
        _set_processing(!updates.empty());
    }

    /// Taken back: the implementation learns it drives a vehicle a player left as it is now
    /// (TController::TakeControl(), Driver.cpp:5700-5712)
    void DriverServer::_on_cabin_person_role_changed(
            const RID & /* p_cabin */, const RID &p_person, const VehiclePersonRole::Role p_role) {
        const DriverData *data = drivers.getptr(p_person);
        if (data == nullptr || data->implementation.is_null() ||
            !(p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER)) {
            return;
        }
        // held: the implementation may change the drivers, which rehashes the table
        const Ref<DriverImplementation> implementation = data->implementation;
        implementation->control_taken(p_person);
    }

    bool DriverServer::vehicle_is_control_active(const RID &p_vehicle) const {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, false);
        const TypedArray<VehiclePerson> at_controls =
                vehicles->vehicle_list_persons(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER);
        for (int index = 0; index < at_controls.size(); ++index) {
            const Ref<VehiclePerson> person = at_controls[index];
            if (drivers.has(person->get_person())) {
                return true;
            }
        }
        return false;
    }

    void DriverServer::driver_schedule_update(const RID &p_driver, const double p_seconds) {
        DriverData *data = drivers.getptr(p_driver);
        ERR_FAIL_NULL(data);
        data->update_sequence = next_sequence++;
        const SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        updates.push(
                UpdateEntry{runtime->simulation_get_time() + MAX(p_seconds, 0.0), data->update_sequence, p_driver});
        _set_processing(!updates.empty());
    }

    TypedArray<RID> DriverServer::driver_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, DriverData> &entry: drivers) {
            result.append(entry.key);
        }
        return result;
    }

    void DriverServer::_on_person_freed(const RID &p_person) {
        if (drivers.has(p_person)) {
            _detach(p_person);
        }
    }

    /// The person is no driver: its implementation learns it, before anyone is told
    void DriverServer::_detach(const RID &p_driver) {
        const Ref<DriverImplementation> implementation = drivers[p_driver].implementation;
        drivers.erase(p_driver);
        if (implementation.is_valid()) {
            implementation->driver_detached(p_driver);
        }
        emit_signal(driver_freed_signal, p_driver);
    }

    /// The driver thinks with p_implementation from now on: the one it thought with learns it lost
    /// it, the new one that it has it
    void DriverServer::_set_implementation(const RID &p_driver, const Ref<DriverImplementation> &p_implementation) {
        DriverData *data = drivers.getptr(p_driver);
        ERR_FAIL_NULL(data);
        const Ref<DriverImplementation> previous = data->implementation;
        data->implementation = p_implementation;
        if (previous.is_valid()) {
            previous->driver_detached(p_driver);
        }
        if (p_implementation.is_valid()) {
            p_implementation->driver_attached(p_driver);
        }
    }

    void
    DriverServer::implementation_register(const StringName &p_name, const Ref<DriverImplementation> &p_implementation) {
        ERR_FAIL_COND(p_implementation.is_null());
        implementations[p_name] = p_implementation;
        // collected first: an implementation learning of a driver may create or free drivers
        Vector<RID> declared;
        for (const KeyValue<RID, DriverData> &entry: drivers) {
            if (entry.value.implementation_name == p_name) {
                declared.push_back(entry.key);
            }
        }
        for (const RID &driver: declared) {
            _set_implementation(driver, p_implementation);
        }
    }

    void DriverServer::implementation_unregister(const StringName &p_name) {
        implementations.erase(p_name);
        Vector<RID> declared;
        for (const KeyValue<RID, DriverData> &entry: drivers) {
            if (entry.value.implementation_name == p_name) {
                declared.push_back(entry.key);
            }
        }
        for (const RID &driver: declared) {
            _set_implementation(driver, Ref<DriverImplementation>());
        }
    }

    void DriverServer::driver_attach_implementation(const RID &p_driver, const StringName &p_implementation) {
        const PersonServer *persons = PersonServer::get_instance();
        ERR_FAIL_NULL(persons);
        ERR_FAIL_COND_MSG(!persons->person_exists(p_driver), "A driver is a person of PersonServer");
        if (p_implementation.is_empty()) {
            if (drivers.has(p_driver)) {
                _detach(p_driver);
            }
            return;
        }
        const bool attached = !drivers.has(p_driver);
        drivers[p_driver].implementation_name = p_implementation;
        const Ref<DriverImplementation> *registered = implementations.getptr(p_implementation);
        _set_implementation(p_driver, registered == nullptr ? Ref<DriverImplementation>() : *registered);
        if (attached) {
            emit_signal(driver_attached_signal, p_driver);
        }
    }

    Ref<DriverImplementation> DriverServer::driver_get_implementation(const RID &p_driver) const {
        const DriverData *data = drivers.getptr(p_driver);
        ERR_FAIL_NULL_V(data, Ref<DriverImplementation>());
        return data->implementation;
    }

    RID DriverServer::vehicle_get_driver(const RID &p_vehicle) const {
        const VehicleServer *vehicles = VehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, RID());
        const TypedArray<VehiclePerson> aboard =
                vehicles->vehicle_list_persons(p_vehicle, VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY);
        for (int index = 0; index < aboard.size(); ++index) {
            const Ref<VehiclePerson> person = aboard[index];
            if (drivers.has(person->get_person())) {
                return person->get_person();
            }
        }
        return RID();
    }

    void DriverServer::driver_send_command(
            const RID &p_driver, const String &p_command, const double p_value1, const double p_value2,
            const Vector3 &p_position) {
        const DriverData *data = drivers.getptr(p_driver);
        ERR_FAIL_NULL(data);
        const Ref<DriverImplementation> implementation = data->implementation;
        if (implementation.is_null()) {
            return;
        }
        implementation->handle_command(p_driver, p_command, p_value1, p_value2, p_position);
    }

    Dictionary DriverServer::driver_get_timetable_state(const RID &p_driver) const {
        const DriverData *data = drivers.getptr(p_driver);
        ERR_FAIL_NULL_V(data, Dictionary());
        return data->implementation.is_valid() ? data->implementation->get_timetable_state(p_driver) : Dictionary();
    }

    double DriverServer::vehicle_get_seconds_until_departure(const RID &p_vehicle, const double p_hours) const {
        const RailVehicleServer *vehicles = RailVehicleServer::get_instance();
        ERR_FAIL_NULL_V(vehicles, 0.0);
        TypedArray<RID> trainset = vehicles->vehicle_get_coupled(
                p_vehicle, RailVehicleController::COUPLER_END_FRONT, RailVehicleController::COUPLING_FLAG_COUPLER);
        trainset.push_front(p_vehicle);
        for (int index = 0; index < trainset.size(); ++index) {
            const RID driver = vehicle_get_driver(trainset[index]);
            const DriverData *data = drivers.getptr(driver);
            if (data == nullptr || data->implementation.is_null()) {
                continue;
            }
            const double seconds = data->implementation->get_seconds_until_departure(driver, p_hours);
            if (!Math::is_nan(seconds)) {
                return seconds;
            }
        }
        return 0.0;
    }

    void DriverServer::driver_report_timetable_changed(const RID &p_driver) {
        ERR_FAIL_COND(!drivers.has(p_driver));
        emit_signal(driver_timetable_changed_signal, p_driver);
    }

    void DriverServer::driver_report_order_changed(const RID &p_driver) {
        ERR_FAIL_COND(!drivers.has(p_driver));
        emit_signal(driver_order_changed_signal, p_driver);
    }

    Dictionary DriverServer::driver_get_state(const RID &p_driver) const {
        const DriverData *data = drivers.getptr(p_driver);
        ERR_FAIL_NULL_V(data, Dictionary());
        return data->implementation.is_valid() ? data->implementation->get_state(p_driver) : Dictionary();
    }
} // namespace godot
