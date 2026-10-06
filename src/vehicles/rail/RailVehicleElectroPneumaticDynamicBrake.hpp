#pragma once
#include "macros.hpp"
#include "vehicles/base/VehicleController.hpp"
#include "vehicles/rail/RailVehicleBrake.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/gd_extension.hpp>

namespace godot {
    class VehicleController;
    class RailVehicleElectroPneumaticDynamicBrake : public RailVehicleComponent {
            GDCLASS(RailVehicleElectroPneumaticDynamicBrake, RailVehicleComponent)


        public:
            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_EP_ED_BRAKE;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual double get_ed_braking_ep_delay() const = 0;
            virtual double get_ep_max_brake_engagement_speed() const = 0;
            virtual double get_ep_min_regenerative_braking() const = 0;
            virtual double get_ep_force() const = 0;
            virtual bool get_ep_fuse() const = 0;
            enum CouplerCheck {
                NONE = 0,
                FRONT = 1,
                BACK = 2,
            };
            virtual void set_ep_brake_force(int p_value) = 0;
            virtual void switch_ep_fuse(bool p_value) = 0;

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        private:
            MAKE_MEMBER_GS_NR(CouplerCheck, coupler_check, CouplerCheck::NONE);
            MAKE_MEMBER_GS(float, electro_pneumatic_brake_delay, 0.0f);
            MAKE_MEMBER_GS(float, electro_pneumatic_min_regenerative_braking, 0.0f);
            MAKE_MEMBER_GS(float, electro_pneumatic_max_ep_brake_engagement_speed, 0.0f);
            MAKE_MEMBER_GS(bool, ep_brake_fuse, false);
            /* Blending: (laczenie trybow hamowania EP+ED) */
            MAKE_MEMBER_GS(double, blending_max_velocity, 0.0);
            MAKE_MEMBER_GS(double, blending_min_velocity, 0.0);
            MAKE_MEMBER_GS(double, blending_reference_velocity, 0.0);
            MAKE_MEMBER_GS(double, blending_max_deceleration, 9.81);
            MAKE_MEMBER_GS(bool, blending_velocity_correction, false);
            MAKE_MEMBER_GS(bool, blending_load_correction, false);
            MAKE_MEMBER_GS(double, blending_min_ed_brake_request, 0.0);
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleElectroPneumaticDynamicBrake::CouplerCheck)
