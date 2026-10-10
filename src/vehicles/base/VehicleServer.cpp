#include "VehicleServer.hpp"
#include "person/PersonServer.hpp"
#include "simulation/SimulationServer.hpp"
#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/base/VehicleImplementationServer.hpp"

#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {
    const char *VehicleServer::vehicle_moved_signal = "vehicle_moved";
    const char *VehicleServer::vehicle_command_received_signal = "vehicle_command_received";
    const char *VehicleServer::vehicle_freed_signal = "vehicle_freed";
    const char *VehicleServer::vehicle_controller_changed_signal = "vehicle_controller_changed";
    const char *VehicleServer::vehicle_configured_signal = "vehicle_configured";
    const char *VehicleServer::vehicle_config_changed_signal = "vehicle_config_changed";
    const char *VehicleServer::cabin_person_entered_signal = "cabin_person_entered";
    const char *VehicleServer::vehicle_cabin_detached_signal = "vehicle_cabin_detached";
    const char *VehicleServer::cabin_person_left_signal = "cabin_person_left";
    const char *VehicleServer::cabin_person_role_changed_signal = "cabin_person_role_changed";
    const char *VehicleServer::cabin_person_moved_signal = "cabin_person_moved";

    VehicleServer::VehicleServer() {
        // The vehicles step by the runtime's clock, which stands still while paused. No explicit
        // disconnect: callable_mp reports this instance as the callable's object, so the engine
        // drops the connection when the instance dies.
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        runtime->connect(
                SimulationServer::simulation_advanced_signal,
                callable_mp(this, &VehicleServer::_on_simulation_advanced));
        // a freed person leaves the cabin it sat in
        PersonServer *persons = PersonServer::get_instance();
        ERR_FAIL_NULL(persons);
        persons->connect(PersonServer::person_freed_signal, callable_mp(this, &VehicleServer::_on_person_freed));
    }

    VehicleServer::~VehicleServer() {
        stepping_enabled = false;
        _refresh_stepping();
    }

    void VehicleServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("implementation_register", "name", "implementation_id"),
                &VehicleServer::implementation_register);
        ClassDB::bind_method(D_METHOD("implementation_unregister", "name"), &VehicleServer::implementation_unregister);
        ClassDB::bind_method(D_METHOD("implementation_get_list"), &VehicleServer::implementation_get_list);
        ClassDB::bind_method(D_METHOD("stepping_advance", "delta"), &VehicleServer::stepping_advance);
        ClassDB::bind_method(D_METHOD("stepping_set_enabled", "enabled"), &VehicleServer::stepping_set_enabled);
        ClassDB::bind_method(D_METHOD("stepping_is_enabled"), &VehicleServer::stepping_is_enabled);
        ClassDB::bind_method(D_METHOD("vehicle_create"), &VehicleServer::vehicle_create);
        ClassDB::bind_method(D_METHOD("vehicle_free", "vehicle"), &VehicleServer::vehicle_free);
        ClassDB::bind_method(D_METHOD("vehicle_exists", "vehicle"), &VehicleServer::vehicle_exists);
        ClassDB::bind_method(D_METHOD("controller_create"), &VehicleServer::controller_create);
        ClassDB::bind_method(
                D_METHOD("controller_configure", "controller", "description"), &VehicleServer::controller_configure);
        ClassDB::bind_method(D_METHOD("controller_free", "controller"), &VehicleServer::controller_free);
        ClassDB::bind_method(
                D_METHOD("vehicle_bind_controller", "vehicle", "controller"), &VehicleServer::vehicle_bind_controller);
        ClassDB::bind_method(
                D_METHOD("vehicle_set_initial_velocity", "vehicle", "velocity"),
                &VehicleServer::vehicle_set_initial_velocity);
        ClassDB::bind_method(D_METHOD("vehicle_set_name", "vehicle", "name"), &VehicleServer::vehicle_set_name);
        ClassDB::bind_method(D_METHOD("vehicle_get_name", "vehicle"), &VehicleServer::vehicle_get_name);
        ClassDB::bind_method(D_METHOD("vehicle_get_rid_by_name", "name"), &VehicleServer::vehicle_get_rid_by_name);
        ClassDB::bind_method(D_METHOD("vehicle_get_rids"), &VehicleServer::vehicle_get_rids);
        ClassDB::bind_method(
                D_METHOD("vehicle_is_simulation_ready", "vehicle"), &VehicleServer::vehicle_is_simulation_ready);
        ClassDB::bind_method(D_METHOD("cabin_create"), &VehicleServer::cabin_create);
        ClassDB::bind_method(D_METHOD("cabin_free", "cabin"), &VehicleServer::cabin_free);
        ClassDB::bind_method(
                D_METHOD("vehicle_cabin_attach", "vehicle", "cabin"), &VehicleServer::vehicle_cabin_attach);
        ClassDB::bind_method(
                D_METHOD("vehicle_cabin_detach", "vehicle", "cabin"), &VehicleServer::vehicle_cabin_detach);
        ClassDB::bind_method(D_METHOD("vehicle_get_cabins", "vehicle"), &VehicleServer::vehicle_get_cabins);
        ClassDB::bind_method(D_METHOD("vehicle_get_cabin_count", "vehicle"), &VehicleServer::vehicle_get_cabin_count);
        ClassDB::bind_method(D_METHOD("cabin_get_vehicle", "cabin"), &VehicleServer::cabin_get_vehicle);
        ClassDB::bind_method(
                D_METHOD("cabin_person_enter", "cabin", "person", "role"), &VehicleServer::cabin_person_enter);
        ClassDB::bind_method(D_METHOD("cabin_person_leave", "cabin", "person"), &VehicleServer::cabin_person_leave);
        ClassDB::bind_method(
                D_METHOD("cabin_person_change_role", "cabin", "person", "role"),
                &VehicleServer::cabin_person_change_role);
        ClassDB::bind_method(D_METHOD("cabin_person_move", "person", "cabin"), &VehicleServer::cabin_person_move);
        ClassDB::bind_method(D_METHOD("cabin_list_persons", "cabin", "role"), &VehicleServer::cabin_list_persons);
        ClassDB::bind_method(D_METHOD("cabin_has_person_role", "cabin", "role"), &VehicleServer::cabin_has_person_role);
        ClassDB::bind_method(D_METHOD("vehicle_list_persons", "vehicle", "role"), &VehicleServer::vehicle_list_persons);
        ClassDB::bind_method(
                D_METHOD("vehicle_has_person_role", "vehicle", "role"), &VehicleServer::vehicle_has_person_role);
        ClassDB::bind_method(D_METHOD("person_get_cabin", "person"), &VehicleServer::person_get_cabin);
        ClassDB::bind_method(D_METHOD("person_get_vehicle", "person"), &VehicleServer::person_get_vehicle);
        ClassDB::bind_method(D_METHOD("person_get_role", "person"), &VehicleServer::person_get_role);
        ClassDB::bind_method(D_METHOD("vehicle_get_dimensions", "vehicle"), &VehicleServer::vehicle_get_dimensions);
        ClassDB::bind_method(
                D_METHOD("vehicle_send_command", "vehicle", "command", "p1", "p2"),
                &VehicleServer::vehicle_send_command, DEFVAL(Variant()), DEFVAL(Variant()));
        ClassDB::bind_method(
                D_METHOD("vehicle_broadcast_command", "command", "p1", "p2"), &VehicleServer::vehicle_broadcast_command,
                DEFVAL(Variant()), DEFVAL(Variant()));
        ClassDB::bind_method(D_METHOD("vehicle_get_commands", "vehicle"), &VehicleServer::vehicle_get_commands);
        ClassDB::bind_method(
                D_METHOD("vehicle_has_command", "vehicle", "command"), &VehicleServer::vehicle_has_command);
        ClassDB::bind_method(D_METHOD("vehicle_get_velocity", "vehicle"), &VehicleServer::vehicle_get_velocity);
        ClassDB::bind_method(D_METHOD("vehicle_get_speed", "vehicle"), &VehicleServer::vehicle_get_speed);
        ClassDB::bind_method(
                D_METHOD("vehicle_component_get", "vehicle", "type"), &VehicleServer::vehicle_component_get);
        ClassDB::bind_method(D_METHOD("vehicle_get_controller", "vehicle"), &VehicleServer::vehicle_get_controller);
        ClassDB::bind_method(
                D_METHOD("vehicle_generic_component_find", "vehicle", "tag"),
                &VehicleServer::vehicle_generic_component_find);
        ClassDB::bind_method(D_METHOD("vehicle_dump_state", "vehicle"), &VehicleServer::vehicle_dump_state);
        ClassDB::bind_method(D_METHOD("vehicle_dump_config", "vehicle"), &VehicleServer::vehicle_dump_config);

        ADD_SIGNAL(MethodInfo(
                vehicle_moved_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::VECTOR3, "position")));
        ADD_SIGNAL(MethodInfo(
                vehicle_command_received_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::STRING, "command"), PropertyInfo(Variant::NIL, "p1"),
                PropertyInfo(Variant::NIL, "p2")));
        ADD_SIGNAL(MethodInfo(vehicle_freed_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(vehicle_controller_changed_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(vehicle_configured_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(vehicle_config_changed_signal, PropertyInfo(Variant::RID, "vehicle")));
        ADD_SIGNAL(MethodInfo(
                cabin_person_entered_signal, PropertyInfo(Variant::RID, "cabin"), PropertyInfo(Variant::RID, "person"),
                PropertyInfo(
                        Variant::INT, "role", PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_CLASS_IS_ENUM,
                        "VehiclePersonRole.Role")));
        ADD_SIGNAL(MethodInfo(
                vehicle_cabin_detached_signal, PropertyInfo(Variant::RID, "vehicle"),
                PropertyInfo(Variant::RID, "cabin")));
        ADD_SIGNAL(MethodInfo(
                cabin_person_left_signal, PropertyInfo(Variant::RID, "cabin"), PropertyInfo(Variant::RID, "person")));
        ADD_SIGNAL(MethodInfo(
                cabin_person_role_changed_signal, PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::RID, "person"),
                PropertyInfo(
                        Variant::INT, "role", PROPERTY_HINT_ENUM, "", PROPERTY_USAGE_CLASS_IS_ENUM,
                        "VehiclePersonRole.Role")));
        ADD_SIGNAL(MethodInfo(
                cabin_person_moved_signal, PropertyInfo(Variant::RID, "person"), PropertyInfo(Variant::RID, "cabin"),
                PropertyInfo(Variant::RID, "previous")));
    }

    void VehicleServer::implementation_register(const StringName &p_name, const uint64_t p_implementation_id) {
        ERR_FAIL_NULL(
                Object::cast_to<VehicleImplementationServer>(ObjectDB::get_instance(ObjectID(p_implementation_id))));
        implementations[p_name] = ObjectID(p_implementation_id);
    }

    void VehicleServer::implementation_unregister(const StringName &p_name) {
        implementations.erase(p_name);
    }

    PackedStringArray VehicleServer::implementation_get_list() const {
        PackedStringArray result;
        for (const KeyValue<StringName, ObjectID> &entry: implementations) {
            result.push_back(entry.key);
        }
        return result;
    }

    void VehicleServer::stepping_set_enabled(const bool p_enabled) {
        stepping_enabled = p_enabled;
        _refresh_stepping();
    }

    bool VehicleServer::stepping_is_enabled() const {
        return stepping_enabled;
    }

    /// Stepping holds the runtime's clock, so time passes while there is a vehicle to move
    void VehicleServer::_refresh_stepping() {
        const bool running = stepping_enabled && !vehicles.is_empty();
        if (running == stepping) {
            return;
        }
        SimulationServer *runtime = SimulationServer::get_instance();
        ERR_FAIL_NULL(runtime);
        stepping = running;
        if (stepping) {
            runtime->clock_hold();
            return;
        }
        runtime->clock_release();
    }

    /// One frame of the clock, before any node has been processed (SceneTree's `process_frame`)
    void VehicleServer::_on_simulation_advanced(const double p_seconds) {
        if (stepping) {
            stepping_advance(p_seconds);
        }
    }

    void VehicleServer::stepping_advance(const double p_delta) {
        if (p_delta <= 0.0) {
            return;
        }
        for (const KeyValue<StringName, SteppedGroup> &entry: stepped_groups) {
            const ObjectID *id = implementations.getptr(entry.key);
            VehicleImplementationServer *implementation =
                    id != nullptr ? Object::cast_to<VehicleImplementationServer>(ObjectDB::get_instance(*id)) : nullptr;
            if (implementation != nullptr && !entry.value.vehicles.is_empty()) {
                implementation->stepping_advance(entry.value.vehicles, entry.value.controllers, p_delta);
            }
        }
    }

    VehicleController *VehicleServer::_get_controller(const RID &p_vehicle) const {
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        const Controller *slot = vehicle != nullptr ? controllers.getptr(vehicle->controller) : nullptr;
        return slot != nullptr ? slot->controller.ptr() : nullptr;
    }

    RID VehicleServer::vehicle_create() {
        ++next_vehicle_id;
        const RID vehicle_rid = UtilityFunctions::rid_from_int64(next_vehicle_id);
        vehicles.insert(vehicle_rid, Vehicle());
        _refresh_stepping();
        return vehicle_rid;
    }

    void VehicleServer::vehicle_free(const RID &p_vehicle) {
        if (!vehicles.has(p_vehicle)) {
            return;
        }
        vehicle_bind_controller(p_vehicle, RID());
        // its cabins go with it, and whoever sits in them leaves first
        const Vector<RID> vehicle_cabins = vehicles.getptr(p_vehicle)->cabins;
        for (const RID &cabin: vehicle_cabins) {
            cabin_free(cabin);
        }
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        // the name may have passed to a later vehicle of the same name - that one keeps it
        if (const RID *named = vehicles_by_name.getptr(vehicle->name); named != nullptr && *named == p_vehicle) {
            vehicles_by_name.erase(vehicle->name);
        }
        vehicles.erase(p_vehicle);
        _refresh_stepping();
        emit_signal(vehicle_freed_signal, p_vehicle);
    }

    bool VehicleServer::vehicle_exists(const RID &p_vehicle) const {
        return vehicles.has(p_vehicle);
    }

    RID VehicleServer::controller_create() {
        ++next_controller_id;
        const RID controller_rid = UtilityFunctions::rid_from_int64(next_controller_id);
        controllers.insert(controller_rid, Controller());
        return controller_rid;
    }

    /* The server's own copy: the description is shared by every vehicle built from it (a cached
     * FIZ). A vehicle this controller drives is stopped on the old configuration and started on
     * the new one. */
    void VehicleServer::controller_configure(const RID &p_controller, const Ref<VehicleController> &p_description) {
        Controller *slot = controllers.getptr(p_controller);
        ERR_FAIL_NULL(slot);
        ERR_FAIL_COND(p_description.is_null());
        const RID vehicle = slot->vehicle;
        if (vehicle.is_valid()) {
            vehicle_bind_controller(vehicle, RID());
        }
        slot = controllers.getptr(p_controller);
        slot->controller = p_description->duplicate_deep(Resource::DEEP_DUPLICATE_INTERNAL);
        if (vehicle.is_valid()) {
            vehicle_bind_controller(vehicle, p_controller);
        }
    }

    void VehicleServer::controller_free(const RID &p_controller) {
        const Controller *slot = controllers.getptr(p_controller);
        if (slot == nullptr) {
            return;
        }
        if (slot->vehicle.is_valid()) {
            vehicle_bind_controller(slot->vehicle, RID());
        }
        controllers.erase(p_controller);
    }

    void VehicleServer::vehicle_bind_controller(const RID &p_vehicle, const RID &p_controller) {
        Vehicle *vehicle = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(vehicle);
        // what the vehicle ran on lets go of its components, commands and simulation
        if (VehicleController *previous = _get_controller(p_vehicle); previous != nullptr) {
            // it leaves the group its implementation steps
            if (SteppedGroup *group = stepped_groups.getptr(vehicle->implementation); group != nullptr) {
                const int64_t index = group->vehicles.find(p_vehicle);
                group->vehicles.remove_at(index);
                group->controllers.remove_at(index);
            }
            _disconnect_relays(p_vehicle);
            previous->release();
            controllers.getptr(vehicle->controller)->vehicle = RID();
        }
        vehicle->controller = RID();
        vehicle->implementation = StringName();
        vehicle->state_dump_valid = false;
        Controller *slot = controllers.getptr(p_controller);
        if (slot == nullptr) {
            emit_signal(vehicle_controller_changed_signal, p_vehicle);
            return;
        }
        ERR_FAIL_COND_MSG(slot->controller.is_null(), "The controller is not configured");
        ERR_FAIL_COND_MSG(slot->vehicle.is_valid(), "The controller drives another vehicle");
        slot->vehicle = p_vehicle;
        vehicle->controller = p_controller;
        VehicleController *controller = slot->controller.ptr();
        controller->set_vehicle_rid(p_vehicle);
        controller->set_vehicle_id(vehicle->name);
        controller->set_initial_velocity(vehicle->initial_velocity);
        vehicle->implementation = controller->get_implementation();
        if (!vehicle->implementation.is_empty()) {
            SteppedGroup &group = stepped_groups[vehicle->implementation];
            group.vehicles.push_back(p_vehicle);
            group.controllers.push_back(slot->controller);
        }
        _connect_relays(p_vehicle);
        emit_signal(vehicle_controller_changed_signal, p_vehicle);
        controller->attach_to_system();
        controller->initialize();
    }

    Ref<VehicleController> VehicleServer::vehicle_get_controller(const RID &p_vehicle) const {
        return Ref<VehicleController>(_get_controller(p_vehicle));
    }

    uint64_t VehicleServer::vehicle_get_controller_instance_id(const RID &p_vehicle) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->get_instance_id() : 0;
    }

    void VehicleServer::vehicle_set_initial_velocity(const RID &p_vehicle, const double p_velocity) {
        Vehicle *vehicle = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL(vehicle);
        vehicle->initial_velocity = p_velocity;
        if (VehicleController *controller = _get_controller(p_vehicle); controller != nullptr) {
            controller->set_initial_velocity(p_velocity);
        }
    }

    void VehicleServer::_connect_relays(const RID &p_vehicle) {
        VehicleController *controller = _get_controller(p_vehicle);
        if (controller == nullptr) {
            return;
        }
        controller->connect(
                VehicleController::position_changed_signal,
                callable_mp(this, &VehicleServer::_on_vehicle_moved).bind(p_vehicle));
        controller->connect(
                VehicleController::command_received,
                callable_mp(this, &VehicleServer::_on_vehicle_command_received).bind(p_vehicle));
        controller->connect(
                VehicleController::simulation_initialized_signal,
                callable_mp(this, &VehicleServer::_on_vehicle_configured).bind(p_vehicle));
        controller->connect(
                VehicleController::config_changed,
                callable_mp(this, &VehicleServer::_on_vehicle_config_changed).bind(p_vehicle));
    }

    void VehicleServer::_disconnect_relays(const RID &p_vehicle) {
        VehicleController *controller = _get_controller(p_vehicle);
        if (controller == nullptr) {
            return;
        }
        controller->disconnect(
                VehicleController::position_changed_signal,
                callable_mp(this, &VehicleServer::_on_vehicle_moved).bind(p_vehicle));
        controller->disconnect(
                VehicleController::command_received,
                callable_mp(this, &VehicleServer::_on_vehicle_command_received).bind(p_vehicle));
        controller->disconnect(
                VehicleController::simulation_initialized_signal,
                callable_mp(this, &VehicleServer::_on_vehicle_configured).bind(p_vehicle));
        controller->disconnect(
                VehicleController::config_changed,
                callable_mp(this, &VehicleServer::_on_vehicle_config_changed).bind(p_vehicle));
    }

    void VehicleServer::_on_vehicle_moved(const Vector3 &p_position, const RID &p_vehicle) {
        emit_signal(vehicle_moved_signal, p_vehicle, p_position);
    }

    void VehicleServer::_on_vehicle_command_received(
            const String &p_command, const Variant &p_p1, const Variant &p_p2, const RID &p_vehicle) {
        emit_signal(vehicle_command_received_signal, p_vehicle, p_command, p_p1, p_p2);
    }

    void VehicleServer::_on_vehicle_configured(const RID &p_vehicle) {
        emit_signal(vehicle_configured_signal, p_vehicle);
    }

    void VehicleServer::_on_vehicle_config_changed(const RID &p_vehicle) {
        emit_signal(vehicle_config_changed_signal, p_vehicle);
    }

    void VehicleServer::vehicle_set_name(const RID &p_vehicle, const String &p_name) {
        Vehicle *vehicle = vehicles.getptr(p_vehicle);
        if (vehicle == nullptr) {
            return;
        }
        if (const RID *named = vehicles_by_name.getptr(vehicle->name); named != nullptr && *named == p_vehicle) {
            vehicles_by_name.erase(vehicle->name);
        }
        vehicle->name = p_name;
        if (VehicleController *controller = _get_controller(p_vehicle); controller != nullptr) {
            controller->set_vehicle_id(p_name);
        }
        // Names.h:29 basic_table::insert - a vehicle named "" or "none" is not looked up by name
        if (p_name.is_empty() || p_name == "none") {
            return;
        }
        /* Two vehicles of one name is a scenery's mistake, and the second one silently taking the
         * name away from the first is how it stays invisible - an event or a console command then
         * reaches a vehicle nobody meant. */
        if (const RID *taken = vehicles_by_name.getptr(p_name); taken != nullptr && *taken != p_vehicle) {
            UtilityFunctions::push_warning(
                    vformat("Bad scenario: two vehicles named \"%s\" - the later one takes the name", p_name));
        }
        vehicles_by_name[p_name] = p_vehicle;
    }

    String VehicleServer::vehicle_get_name(const RID &p_vehicle) const {
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        return vehicle != nullptr ? vehicle->name : String();
    }

    RID VehicleServer::vehicle_get_rid_by_name(const String &p_name) const {
        const RID *found = vehicles_by_name.getptr(p_name);
        return found != nullptr ? *found : RID();
    }

    TypedArray<RID> VehicleServer::vehicle_get_rids() const {
        TypedArray<RID> result;
        for (const KeyValue<RID, Vehicle> &entry: vehicles) {
            result.push_back(entry.key);
        }
        return result;
    }

    bool VehicleServer::vehicle_is_simulation_ready(const RID &p_vehicle) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr && controller->is_simulation_ready();
    }

    Vector3 VehicleServer::vehicle_get_dimensions(const RID &p_vehicle) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        if (controller == nullptr) {
            return Vector3();
        }
        return Vector3(
                static_cast<real_t>(controller->get_dimensions_width()),
                static_cast<real_t>(controller->get_dimensions_height()),
                static_cast<real_t>(controller->get_dimensions_length()));
    }

    Variant VehicleServer::vehicle_send_command(
            const RID &p_vehicle, const StringName &p_command, const Variant &p_p1, const Variant &p_p2) {
        ERR_FAIL_COND_V(!vehicles.has(p_vehicle), Variant());
        VehicleController *controller = _get_controller(p_vehicle);
        ERR_FAIL_NULL_V(controller, Variant());
        return controller->send_command(p_command, p_p1, p_p2);
    }

    void
    VehicleServer::vehicle_broadcast_command(const StringName &p_command, const Variant &p_p1, const Variant &p_p2) {
        bool known = false;
        for (const KeyValue<RID, Vehicle> &entry: vehicles) {
            VehicleController *controller = _get_controller(entry.key);
            if (controller != nullptr && controller->get_commands().has(p_command)) {
                known = true;
                controller->send_command(p_command, p_p1, p_p2);
            }
        }
        if (!known) {
            UtilityFunctions::push_error(vformat("Unknown command: %s", p_command));
        }
    }

    PackedStringArray VehicleServer::vehicle_get_commands(const RID &p_vehicle) const {
        ERR_FAIL_COND_V(!vehicles.has(p_vehicle), PackedStringArray());
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->get_commands() : PackedStringArray();
    }

    bool VehicleServer::vehicle_has_command(const RID &p_vehicle, const StringName &p_command) const {
        ERR_FAIL_COND_V(!vehicles.has(p_vehicle), false);
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr && controller->has_command(p_command);
    }

    double VehicleServer::vehicle_get_velocity(const RID &p_vehicle) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->get_velocity() : 0.0;
    }

    double VehicleServer::vehicle_get_speed(const RID &p_vehicle) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->get_speed() : 0.0;
    }

    Ref<VehicleComponent>
    VehicleServer::vehicle_component_get(const RID &p_vehicle, const VehicleComponentType::Type p_type) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->get_component(p_type) : Ref<VehicleComponent>();
    }

    TypedArray<VehicleComponent>
    VehicleServer::vehicle_generic_component_find(const RID &p_vehicle, const StringName &p_tag) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->find_generic_components(p_tag) : TypedArray<VehicleComponent>();
    }

    /* Built lazily, on the first read after a step or a command moves the state - the
     * controller's state serial says when - and handed out unchanged until then: at most once per
     * tick, and not at all for a vehicle nobody reads. A deliberate exception to "a getter never
     * changes state" (the operator's decision, RC-024): a tick building every vehicle's dump would
     * pay for hundreds of keys nobody asks for. */
    Dictionary VehicleServer::vehicle_dump_state(const RID &p_vehicle) {
        Vehicle *vehicle = vehicles.getptr(p_vehicle);
        if (vehicle == nullptr) {
            return Dictionary();
        }
        VehicleController *controller = _get_controller(p_vehicle);
        if (controller == nullptr) {
            return Dictionary();
        }
        const uint64_t serial = controller->get_state_serial();
        if (vehicle->state_dump_valid && vehicle->state_dump_serial == serial) {
            return vehicle->state_dump;
        }
        vehicle->state_dump = controller->get_state();
        vehicle->state_dump_serial = serial;
        vehicle->state_dump_valid = true;
        return vehicle->state_dump;
    }

    /* The configuration this vehicle was built with, by name. Diagnostic, like the state dump -
     * a reader after one value takes the component that owns it. */
    Dictionary VehicleServer::vehicle_dump_config(const RID &p_vehicle) const {
        const VehicleController *controller = _get_controller(p_vehicle);
        return controller != nullptr ? controller->get_config() : Dictionary();
    }

    RID VehicleServer::cabin_create() {
        const RID cabin = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        cabins.insert(cabin, Cabin());
        return cabin;
    }

    void VehicleServer::cabin_free(const RID &p_cabin) {
        const Cabin *cabin = cabins.getptr(p_cabin);
        if (cabin == nullptr) {
            return;
        }
        // a copy: detaching clears the cabin's own
        if (const RID vehicle = cabin->vehicle; vehicle.is_valid()) {
            vehicle_cabin_detach(vehicle, p_cabin);
        }
        cabins.erase(p_cabin);
    }

    void VehicleServer::vehicle_cabin_attach(const RID &p_vehicle, const RID &p_cabin) {
        Vehicle *vehicle = vehicles.getptr(p_vehicle);
        Cabin *cabin = cabins.getptr(p_cabin);
        ERR_FAIL_NULL(vehicle);
        ERR_FAIL_NULL(cabin);
        ERR_FAIL_COND_MSG(cabin->vehicle.is_valid(), "The cabin belongs to a vehicle already");
        cabin->vehicle = p_vehicle;
        vehicle->cabins.push_back(p_cabin);
    }

    void VehicleServer::vehicle_cabin_detach(const RID &p_vehicle, const RID &p_cabin) {
        Cabin *cabin = cabins.getptr(p_cabin);
        ERR_FAIL_NULL(cabin);
        ERR_FAIL_COND(!(cabin->vehicle == p_vehicle));
        Vector<RID> seated;
        for (const KeyValue<RID, VehiclePersonRole::Role> &entry: cabin->persons) {
            seated.push_back(entry.key);
        }
        for (const RID &person: seated) {
            cabin_person_leave(p_cabin, person);
        }
        cabins.getptr(p_cabin)->vehicle = RID();
        if (Vehicle *vehicle = vehicles.getptr(p_vehicle); vehicle != nullptr) {
            vehicle->cabins.erase(p_cabin);
        }
        emit_signal(vehicle_cabin_detached_signal, p_vehicle, p_cabin);
    }

    TypedArray<RID> VehicleServer::vehicle_get_cabins(const RID &p_vehicle) const {
        TypedArray<RID> result;
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(vehicle, result);
        for (const RID &cabin: vehicle->cabins) {
            result.push_back(cabin);
        }
        return result;
    }

    int VehicleServer::vehicle_get_cabin_count(const RID &p_vehicle) const {
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        return vehicle != nullptr ? static_cast<int>(vehicle->cabins.size()) : 0;
    }

    RID VehicleServer::cabin_get_vehicle(const RID &p_cabin) const {
        const Cabin *cabin = cabins.getptr(p_cabin);
        return cabin != nullptr ? cabin->vehicle : RID();
    }

    bool VehicleServer::_cabin_has_other_driver(const Cabin &p_cabin, const RID &p_person) const {
        for (const KeyValue<RID, VehiclePersonRole::Role> &entry: p_cabin.persons) {
            if (entry.value == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER && !(entry.key == p_person)) {
                return true;
            }
        }
        return false;
    }

    Error
    VehicleServer::cabin_person_enter(const RID &p_cabin, const RID &p_person, const VehiclePersonRole::Role p_role) {
        Cabin *cabin = cabins.getptr(p_cabin);
        const PersonServer *persons = PersonServer::get_instance();
        ERR_FAIL_NULL_V(persons, ERR_UNCONFIGURED);
        ERR_FAIL_COND_V(!persons->person_exists(p_person), ERR_INVALID_PARAMETER);
        if (cabin == nullptr || !cabin->vehicle.is_valid() || p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY) {
            return ERR_INVALID_PARAMETER;
        }
        if (person_cabins.has(p_person)) {
            return ERR_ALREADY_IN_USE;
        }
        if (p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER && _cabin_has_other_driver(*cabin, p_person)) {
            return ERR_UNAVAILABLE;
        }
        cabin->persons.insert(p_person, p_role);
        person_cabins.insert(p_person, p_cabin);
        emit_signal(cabin_person_entered_signal, p_cabin, p_person, p_role);
        return OK;
    }

    void VehicleServer::cabin_person_leave(const RID &p_cabin, const RID &p_person) {
        Cabin *cabin = cabins.getptr(p_cabin);
        ERR_FAIL_NULL(cabin);
        ERR_FAIL_COND(!cabin->persons.has(p_person));
        cabin->persons.erase(p_person);
        person_cabins.erase(p_person);
        emit_signal(cabin_person_left_signal, p_cabin, p_person);
    }

    Error VehicleServer::cabin_person_change_role(
            const RID &p_cabin, const RID &p_person, const VehiclePersonRole::Role p_role) {
        Cabin *cabin = cabins.getptr(p_cabin);
        if (cabin == nullptr || !cabin->persons.has(p_person) || p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY) {
            return ERR_INVALID_PARAMETER;
        }
        if (cabin->persons[p_person] == p_role) {
            return OK;
        }
        if (p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER && _cabin_has_other_driver(*cabin, p_person)) {
            return ERR_UNAVAILABLE;
        }
        cabin->persons[p_person] = p_role;
        emit_signal(cabin_person_role_changed_signal, p_cabin, p_person, p_role);
        return OK;
    }

    Error VehicleServer::cabin_person_move(const RID &p_person, const RID &p_cabin) {
        const RID *seat = person_cabins.getptr(p_person);
        Cabin *target = cabins.getptr(p_cabin);
        if (seat == nullptr || target == nullptr) {
            return ERR_INVALID_PARAMETER;
        }
        const RID previous = *seat;
        if (previous == p_cabin) {
            return OK;
        }
        Cabin *source = cabins.getptr(previous);
        const VehiclePersonRole::Role role = source->persons[p_person];
        if (role == VehiclePersonRole::VEHICLE_PERSON_ROLE_DRIVER && _cabin_has_other_driver(*target, p_person)) {
            return ERR_UNAVAILABLE;
        }
        source->persons.erase(p_person);
        target->persons.insert(p_person, role);
        person_cabins[p_person] = p_cabin;
        emit_signal(cabin_person_moved_signal, p_person, p_cabin, previous);
        return OK;
    }

    TypedArray<VehiclePerson>
    VehicleServer::cabin_list_persons(const RID &p_cabin, const VehiclePersonRole::Role p_role) const {
        TypedArray<VehiclePerson> result;
        const Cabin *cabin = cabins.getptr(p_cabin);
        ERR_FAIL_NULL_V(cabin, result);
        for (const KeyValue<RID, VehiclePersonRole::Role> &entry: cabin->persons) {
            if (p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY || entry.value == p_role) {
                result.push_back(VehiclePerson::create(entry.key, p_cabin, entry.value));
            }
        }
        return result;
    }

    bool VehicleServer::cabin_has_person_role(const RID &p_cabin, const VehiclePersonRole::Role p_role) const {
        const Cabin *cabin = cabins.getptr(p_cabin);
        if (cabin == nullptr) {
            return false;
        }
        for (const KeyValue<RID, VehiclePersonRole::Role> &entry: cabin->persons) {
            if (p_role == VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY || entry.value == p_role) {
                return true;
            }
        }
        return false;
    }

    TypedArray<VehiclePerson>
    VehicleServer::vehicle_list_persons(const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        TypedArray<VehiclePerson> result;
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        ERR_FAIL_NULL_V(vehicle, result);
        for (const RID &cabin: vehicle->cabins) {
            result.append_array(cabin_list_persons(cabin, p_role));
        }
        return result;
    }

    bool VehicleServer::vehicle_has_person_role(const RID &p_vehicle, const VehiclePersonRole::Role p_role) const {
        const Vehicle *vehicle = vehicles.getptr(p_vehicle);
        if (vehicle == nullptr) {
            return false;
        }
        for (const RID &cabin: vehicle->cabins) {
            if (cabin_has_person_role(cabin, p_role)) {
                return true;
            }
        }
        return false;
    }

    RID VehicleServer::person_get_cabin(const RID &p_person) const {
        const RID *cabin = person_cabins.getptr(p_person);
        return cabin != nullptr ? *cabin : RID();
    }

    RID VehicleServer::person_get_vehicle(const RID &p_person) const {
        return cabin_get_vehicle(person_get_cabin(p_person));
    }

    VehiclePersonRole::Role VehicleServer::person_get_role(const RID &p_person) const {
        const RID *seat = person_cabins.getptr(p_person);
        const Cabin *cabin = seat != nullptr ? cabins.getptr(*seat) : nullptr;
        const VehiclePersonRole::Role *role = cabin != nullptr ? cabin->persons.getptr(p_person) : nullptr;
        return role != nullptr ? *role : VehiclePersonRole::VEHICLE_PERSON_ROLE_ANY;
    }

    void VehicleServer::_on_person_freed(const RID &p_person) {
        if (const RID *cabin = person_cabins.getptr(p_person); cabin != nullptr) {
            cabin_person_leave(*cabin, p_person);
        }
    }
} // namespace godot
