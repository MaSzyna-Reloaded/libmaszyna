#pragma once
#include "legacy/maszyna-mover/McZapkie/MOVER.h"
#include "legacy/vehicles/MoverComponent.hpp"
#include "vehicles/rail/RailVehicleLighting.hpp"

namespace godot {
    /* RailVehicleLighting on the vendored Mover - the only class here that knows TMoverParameters. */
    class MoverRailVehicleLighting : public RailVehicleLighting, public MoverComponent {
            GDCLASS(MoverRailVehicleLighting, RailVehicleLighting);

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
            int get_position() const override;
            double get_power() const override;
            int get_power_source() const override;
            bool get_front_headlight_upper_enabled() const override;
            bool get_front_headlight_left_enabled() const override;
            bool get_front_headlight_right_enabled() const override;
            bool get_front_redmarker_left_enabled() const override;
            bool get_front_redmarker_right_enabled() const override;
            bool get_rear_headlight_upper_enabled() const override;
            bool get_rear_headlight_left_enabled() const override;
            bool get_rear_headlight_right_enabled() const override;
            bool get_rear_redmarker_left_enabled() const override;
            bool get_rear_redmarker_right_enabled() const override;
            bool get_active_headlight_upper_enabled() const override;
            bool get_active_headlight_left_enabled() const override;
            bool get_active_headlight_right_enabled() const override;
            bool get_active_redmarker_left_enabled() const override;
            bool get_active_redmarker_right_enabled() const override;
            bool get_opposite_headlight_upper_enabled() const override;
            bool get_opposite_headlight_left_enabled() const override;
            bool get_opposite_headlight_right_enabled() const override;
            bool get_opposite_redmarker_left_enabled() const override;
            bool get_opposite_redmarker_right_enabled() const override;

        private:
            /* Lights[2][17] - readLightsList refuses a row past index 16 (Mover.cpp:8566) */
            static constexpr int LIGHTS_LIST_CAPACITY = 17;
            /* the preset that lights both ends of every vehicle (DynObj.cpp:7337) */
            static constexpr int LIGHTS_POSITION_ALL_ENDS = 18;
            TypedArray<RailVehicleLightListItem> light_position_list;
            /* DynObj's DimHeadlights - the vendored Mover has no dimmer of its own */
            bool headlights_dimmed = false;
            const std::unordered_map<LightEnd, Maszyna::end> light_end_map = {
                    {LIGHT_END_FRONT, Maszyna::end::front},
                    {LIGHT_END_REAR, Maszyna::end::rear},
            };
            const std::unordered_map<LightType, int> light_type_mask_map = {
                    {LIGHT_TYPE_HEADLIGHT_UPPER, Maszyna::light::headlight_upper},
                    {LIGHT_TYPE_HEADLIGHT_LEFT, Maszyna::light::headlight_left},
                    {LIGHT_TYPE_HEADLIGHT_RIGHT, Maszyna::light::headlight_right},
                    {LIGHT_TYPE_REDMARKER_LEFT, Maszyna::light::redmarker_left},
                    {LIGHT_TYPE_REDMARKER_RIGHT, Maszyna::light::redmarker_right},
            };
            bool _light_enabled(const TMoverParameters *p_mover, LightEnd p_end, LightType p_type) const;
            static LightEnd _active_end(const TMoverParameters *p_mover);
            static LightEnd _opposite_end(const TMoverParameters *p_mover);
            void _set_lights(TMoverParameters *p_mover) const;

        protected:
            void _apply_configuration() override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

        public:
            TypedArray<RailVehicleLightListItem> get_lights_list() override {
                return light_position_list;
            };
            void set_lights_list(const TypedArray<RailVehicleLightListItem> &p_list) override {
                light_position_list.clear();
                light_position_list.append_array(p_list);
            };
            void increase_light_selector_position() override;
            void decrease_light_selector_position() override;
            void light(const String &p_light, bool p_enabled) override;
            bool light_is_enabled(const String &p_light) const override;
            void light_switch(const String &p_light, bool p_enabled) override;
            void headlights_dim(bool p_enabled) override;
            bool get_headlights_dimmed() const override;
            bool get_any_light_enabled() const override;
    };
} // namespace godot
