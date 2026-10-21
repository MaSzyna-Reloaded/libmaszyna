#pragma once
#include "maszyna/maszyna-mover/McZapkie/MOVER.h"
#include "maszyna/vehicles/MaszynaMoverComponent.hpp"
#include "vehicles/rail/RailVehicleElectroPneumaticDynamicBrake.hpp"

namespace godot {
    /* RailVehicleElectroPneumaticDynamicBrake on the vendored Mover - the only class here that knows TMoverParameters.
     */
    class MaszynaMoverRailVehicleElectroPneumaticDynamicBrake : public RailVehicleElectroPneumaticDynamicBrake,
                                                                public MaszynaMoverComponent {
            GDCLASS(MaszynaMoverRailVehicleElectroPneumaticDynamicBrake, RailVehicleElectroPneumaticDynamicBrake);

        protected:
            /* The vehicle's Mover, taken when its simulation starts and dropped before it is freed */
            void _implementation_changed() override {
                take_mover(
                        get_implementation(),
                        train_controller_node != nullptr ? train_controller_node->get_rid() : RID());
            }

        private:
            static void _bind_methods();

        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            double get_ed_braking_ep_delay() const override;
            double get_ep_max_brake_engagement_speed() const override;
            double get_ep_min_regenerative_braking() const override;
            double get_ep_force() const override;
            bool get_ep_fuse() const override;
            void switch_ep_fuse(bool p_value) override;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override {};

        private:
            CouplerCheck coupler_check = CouplerCheck::NONE;
            float electro_pneumatic_min_regenerative_braking = 0.0f;
            float electro_pneumatic_max_ep_brake_engagement_speed = 0.0f;
            float electro_pneumatic_brake_delay = 0.0f;
            /* Blending: (laczenie trybow hamowania EP+ED) */
            double blending_max_velocity = 0.0;
            double blending_min_velocity = 0.0;
            double blending_reference_velocity = 0.0;
            double blending_max_deceleration = 9.81;
            bool blending_velocity_correction = false;
            bool blending_load_correction = false;
            double blending_min_ed_brake_request = 0.0;

        public:
            MAKE_GS_NR(CouplerCheck, coupler_check);
            MAKE_GS(float, electro_pneumatic_min_regenerative_braking);
            MAKE_GS(float, electro_pneumatic_max_ep_brake_engagement_speed);
            MAKE_GS(float, electro_pneumatic_brake_delay);
            MAKE_GS(double, blending_max_velocity);
            MAKE_GS(double, blending_min_velocity);
            MAKE_GS(double, blending_reference_velocity);
            MAKE_GS(double, blending_max_deceleration);
            MAKE_GS(bool, blending_velocity_correction);
            MAKE_GS(bool, blending_load_correction);
            MAKE_GS(double, blending_min_ed_brake_request);
    };
} // namespace godot
