#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleUniversalControllerListItem.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class RailVehicleUniversalController : public RailVehicleComponent {
            GDCLASS(RailVehicleUniversalController, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_UNIVERSAL_CONTROLLER;
            }

        private:
            static void _bind_methods();

            bool integrated_brake_pn = false;
            bool integrated_brake = false;
            /* IntegratedLocBrake: the controller's braking positions work the local brake */
            bool integrated_local_brake = false;
            int selector_position = 0;
            TypedArray<RailVehicleUniversalControllerListItem> positions;

        public:
            MAKE_GS(bool, integrated_brake_pn);
            MAKE_GS(bool, integrated_brake);
            MAKE_GS(bool, integrated_local_brake);
            MAKE_GS(int, selector_position);
            MAKE_GS_NR(TypedArray<RailVehicleUniversalControllerListItem>, positions);
    };
} // namespace godot
