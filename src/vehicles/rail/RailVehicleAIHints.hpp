#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class VehicleController;
    class RailVehicleAIHints : public RailVehicleComponent {
            GDCLASS(RailVehicleAIHints, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_AI_HINTS;
            }

        private:
            static void _bind_methods();

        public:
            /* Pantstate= : suggested pantograph setup for the AI driver */
            enum PantographState {
                /* the driver's own choice: the pantograph at the rear the way it drives
                 * (control_pantographs(), Driver.cpp:6258) */
                PANTOGRAPH_STATE_AUTOMATIC = 0,
                PANTOGRAPH_STATE_FRONT = 1,
                PANTOGRAPH_STATE_REAR = 2,
                PANTOGRAPH_STATE_BOTH = 3,
            };
            MAKE_MEMBER_GS_NR(PantographState, pantograph_state, PANTOGRAPH_STATE_AUTOMATIC);
            MAKE_MEMBER_GS(bool, raise_pantographs_when_idle, true);
            MAKE_MEMBER_GS(double, local_brake_acceleration_factor, 1.05); // MOVER.h:2286
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleAIHints::PantographState)
