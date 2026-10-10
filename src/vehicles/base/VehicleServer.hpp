#pragma once
#include "vehicles/base/VehicleComponentType.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/base/VehiclePerson.hpp"
#include "vehicles/base/VehiclePersonRole.hpp"

#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    class VehicleComponent;

    /* The vehicles, whatever they run on: their handles, the controller behind each, the names a
     * scenery gives them, their cabins and who sits in them, their commands, components and dumps.
     * What a kind of vehicle adds - a rail vehicle's track, stepping, couplers and the kinds of its
     * cabins - is a server of its own (RailVehicleServer), which takes a vehicle created here and
     * knows it by the same handle. */
    class VehicleServer : public Object {
            GDCLASS(VehicleServer, Object)

        public:
            static VehicleServer *get_instance() {
                return Object::cast_to<VehicleServer>(Engine::get_singleton()->get_singleton("VehicleServer"));
            }

        private:
            /* A controller this server holds (controller_create()): the copy of a description it
             * was configured with, and the vehicle it drives while bound */
            struct Controller {
                    Ref<VehicleController> controller;
                    RID vehicle;
            };

            struct Vehicle {
                    /* The controller driving this vehicle (vehicle_bind_controller()), RID() for none */
                    RID controller;
                    /* What the scenery placed the vehicle with, handed to every controller bound
                     * to it before its simulation starts (the `.scn` `dynamic` entry) */
                    double initial_velocity = 0.0;
                    /* Its cabins, in the order they were attached */
                    Vector<RID> cabins;
                    /* The implementation that steps it, as its controller names it */
                    StringName implementation;
                    /* What a scenery calls this vehicle. Only the things that know a vehicle by
                     * name alone need it - an event, the console, the radio, a `.scn` command -
                     * and they reach the vehicle through vehicle_get_rid_by_name(). */
                    String name;
                    /* The last dump handed out, and the controller's state serial it was built
                     * at. A cab is dozens of widgets asking the same vehicle in one frame, and
                     * only a step or a command moves the values between them. */
                    Dictionary state_dump;
                    uint64_t state_dump_serial = 0;
                    bool state_dump_valid = false;
            };

            /* A place in a vehicle people sit in (cabin_create()): the vehicle it is attached to
             * and who is in it, in what role */
            struct Cabin {
                    RID vehicle;
                    HashMap<RID, VehiclePersonRole::Role> persons;
            };

            HashMap<RID, Vehicle> vehicles;
            HashMap<RID, Cabin> cabins;
            /* The cabin each person aboard sits in - a person is in one cabin at a time */
            HashMap<RID, RID> person_cabins;
            HashMap<String, RID> vehicles_by_name;
            int64_t next_vehicle_id = 0;
            HashMap<RID, Controller> controllers;
            int64_t next_controller_id = 0;
            /* What simulates vehicles, by the name a controller gives (implementation_register()) */
            HashMap<StringName, ObjectID> implementations;
            /// Stepping holds SimulationServer's clock and steps as it advances
            bool stepping = false;
            bool stepping_enabled = true;
            /* The vehicles each implementation steps, with their controllers - set where a
             * controller is bound to a vehicle or unbound from it, not per step */
            struct SteppedGroup {
                    Vector<RID> vehicles;
                    Vector<Ref<VehicleController>> controllers;
            };
            HashMap<StringName, SteppedGroup> stepped_groups;

            void _refresh_stepping();
            void _on_simulation_advanced(double p_seconds);

            VehicleController *_get_controller(const RID &p_vehicle) const;
            /* The controller's own events, relayed under the handle, so whoever follows a vehicle
             * never holds its controller - connected when a controller is attached, disconnected
             * when it is replaced or the vehicle is freed */
            void _connect_relays(const RID &p_vehicle);
            void _disconnect_relays(const RID &p_vehicle);
            void _on_vehicle_moved(const Vector3 &p_position, const RID &p_vehicle);
            void _on_vehicle_command_received(
                    const String &p_command, const Variant &p_p1, const Variant &p_p2, const RID &p_vehicle);
            void _on_vehicle_configured(const RID &p_vehicle);
            void _on_vehicle_config_changed(const RID &p_vehicle);
            void _on_person_freed(const RID &p_person);
            /* Whether a cabin other than p_person's own seat has its driver */
            bool _cabin_has_other_driver(const Cabin &p_cabin, const RID &p_person) const;

        protected:
            static void _bind_methods();

        public:
            static const char *vehicle_moved_signal;
            static const char *vehicle_command_received_signal;
            static const char *vehicle_freed_signal;
            /* Another controller drives the vehicle now - whoever relays its events reconnects */
            static const char *vehicle_controller_changed_signal;
            /* The simulation behind the vehicle exists and carries its configuration */
            static const char *vehicle_configured_signal;
            /* A component of the vehicle (re)applied its configuration */
            static const char *vehicle_config_changed_signal;
            /* The cabin is no part of the vehicle any more - detached or freed, everybody out first
             * (vehicle: RID, cabin: RID) */
            static const char *vehicle_cabin_detached_signal;
            /* Somebody sat down in a cabin (cabin: RID, person: RID, role: VehiclePersonRole.Role) */
            static const char *cabin_person_entered_signal;
            /* Somebody left a cabin (cabin: RID, person: RID) */
            static const char *cabin_person_left_signal;
            /* Somebody took another role in the cabin (cabin: RID, person: RID, role) */
            static const char *cabin_person_role_changed_signal;
            /* Somebody went over to another cabin - of the vehicle or, through a gangway, of another
             * one - in the role it had (person: RID, cabin: RID, previous: RID) */
            static const char *cabin_person_moved_signal;

            VehicleServer();
            ~VehicleServer() override;

            /* What simulates the vehicles whose controller names p_name, by the instance id of its
             * VehicleImplementationServer - the extension registers the Mover, an addon may
             * register its own */
            void implementation_register(const StringName &p_name, uint64_t p_implementation_id);
            void implementation_unregister(const StringName &p_name);
            PackedStringArray implementation_get_list() const;

            /* One step of every vehicle: each implementation is handed its own vehicles, at once.
             * Driven by SimulationServer's clock, and callable directly with an explicit delta
             * where the caller wants to decide when it happens. */
            void stepping_advance(double p_delta);
            /* Freezing the step while a scenery is torn down: the vehicles are freed one by one and
             * stepping a registry that is being emptied is work for nothing. */
            void stepping_set_enabled(bool p_enabled);
            bool stepping_is_enabled() const;

            RID vehicle_create();
            void vehicle_free(const RID &p_vehicle);
            bool vehicle_exists(const RID &p_vehicle) const;
            /* A controller of this server, empty until configured */
            RID controller_create();
            /* The controller becomes a copy of p_description (a VehicleController with its
             * components - its stored configuration); a vehicle it drives is restarted on it */
            void controller_configure(const RID &p_controller, const Ref<VehicleController> &p_description);
            void controller_free(const RID &p_controller);
            /* The vehicle is driven by p_controller from now on: the controller it had lets go of
             * its simulation, this one takes the scenery's values and starts its own. RID()
             * unbinds; binding the same pair again restarts the vehicle's simulation. */
            void vehicle_bind_controller(const RID &p_vehicle, const RID &p_controller);
            /* The controller object bound to the vehicle, 0 without one - C++ only and unbound,
             * for the servers that step and couple the vehicles */
            uint64_t vehicle_get_controller_instance_id(const RID &p_vehicle) const;
            /* The controller the vehicle runs on - the copy its simulation was built from, null
             * without one */
            Ref<VehicleController> vehicle_get_controller(const RID &p_vehicle) const;
            /* What the scenery placed the vehicle with - the velocity it starts with. Handed to its
             * controller when it is bound. */
            void vehicle_set_initial_velocity(const RID &p_vehicle, double p_velocity);
            /* The scenery's name for this vehicle, and the way back from one. A name is what a
             * `.scn`, an event or the console has; everything that holds the vehicle uses its
             * handle and never comes through here (TrackServer::track_get_rid_by_name() is the
             * same shape, for the same reason). */
            void vehicle_set_name(const RID &p_vehicle, const String &p_name);
            String vehicle_get_name(const RID &p_vehicle) const;
            RID vehicle_get_rid_by_name(const String &p_name) const;
            TypedArray<RID> vehicle_get_rids() const;
            /* Whether the simulation behind the vehicle exists yet - nothing can be read off a
             * vehicle before it does */
            bool vehicle_is_simulation_ready(const RID &p_vehicle) const;

            /* A place people sit in, of nothing yet; attached to a vehicle it can be entered. A
             * vehicle without a cabin cannot be entered at all. A freed vehicle frees its cabins. */
            RID cabin_create();
            void cabin_free(const RID &p_cabin);
            void vehicle_cabin_attach(const RID &p_vehicle, const RID &p_cabin);
            /* Whoever sits in the cabin leaves it first */
            void vehicle_cabin_detach(const RID &p_vehicle, const RID &p_cabin);
            TypedArray<RID> vehicle_get_cabins(const RID &p_vehicle) const;
            int vehicle_get_cabin_count(const RID &p_vehicle) const;
            RID cabin_get_vehicle(const RID &p_cabin) const;

            /* Who sits where, in what role. Refused - with the reason, and nothing changed - for a
             * person aboard already (ERR_ALREADY_IN_USE), a cabin of no vehicle or a role that is
             * no role (ERR_INVALID_PARAMETER), and the driver's seat of a cabin somebody else
             * drives from (ERR_UNAVAILABLE): one driver a cabin, and taking it over is that one's
             * change of role first. */
            Error cabin_person_enter(const RID &p_cabin, const RID &p_person, VehiclePersonRole::Role p_role);
            void cabin_person_leave(const RID &p_cabin, const RID &p_person);
            Error cabin_person_change_role(const RID &p_cabin, const RID &p_person, VehiclePersonRole::Role p_role);
            /* Over to another cabin, of the same vehicle or another, in the role the person has;
             * ERR_UNAVAILABLE for a driver where one already drives */
            Error cabin_person_move(const RID &p_person, const RID &p_cabin);
            /* Who sits in the cabin, or in any cabin of the vehicle, in p_role -
             * VEHICLE_PERSON_ROLE_ANY for everybody */
            TypedArray<VehiclePerson> cabin_list_persons(const RID &p_cabin, VehiclePersonRole::Role p_role) const;
            bool cabin_has_person_role(const RID &p_cabin, VehiclePersonRole::Role p_role) const;
            TypedArray<VehiclePerson> vehicle_list_persons(const RID &p_vehicle, VehiclePersonRole::Role p_role) const;
            bool vehicle_has_person_role(const RID &p_vehicle, VehiclePersonRole::Role p_role) const;
            /* The cabin the person sits in, and its vehicle - RID() for a person on foot */
            RID person_get_cabin(const RID &p_person) const;
            RID person_get_vehicle(const RID &p_person) const;
            /* The role the person has where it sits, VEHICLE_PERSON_ROLE_ANY for a person on foot */
            VehiclePersonRole::Role person_get_role(const RID &p_person) const;
            /* Width (x), height (y) and length (z) of the body [m], in the vehicle's own frame */
            Vector3 vehicle_get_dimensions(const RID &p_vehicle) const;

            /* A command to one vehicle, by handle; returns what its handler answered (#43), or
             * Variant() when the vehicle has no such command */
            Variant vehicle_send_command(
                    const RID &p_vehicle, const StringName &p_command, const Variant &p_p1 = Variant(),
                    const Variant &p_p2 = Variant());
            /* The same command to every vehicle that has it */
            void vehicle_broadcast_command(
                    const StringName &p_command, const Variant &p_p1 = Variant(), const Variant &p_p2 = Variant());
            PackedStringArray vehicle_get_commands(const RID &p_vehicle) const;
            bool vehicle_has_command(const RID &p_vehicle, const StringName &p_command) const;

            /* The hot values, typed and by handle - which backend answers is not the caller's
             * business. Everything else is read from the component that owns it. */
            double vehicle_get_velocity(const RID &p_vehicle) const;
            double vehicle_get_speed(const RID &p_vehicle) const;
            /* The component of a kind, as a typed object - the shape
             * PhysicsServer3D::body_get_direct_state() has: a live view on the vehicle, valid
             * while the vehicle is. A per-frame reader takes it once and reads its properties. */
            Ref<VehicleComponent> vehicle_component_get(const RID &p_vehicle, VehicleComponentType::Type p_type) const;
            /* Scripted components carrying a tag of the modder's own choosing */
            TypedArray<VehicleComponent>
            vehicle_generic_component_find(const RID &p_vehicle, const StringName &p_tag) const;
            /* Everything this vehicle publishes, by name, in one Dictionary. Expensive on
             * purpose: a console, a test or a diagnostic dump asks for it, never a per-frame
             * reader - those take the component that owns the value and read its property. */
            Dictionary vehicle_dump_state(const RID &p_vehicle);
            Dictionary vehicle_dump_config(const RID &p_vehicle) const;
    };
} // namespace godot
