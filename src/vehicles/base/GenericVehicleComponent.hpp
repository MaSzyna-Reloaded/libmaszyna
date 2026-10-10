#pragma once
#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/binder_common.hpp>

#include "VehicleComponent.hpp"

namespace godot {
    class GenericVehicleComponent : public VehicleComponent {
            GDCLASS(GenericVehicleComponent, VehicleComponent)


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_GENERIC;
            }

        private:
            static void _bind_methods();
            /* The node carrying the modder's script. A component is not a node, so the script
             * lives on the proxy that authored it and the calls go back there. The proxy may be
             * freed before the component, so it is held by its ObjectID. */
            ObjectID script_owner;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;
            void _do_process_component(double p_delta) override;

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            virtual void _process_component(double p_delta);
            virtual Dictionary _get_component_state();
            virtual Dictionary _get_component_config();
            /* Set by GenericVehicleComponentNode when it puts this component into a vehicle. */
            void set_script_owner(const ObjectID &p_owner);
            Object *script_target();

            Dictionary get_vehicle_state();
    };
} // namespace godot
