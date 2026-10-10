#include "vehicles/base/VehicleComponent.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehicleServer.hpp"
#include <godot_cpp/classes/gd_extension.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

namespace godot {

    const char *VehicleController::simulation_configured_signal = "simulation_configured";
    const char *VehicleController::simulation_initialized_signal = "simulation_initialized";
    const char *VehicleController::command_received = "command_received";
    const char *VehicleController::config_changed = "config_changed";
    const char *VehicleController::position_changed_signal = "position_changed";

    void VehicleController::_bind_methods() {
        ClassDB::bind_method(D_METHOD("get_state"), &VehicleController::get_state);
        ClassDB::bind_method(D_METHOD("get_config"), &VehicleController::get_config);

        ClassDB::bind_method(
                D_METHOD("send_command", "command", "p1", "p2"), &VehicleController::send_command, DEFVAL(Variant()),
                DEFVAL(Variant()));
        ClassDB::bind_method(D_METHOD("get_commands"), &VehicleController::get_commands);


        ClassDB::bind_method(D_METHOD("register_command", "command", "callable"), &VehicleController::register_command);
        ClassDB::bind_method(D_METHOD("unregister_command", "command"), &VehicleController::unregister_command);
        ClassDB::bind_method(D_METHOD("apply_config"), &VehicleController::apply_config);
        ClassDB::bind_method(D_METHOD("initialize"), &VehicleController::initialize);
        ClassDB::bind_method(D_METHOD("process_components", "delta"), &VehicleController::process_components);
        ClassDB::bind_method(D_METHOD("update_state"), &VehicleController::update_state);
        ClassDB::bind_method(D_METHOD("get_velocity"), &VehicleController::get_velocity);
        ClassDB::bind_method(D_METHOD("get_speed"), &VehicleController::get_speed);
        ClassDB::bind_method(D_METHOD("get_acceleration"), &VehicleController::get_acceleration);
        ClassDB::bind_method(D_METHOD("get_mass_total"), &VehicleController::get_mass_total);
        ClassDB::bind_method(D_METHOD("get_total_distance"), &VehicleController::get_total_distance);
        ClassDB::bind_method(D_METHOD("get_direction"), &VehicleController::get_direction);
        ClassDB::bind_method(D_METHOD("emit_config_changed"), &VehicleController::emit_config_changed);
        ClassDB::bind_method(D_METHOD("apply_configuration"), &VehicleController::apply_configuration);
        ClassDB::bind_method(D_METHOD("is_simulation_ready"), &VehicleController::is_simulation_ready);
        ClassDB::bind_method(D_METHOD("add_component", "component"), &VehicleController::add_component);
        ClassDB::bind_method(D_METHOD("remove_component", "component"), &VehicleController::remove_component);
        ClassDB::bind_method(D_METHOD("set_components", "components"), &VehicleController::set_components);
        ClassDB::bind_method(D_METHOD("get_components"), &VehicleController::get_components);
        ADD_PROPERTY(
                PropertyInfo(
                        Variant::ARRAY, "components", PROPERTY_HINT_ARRAY_TYPE,
                        vformat("%s/%s:%s", Variant::OBJECT, PROPERTY_HINT_RESOURCE_TYPE, "VehicleComponent")),
                "set_components", "get_components");
        ClassDB::bind_method(D_METHOD("get_component", "type"), &VehicleController::get_component);
        ClassDB::bind_method(D_METHOD("find_components", "type"), &VehicleController::find_components);
        /* Read by whoever caches this vehicle's dump: a step or a command moves the state on. */
        ClassDB::bind_method(D_METHOD("get_state_serial"), &VehicleController::get_state_serial);
        ClassDB::bind_method(D_METHOD("find_generic_components", "tag"), &VehicleController::find_generic_components);
        ClassDB::bind_method(D_METHOD("is_physics_active"), &VehicleController::is_physics_active);
        ClassDB::bind_method(D_METHOD("get_world_transform"), &VehicleController::get_world_transform);
        ClassDB::bind_method(D_METHOD("get_world_position"), &VehicleController::get_world_position);
        ClassDB::bind_method(
                D_METHOD("emit_position_changed_if_needed"), &VehicleController::emit_position_changed_if_needed);
        ClassDB::bind_method(D_METHOD("set_vehicle_rid", "vehicle"), &VehicleController::set_vehicle_rid);

        ClassDB::bind_method(D_METHOD("set_implementation", "implementation"), &VehicleController::set_implementation);
        ClassDB::bind_method(D_METHOD("get_implementation"), &VehicleController::get_implementation);
        ADD_PROPERTY(PropertyInfo(Variant::STRING_NAME, "implementation"), "set_implementation", "get_implementation");
        BIND_PROPERTY(VehicleController, Variant::STRING, vehicle_id);
        BIND_PROPERTY(VehicleController, Variant::FLOAT, mass);
        BIND_PROPERTY(VehicleController, Variant::FLOAT, power);
        BIND_PROPERTY(VehicleController, Variant::FLOAT, max_velocity);
        BIND_PROPERTY_W_HINT(
                VehicleController, Variant::INT, category, PROPERTY_HINT_ENUM,
                enum_hint(
                        {{"Train", CATEGORY_TRAIN},
                         {"Road", CATEGORY_ROAD},
                         {"Ship", CATEGORY_SHIP},
                         {"Airplane", CATEGORY_AIRPLANE}}));
        BIND_PROPERTY(VehicleController, Variant::FLOAT, dimensions_length, "dimensions");
        BIND_PROPERTY(VehicleController, Variant::FLOAT, dimensions_height, "dimensions");
        BIND_PROPERTY(VehicleController, Variant::FLOAT, dimensions_width, "dimensions");
        BIND_PROPERTY(VehicleController, Variant::FLOAT, dimensions_drag_coefficient, "dimensions");
        BIND_PROPERTY(VehicleController, Variant::FLOAT, dimensions_floor_height, "dimensions");
        BIND_PROPERTY(VehicleController, Variant::FLOAT, initial_velocity);

        ADD_SIGNAL(MethodInfo(simulation_configured_signal));
        ADD_SIGNAL(MethodInfo(simulation_initialized_signal));
        ADD_SIGNAL(MethodInfo(config_changed));
        ADD_SIGNAL(MethodInfo(position_changed_signal, PropertyInfo(Variant::VECTOR3, "position")));
        ADD_SIGNAL(MethodInfo(
                command_received, PropertyInfo(Variant::STRING, "command"), PropertyInfo(Variant::NIL, "p1"),
                PropertyInfo(Variant::NIL, "p2")));

        BIND_ENUM_CONSTANT(CATEGORY_TRAIN);
        BIND_ENUM_CONSTANT(CATEGORY_ROAD);
        BIND_ENUM_CONSTANT(CATEGORY_SHIP);
        BIND_ENUM_CONSTANT(CATEGORY_AIRPLANE);

        BIND_ENUM_CONSTANT(DIRECTION_BACKWARD);
        BIND_ENUM_CONSTANT(DIRECTION_NEUTRAL);
        BIND_ENUM_CONSTANT(DIRECTION_FORWARD);
    }

    void VehicleController::register_command(const StringName &p_command, const Callable &p_callable) {
        ERR_FAIL_COND_MSG(commands.has(p_command), vformat("Command is already registered: %s", p_command));
        commands[p_command] = p_callable;
    }

    void VehicleController::unregister_command(const StringName &p_command) {
        commands.erase(p_command);
    }

    PackedStringArray VehicleController::get_commands() const {
        PackedStringArray names;
        for (const KeyValue<StringName, Callable> &entry: commands) {
            names.push_back(entry.key);
        }
        return names;
    }

    bool VehicleController::has_command(const StringName &p_command) const {
        return commands.has(p_command);
    }

    /* In the editor too: a component outliving its controller must not keep a pointer to it */
    void VehicleController::_notification(const int p_what) {
        if (p_what == NOTIFICATION_PREDELETE) {
            release();
        }
    }

    /* Applying the vehicle's configuration to the backend, in two passes. The first writes what
     * each part owns alone - the vehicle's own, then every component's, in registration order
     * (the order of the FIZ sections). The second writes what depends on other parts of the
     * vehicle (a bare coupler on the engine's tractive force), so it never depends on that order.
     * The signal is emitted afterwards and means exactly "the backend now carries this" - it is
     * not how the components are reached, because a component of this vehicle is applied by name
     * here rather than by whoever happens to be connected. */
    void VehicleController::apply_configuration() {
        apply_config();
        for (const Ref<VehicleComponent> &component: components) {
            component->apply_config();
        }
        apply_vehicle_config();
        for (const Ref<VehicleComponent> &component: components) {
            component->apply_vehicle_config();
        }
        emit_config_changed();
        emit_signal(simulation_configured_signal);
    }

    /* Registering the vehicle's name and its own commands, where the vehicle comes into being -
     * it used to wait for NOTIFICATION_ENTER_TREE, which a vehicle outside a tree never gets. */
    /* A vehicle that is rebuilt keeps its identity - every reference taken to it stays valid -
     * so what it holds is handed back by name rather than by destroying the vehicle. */
    void VehicleController::release() {
        shutdown();
        _detach_components();
        implementation_server = ObjectID();
    }

    void VehicleController::_attach_implementation(const ObjectID &p_implementation) {
        implementation_server = p_implementation;
        for (const Ref<VehicleComponent> &component: components) {
            component->attach_implementation(implementation_server);
        }
    }

    /* From here the vehicle is a live one: its components join it (commands, the configuration
     * signal, the implementation), which a description - the same class, only stored - never does. */
    void VehicleController::attach_to_system() {
        in_system = true;
        for (const Ref<VehicleComponent> &component: components) {
            component->attach(this);
        }
        _register_commands();
    }

    /* The simulation, once every component is attached - _initialize_simulation() pushes the
     * configuration out to all of them (simulation_configured). */
    void VehicleController::initialize() {
        _initialize_simulation();
        update_state();
    }

    void VehicleController::process_components(const double p_delta) {
        ++state_serial;
        for (const Ref<VehicleComponent> &component: components) {
            component->process(p_delta);
        }
    }

    /// Only marks the state for a rebuild - whoever reads it gets it fresh (see get_state())
    void VehicleController::update_state() {}

    void VehicleController::emit_position_changed_if_needed() {
        const Vector3 position = get_world_position();
        if (position.distance_to(last_emitted_position) < POSITION_CHANGED_MIN_DISTANCE) {
            return;
        }
        last_emitted_position = position;
        emit_signal(position_changed_signal, position);
    }


    void VehicleController::_fill_state_dictionary(Dictionary &p_state) const {
        if (!is_simulation_ready()) {
            return;
        }
        p_state["mass_total"] = get_mass_total();
        p_state["velocity"] = get_velocity();
        p_state["speed"] = get_speed();
        p_state["acceleration"] = get_acceleration();
        p_state["total_distance"] = get_total_distance();
        p_state["direction"] = get_direction();
    }

    /* The whole vehicle's configuration: its own plus every component's, composed when asked.
     * Unlike the state it is not a view on the backend - the wrapper's own properties and enums
     * are the authoring source of truth, and the backend is configured from them. */
    Dictionary VehicleController::get_config() const {
        Dictionary result;
        _fill_config_dictionary(result);
        for (const Ref<VehicleComponent> &component: components) {
            component->_fill_config_dictionary(result);
        }
        return result;
    }

    void VehicleController::emit_config_changed() {
        emit_signal(config_changed);
    }

    /// A proxy, not a store: the vehicle keeps no state Dictionary of its own. Every value is
    /// answered by the component that owns it, and this walks them by name for the callers that
    /// still want one - a console, a test, a diagnostic dump. Nothing on the frame path builds it.
    /* The whole vehicle's dump: its own share plus every component's. Expensive on purpose -
     * a console, a test or a diagnostic asks for it, never a per-frame reader. */
    Ref<VehicleComponent> VehicleController::get_component(const VehicleComponentType::Type p_type) const {
        return _get_component_of_type(p_type);
    }

    TypedArray<VehicleComponent> VehicleController::find_components(const VehicleComponentType::Type p_type) const {
        return _find_components_of_type(p_type);
    }

    Ref<VehicleComponent> VehicleController::_get_component_of_type(const int p_type) const {
        for (const Ref<VehicleComponent> &component: components) {
            if (component->get_component_type() == p_type) {
                return component;
            }
        }
        return Ref<VehicleComponent>();
    }

    TypedArray<VehicleComponent> VehicleController::_find_components_of_type(const int p_type) const {
        TypedArray<VehicleComponent> found;
        for (const Ref<VehicleComponent> &component: components) {
            if (component->get_component_type() == p_type) {
                found.push_back(component);
            }
        }
        return found;
    }

    TypedArray<VehicleComponent> VehicleController::find_generic_components(const StringName &p_tag) const {
        TypedArray<VehicleComponent> found;
        for (const Ref<VehicleComponent> &component: components) {
            if (component->get_component_type() == VehicleComponentType::COMPONENT_GENERIC &&
                component->get_component_tag() == p_tag) {
                found.push_back(component);
            }
        }
        return found;
    }

    /* Attaching a component is a complete operation: it joins the vehicle and, when the vehicle
     * is already running, its configuration is written to the backend and announced there and
     * then. A component added to a built vehicle - a modder's, or one a test adds - must not
     * leave the vehicle describing geometry it does not have. */
    void VehicleController::add_component(const Ref<VehicleComponent> &p_component) {
        ERR_FAIL_COND(p_component.is_null());
        components.push_back(p_component);
        if (!in_system) {
            return;
        }
        p_component->attach(this);
        if (is_simulation_ready()) {
            p_component->apply_config();
        }
    }

    void VehicleController::remove_component(const Ref<VehicleComponent> &p_component) {
        ERR_FAIL_COND(p_component.is_null());
        p_component->detach();
        components.erase(p_component);
    }

    void VehicleController::set_components(const TypedArray<VehicleComponent> &p_components) {
        _detach_components();
        components.clear();
        for (int index = 0; index < p_components.size(); ++index) {
            add_component(p_components[index]);
        }
    }

    TypedArray<VehicleComponent> VehicleController::get_components() const {
        TypedArray<VehicleComponent> result;
        for (const Ref<VehicleComponent> &component: components) {
            result.push_back(component);
        }
        return result;
    }

    /* Every component goes with the vehicle; nothing outside it holds one. */
    void VehicleController::shutdown() {
        in_system = false;
        _unregister_commands();
        // the handle belongs to RailVehicle3D, which frees it with itself
        rid = RID();
    }

    /* The components let go of the vehicle; they stay its components - the list is its
     * configuration - and join it again only in a vehicle brought into the system again. */
    void VehicleController::_detach_components() {
        for (const Ref<VehicleComponent> &component: components) {
            component->detach();
        }
    }

    void VehicleController::register_component(VehicleComponent *p_component) {
        p_component->attach_implementation(implementation_server);
    }

    void VehicleController::unregister_component(VehicleComponent *p_component) {
        p_component->attach_implementation(ObjectID());
    }

    Dictionary VehicleController::get_state() const {
        Dictionary result;
        _fill_state_dictionary(result);
        for (const Ref<VehicleComponent> &component: components) {
            if (component->get_enabled()) {
                component->_fill_state_dictionary(result);
            }
        }
        return result;
    }

    Vector3 VehicleController::get_world_position() const {
        return get_world_transform().get_origin();
    }

    Transform3D VehicleController::get_world_transform() const {
        return world_transform;
    }

    void VehicleController::set_world_transform(const Transform3D &p_transform) {
        world_transform = p_transform;
    }

    void VehicleController::set_implementation(const StringName &p_implementation) {
        implementation = p_implementation;
    }

    StringName VehicleController::get_implementation() const {
        return implementation;
    }

    void VehicleController::set_vehicle_rid(const RID &p_vehicle_rid) {
        rid = p_vehicle_rid;
    }

    RID VehicleController::_get_rid() const {
        return rid;
    }

    void VehicleController::command_executed(const String &p_command, const Variant &p_p1, const Variant &p_p2) {
        ++state_serial;
        update_state();
        emit_signal(command_received, p_command, p_p1, p_p2);
    }

    uint64_t VehicleController::get_state_serial() const {
        return state_serial;
    }

    /* A handler takes as many of the two arguments as it declares; its return value says whether
     * the command was accepted (#43), and Variant() means nothing handled it. */
    Variant VehicleController::send_command(const StringName &p_command, const Variant &p_p1, const Variant &p_p2) {
        Variant result;
        if (const Callable *handler = commands.getptr(p_command); handler != nullptr) {
            Array args;
            const int argc = static_cast<int>(handler->get_argument_count());
            if (argc > 0) {
                args.append(p_p1);
            }
            if (argc > 1) {
                args.append(p_p2);
            }
            result = handler->callv(args);
        } else {
            UtilityFunctions::push_error(vformat("%s: Unknown command: %s", vehicle_id, p_command));
        }
        command_executed(p_command, p_p1, p_p2);
        return result;
    }


} // namespace godot
