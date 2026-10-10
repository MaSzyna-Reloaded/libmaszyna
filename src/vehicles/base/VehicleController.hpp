#pragma once
#include "VehicleComponentType.hpp"
#include "macros.hpp"
#include <godot_cpp/classes/ref.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/transform3d.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>


namespace godot {
    class VehicleComponent;


    /// The vehicle itself: its configuration, its components and the operations that change them,
    /// with no statement about what simulates it - that is the implementation's business. It is
    /// not a node - VehiclePhysicsNode is the vehicle's presence in the tree, and it owns one of
    /// these. Reached from outside by RID, through VehicleServer.
    ///
    /// A Resource: its properties (and its components') are the vehicle's stored configuration,
    /// so a parsed vehicle is saved and loaded as it is (the FIZ cache, a .tres), and a vehicle is
    /// built from a copy of it (VehiclePhysicsNode).
    class VehicleController : public Resource {
            GDCLASS(VehicleController, Resource)
        public:
        private:
            StringName implementation;
            /// Bumped by every step (process_components()) and every command (command_executed());
            /// what tells a cached state dump that it is stale.
            uint64_t state_serial = 0;
            /// Where the vehicle is in the world, handed down by the server that places it
            /// (set_world_transform())
            Transform3D world_transform;
            /// What this vehicle answers to, registered by itself and its components. A vehicle
            /// holds its own commands, so a scenery name shared by two vehicles, or none at all,
            /// leaves every one of them commandable.
            HashMap<StringName, Callable> commands;

        protected:
            /* The lookups behind get_component()/find_components(), by a VehicleComponentType or
             * a kind of vehicle's own type numbered on from it - a kind of vehicle answers its
             * own typed lookup through them (RailVehicleController::get_rail_component()) */
            Ref<VehicleComponent> _get_component_of_type(int p_type) const;
            TypedArray<VehicleComponent> _find_components_of_type(int p_type) const;
            /// Writes the wrapper's configuration - the vehicle's and every component's - to the
            /// backend, then announces that the backend carries it.
            void apply_configuration();
            /* Creates the simulation behind the vehicle and writes its configuration there. */
            virtual void _initialize_simulation() = 0;
            /* The simulation behind the vehicle exists (p_implementation, the
             * VehicleImplementationServer that runs it) or is about to go (ObjectID()): every
             * component takes it, and every one joining later too. */
            void _attach_implementation(const ObjectID &p_implementation);
            virtual void _fill_config_dictionary(Dictionary &p_config) const = 0;
            /* The vehicle's own share of the dump - what every vehicle has, whatever it is
             * made of. Its components add theirs. */
            virtual void _fill_state_dictionary(Dictionary &p_state) const;
            /* The vehicle's own commands, registered when it joins the system and given back on
             * shutdown - a kind of vehicle adds its own. */
            virtual void _register_commands() {}
            virtual void _unregister_commands() {}

        public:
            enum Category {
                CATEGORY_TRAIN = 1,
                CATEGORY_ROAD = 2,
                CATEGORY_SHIP = 4,
                CATEGORY_AIRPLANE = 8,
            };
            /* The reverser, counted from the cab it is set in (DirActive): forward is forward from
             * that cab, and the vehicle's own direction is this times the active cab (DirAbsolute,
             * Mover.cpp:669) */
            enum Direction {
                DIRECTION_BACKWARD = -1,
                DIRECTION_NEUTRAL = 0,
                DIRECTION_FORWARD = 1,
            };


            /// The simulation now carries the configuration (the vehicle's and every component's)
            static const char *simulation_configured_signal;
            /// The simulation behind the vehicle exists and is configured
            static const char *simulation_initialized_signal;
            static const char *command_received;
            static const char *config_changed;
            static const char *position_changed_signal;

            Dictionary get_config() const;
            /* One of this vehicle's components (re)applied its configuration. */
            void emit_config_changed();
            // NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): Godot's GDCLASS dispatches to this name
            void _notification(int p_what);
            Variant
            send_command(const StringName &p_command, const Variant &p_p1 = Variant(), const Variant &p_p2 = Variant());
            PackedStringArray get_commands() const;
            bool has_command(const StringName &p_command) const;
            /* A command has run against this vehicle. Its state has moved on in the middle of a
             * step, so it bumps the serial below as a step does (VehicleServer::vehicle_dump_state). */
            void
            command_executed(const String &p_command, const Variant &p_p1 = Variant(), const Variant &p_p2 = Variant());
            uint64_t get_state_serial() const;
            void register_command(const StringName &p_command, const Callable &p_callable);
            void unregister_command(const StringName &p_command);
            /* One tick of everything the vehicle is made of, after its physics has moved. The
             * components have no _process of their own to be driven by a scene tree. */
            /* Brings the vehicle up: its simulation, its state and the signals whose initial
             * value listeners expect. Called by whatever owns the vehicle, once it is built -
             * it used to wait for NOTIFICATION_READY, which a vehicle outside a tree never gets. */
            /// Whether this vehicle's simulation exists yet. A vehicle is a vehicle from the
            /// moment it is built, but nothing can be coupled to it or read off it until the
            /// backend behind it is there.
            virtual bool is_simulation_ready() const = 0;
            void attach_to_system();
            /// Lets go of everything this vehicle holds - its components, its registration and
            /// its simulation - without destroying the vehicle, so every reference to it stays
            /// valid across a rebuild.
            virtual void release();
            virtual void initialize();
            /* The reverse: the vehicle gives its commands back and lets go of its handle. */
            void shutdown();
            void process_components(double p_delta);
            virtual void update_state();
            /// Straight from the backend, for the per-frame readers that only want this one number
            /// and would otherwise force the whole state dictionary to be rebuilt
            virtual double get_velocity() const = 0;
            /// Straight from the backend, like get_velocity() - the speed readers want this
            /// one number, not the whole state
            virtual double get_speed() const = 0;
            /// The acceleration along the track [m/s2], every force counted (AccS)
            virtual double get_acceleration() const = 0;
            /// The rest of what every vehicle has, whatever it is made of. Read straight from the
            /// backend - nothing is stored, and the dump is built from these.
            virtual double get_mass_total() const = 0;
            virtual double get_total_distance() const = 0;
            virtual Direction get_direction() const = 0;
            virtual void apply_config() = 0;
            /// The second pass of apply_configuration(): what depends on the whole vehicle, written
            /// once every component has applied its own configuration
            virtual void apply_vehicle_config() {}
            virtual bool is_physics_active() const = 0;
            static void _bind_methods();
            /* This vehicle's handle in VehicleServer, set when the server attaches it - what
             * Resource.get_rid() answers, as a Mesh answers its RenderingServer handle. */
            void set_vehicle_rid(const RID &p_vehicle_rid);
            RID _get_rid() const override;
            void emit_position_changed_if_needed();
            Vector3 get_world_position() const;
            Transform3D get_world_transform() const;
            /* The vehicle's place in the world - state its owner hands down: only the server
             * that places the vehicle sets it, where the placement changes */
            void set_world_transform(const Transform3D &p_transform);
            /* The name of what simulates this vehicle, as registered with VehicleServer
             * (VehicleServer::implementation_register()) - the server hands that one the step */
            void set_implementation(const StringName &p_implementation);
            StringName get_implementation() const;
            MAKE_MEMBER_GS(String, vehicle_id, "");
            MAKE_MEMBER_GS(double, mass, 0.0);
            MAKE_MEMBER_GS(double, power, 0.0);
            MAKE_MEMBER_GS(double, max_velocity, 0.0);
            MAKE_MEMBER_GS_NR(Category, category, CATEGORY_TRAIN);
            MAKE_MEMBER_GS(double, dimensions_length, 0.0);
            MAKE_MEMBER_GS(double, dimensions_height, 0.0);
            MAKE_MEMBER_GS(double, dimensions_width, 0.0);
            MAKE_MEMBER_GS(double, dimensions_drag_coefficient, 0.0);
            MAKE_MEMBER_GS(double, dimensions_floor_height, 0.96);

            // Mirrors the original engine's scenery-line velocity token (TDynamicObject::Init's
            // `driveractive = (fVel != 0.0)`): a vehicle with initial_velocity == 0.0 is "not
            // ready to depart" (ReadyFlag=false, battery stays off until switched on manually);
            // any non-zero value (scenario authors commonly use 0.1 for a stationary-but-ready
            // vehicle) marks it ready, so CheckLocomotiveParameters() turns the battery on per
            // cntrl_battery_start_mode.
            MAKE_MEMBER_GS(double, initial_velocity, 0.0);

            /* The whole state of this vehicle, by name: this vehicle and every enabled component,
             * composed on each call. A reader asking per frame takes the cached dump instead
             * (VehicleServer::vehicle_dump_state()). */
            Dictionary get_state() const;

            /* This vehicle's components, in the order they joined - which is the order of the
             * FIZ sections that built them. They announce themselves rather than being searched
             * for in the subtree. */
            /* The component of a kind, or null when this vehicle has none. One per kind: a
             * vehicle has one brake system and one engine, whatever kind it is. */
            Ref<VehicleComponent> get_component(VehicleComponentType::Type p_type) const;
            /// Every component of a type - a vehicle has two couplers, one per end
            TypedArray<VehicleComponent> find_components(VehicleComponentType::Type p_type) const;
            /* Every scripted component carrying this tag - modders add as many as they like */
            TypedArray<VehicleComponent> find_generic_components(const StringName &p_tag) const;

            /* Takes a component into the vehicle and owns it from then on - it is ticked with
             * the vehicle and freed with it. The shape Node::add_child() has, for the same
             * reason: the thing being handed over has no life of its own outside its owner. */
            void add_component(const Ref<VehicleComponent> &p_component);
            /* Lets a component go: it leaves the vehicle and is no longer ticked nor read with it */
            void remove_component(const Ref<VehicleComponent> &p_component);
            /* The components as stored configuration: setting them lets go of the ones the vehicle
             * had and takes these in, in their order (add_component()) */
            void set_components(const TypedArray<VehicleComponent> &p_components);
            TypedArray<VehicleComponent> get_components() const;

            void register_component(VehicleComponent *p_component);
            void unregister_component(VehicleComponent *p_component);

        private:
            Vector<Ref<VehicleComponent>> components;
            /* In the system as a live vehicle (attach_to_system() .. release()): its components
             * are joined to it. A description - the same class, only stored - never is. */
            bool in_system = false;
            void _detach_components();
            RID rid;
            /* What runs the simulation while it exists (_attach_implementation()) */
            ObjectID implementation_server;
            /* position_changed is emitted once the vehicle has moved this far (m) */
            static constexpr double POSITION_CHANGED_MIN_DISTANCE = 1.0;
            /* Far from anywhere, so the first position is always announced */
            static constexpr real_t UNSET_POSITION = 1e10;
            Vector3 last_emitted_position = Vector3(UNSET_POSITION, UNSET_POSITION, UNSET_POSITION);
    };
} // namespace godot

VARIANT_ENUM_CAST(VehicleController::Category);
VARIANT_ENUM_CAST(VehicleController::Direction);
