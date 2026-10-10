#pragma once
#include "GenericVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    /* Where a scripted vehicle component is authored.
     *
     * A component is not a node - it belongs to a vehicle, not to a scene - but a modder needs
     * somewhere to put one and something to attach a script to. This is that place: put it under
     * a VehiclePhysicsNode, write `extends GenericVehicleComponentNode`, and the vehicle you sit
     * under gets a component that calls your script.
     *
     * The methods a script overrides are the ones GenericVehicleComponent declares:
     * _process_component(delta), _get_component_state(), _get_component_config(). */
    class GenericVehicleComponentNode : public Node {
            GDCLASS(GenericVehicleComponentNode, Node)

        private:
            Ref<GenericVehicleComponent> component;
            /* The vehicle the component was put into, to take it out of again on leaving */
            RID vehicle;

        protected:
            static void _bind_methods();
            void _notification(int p_what); // NOLINT(bugprone-derived-method-shadowing-base-method)

        public:
            /* The component this node put into the vehicle, for anything that wants it directly */
            Ref<GenericVehicleComponent> get_component() const;

            /* Shortcuts for a script written on this node, so it does not go through
             * get_component() for what it calls most. They belong to the component. */
            void register_command(const String &p_command, const Callable &p_callback);
            void unregister_command(const String &p_command);
            Ref<VehicleController> get_controller() const;
            void log_debug(const String &p_line);
            void log_warning(const String &p_line);
    };
} // namespace godot
