#pragma once
#include "VehicleController.hpp"
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/gdvirtual.gen.inc>

namespace godot {
    /* A vehicle's presence in the scene tree.
     *
     * The vehicle itself is an object of the server, addressed by a RID; this node owns that
     * handle, builds the vehicle from a copy of its controller - a VehicleController with its
     * components, the vehicle's stored configuration - and frees it. It is the anchor everything in
     * the tree hangs off - a scripted component a modder adds, a cabin, a sound bank - so that
     * "which vehicle am I part of" is answered by where a node sits, not by a path it carries.
     *
     * It knows nothing about where the controller came from: it is given one, or a subclass builds
     * one (_build_controller()) - MaszynaRailVehiclePhysicsNode asks the .fiz builder for it, and
     * another format would be another subclass. */
    class VehiclePhysicsNode : public Node {
            GDCLASS(VehiclePhysicsNode, Node)

        private:
            /* The VehicleController implementation every vehicle is built with - the simulation
             * the extension ships, named once where the classes are registered. Held in a function
             * rather than a static member: a StringName cannot be built before the engine is up. */
            static StringName &controller_implementation();
            RID vehicle_rid;
            RID controller_rid;
            Ref<VehicleController> controller;
            void _build();
            String vehicle_id;
            double initial_velocity = 0.0;

        protected:
            static void _bind_methods();
            /* The vehicle exists and is about to be configured and (re)started: a kind of vehicle
             * hands its own servers what it knows of it (RailVehiclePhysicsNode) */
            virtual void _prepare_vehicle(const RID &p_vehicle) {}
            /* The controller a node that was given none builds its vehicle from, as it enters the
             * tree - once, before anything can see the vehicle */
            GDVIRTUAL0RC(Ref<VehicleController>, _build_controller)
            void _notification(int p_what); // NOLINT(bugprone-derived-method-shadowing-base-method)

        public:
            static const char *vehicle_changed_signal;

            /* C++ only: register_types says which simulation the vehicles run on. */
            static void set_controller_implementation(const StringName &p_class);

            /* The vehicle's configuration, a VehicleController its simulation is built from a copy of;
             * a new one rebuilds the vehicle, replacing whatever this node held. The running copy
             * is the vehicle's, reached by its handle (VehicleServer::vehicle_get_controller()). */
            void set_controller(const Ref<VehicleController> &p_controller);
            Ref<VehicleController> get_controller() const;

            /* This vehicle's handle, for anything that talks to the servers */
            RID get_vehicle_rid() const;

            /* Adds a component to this vehicle - what a proxy node in the tree calls when it
             * joins, so a modder's component reaches the vehicle it sits under. */
            void add_component(const Ref<VehicleComponent> &p_component);

            /* The vehicle's name (VehicleServer.vehicle_set_name()), set every time it is built. Not
             * derived from whatever file the description came from - a scenery names its vehicles,
             * a .fiz does not. */
            void set_vehicle_id(const String &p_vehicle_id);
            String get_vehicle_id() const;
            void set_initial_velocity(double p_velocity);
            double get_initial_velocity() const;
    };
} // namespace godot
