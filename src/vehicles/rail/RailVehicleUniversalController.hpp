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

        public:
            MAKE_MEMBER_GS(bool, integrated_brake_pn, false);
            MAKE_MEMBER_GS(bool, integrated_brake, false);
            /* IntegratedLocBrake: the controller's braking positions work the local brake */
            MAKE_MEMBER_GS(bool, integrated_local_brake, false);
            MAKE_MEMBER_GS(int, selector_position, 0);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleUniversalControllerListItem>, positions)
    };
} // namespace godot
