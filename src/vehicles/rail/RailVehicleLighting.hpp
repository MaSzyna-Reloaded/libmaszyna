#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include "vehicles/rail/RailVehicleController.hpp"
#include "vehicles/rail/RailVehicleElectricEngine.hpp"
#include "vehicles/rail/RailVehicleLightListItem.hpp"
#include <godot_cpp/classes/node.hpp>
#include <unordered_map>

namespace godot {
    class VehicleController;
    class RailVehicleLighting : public RailVehicleComponent {
            GDCLASS(RailVehicleLighting, RailVehicleComponent);


        public:
            int get_component_type() const override {
                return VehicleComponentType::COMPONENT_LIGHTING;
            }

        private:
            static void _bind_methods();

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual int get_position() const = 0;
            virtual double get_power() const = 0;
            virtual int get_power_source() const = 0;
            virtual bool get_front_headlight_upper_enabled() const = 0;
            virtual bool get_front_headlight_left_enabled() const = 0;
            virtual bool get_front_headlight_right_enabled() const = 0;
            virtual bool get_front_redmarker_left_enabled() const = 0;
            virtual bool get_front_redmarker_right_enabled() const = 0;
            virtual bool get_rear_headlight_upper_enabled() const = 0;
            virtual bool get_rear_headlight_left_enabled() const = 0;
            virtual bool get_rear_headlight_right_enabled() const = 0;
            virtual bool get_rear_redmarker_left_enabled() const = 0;
            virtual bool get_rear_redmarker_right_enabled() const = 0;
            virtual bool get_active_headlight_upper_enabled() const = 0;
            virtual bool get_active_headlight_left_enabled() const = 0;
            virtual bool get_active_headlight_right_enabled() const = 0;
            virtual bool get_active_redmarker_left_enabled() const = 0;
            virtual bool get_active_redmarker_right_enabled() const = 0;
            virtual bool get_opposite_headlight_upper_enabled() const = 0;
            virtual bool get_opposite_headlight_left_enabled() const = 0;
            virtual bool get_opposite_headlight_right_enabled() const = 0;
            virtual bool get_opposite_redmarker_left_enabled() const = 0;
            virtual bool get_opposite_redmarker_right_enabled() const = 0;
            enum LightEnd { LIGHT_END_FRONT, LIGHT_END_REAR };
            enum LightType {
                LIGHT_TYPE_HEADLIGHT_UPPER,
                LIGHT_TYPE_HEADLIGHT_LEFT,
                LIGHT_TYPE_HEADLIGHT_RIGHT,
                LIGHT_TYPE_REDMARKER_LEFT,
                LIGHT_TYPE_REDMARKER_RIGHT,
            };

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            static const char *selector_position_changed_signal;
            MAKE_MEMBER_GS(bool, lights_wrap_selector, false);
            /* MOVER.h:1701 LightsDefPos */
            MAKE_MEMBER_GS(int, lights_default_selector_position, 1);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::TrainPowerSource, light_source,
                    RailVehicleController::TrainPowerSource::POWER_SOURCE_GENERATOR);
            MAKE_MEMBER_GS_NR(
                    RailVehicleEngine::EngineType, source_generator_engine, RailVehicleEngine::EngineType::MAIN);
            MAKE_MEMBER_GS(double, source_accumulator_max_voltage, 0.0);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::TrainPowerSource, light_alternative_source,
                    RailVehicleController::TrainPowerSource::POWER_SOURCE_ACCUMULATOR);
            /* TPowerParameters defaults (MOVER.h:1032, the accumulator's MaxCapacity MOVER.h:950) */
            MAKE_MEMBER_GS(double, light_alternative_max_voltage, 0.0);
            MAKE_MEMBER_GS(double, light_alternative_capacity, 0.0);
            MAKE_MEMBER_GS_NR(
                    RailVehicleController::TrainPowerSource, source_accumulator_recharge_source,
                    RailVehicleController::TrainPowerSource::POWER_SOURCE_GENERATOR);
            /* Headlights: defaults of MOVER.h:2273-2280 */
            MAKE_MEMBER_GS(Color, head_light_color, Color(1, 1, 1));
            MAKE_MEMBER_GS(double, head_light_dimmed_multiplier, 0.6);
            MAKE_MEMBER_GS(double, head_light_normal_multiplier, 1.0);
            MAKE_MEMBER_GS(double, head_light_high_beam_dimmed_multiplier, 2.5);
            MAKE_MEMBER_GS(double, head_light_high_beam_normal_multiplier, 2.8);
            MAKE_MEMBER_GS(int, instrument_type, 0);
            virtual TypedArray<RailVehicleLightListItem> get_lights_list() = 0;
            virtual void set_lights_list(const TypedArray<RailVehicleLightListItem> &p_list) = 0;
            virtual void increase_light_selector_position() = 0;
            virtual void decrease_light_selector_position() = 0;
            /* The headlights dimmer (dimheadlights_sw:, Train.cpp:6125) */
            virtual void headlights_dim(bool p_enabled) = 0;
            virtual bool get_headlights_dimmed() const = 0;
            /* Whether any lamp is lit at either end (iLights[front] != 0 || iLights[rear] != 0) - what
             * an instrument light of the head lights follows (Train.cpp:9570) */
            virtual bool get_any_light_enabled() const = 0;
            // Direct per-light override, independent of the selector/"light programator"
            // (LightsPos + light_position_list) system above - sets/clears a single bit of
            // iLights directly, for debugging/testing individual bulbs regardless of what the
            // programator would normally compute. p_light matches the short name half of this
            // class's own state keys (state key = "lights/" + p_light + "_enabled"): e.g.
            // "front_headlight_left", "rear_redmarker_right".
            virtual void light(const String &p_light, bool p_enabled) = 0;
            // Whether the lamp light() names is lit, by the same name
            virtual bool light_is_enabled(const String &p_light) const = 0;
            // Cab-relative toggle for the actual MMD cabin switches (upperlight_sw:/leftlight_sw:
            // /rightlight_sw:/leftend_sw:/rightend_sw:/rearupperlight_sw:/rearleftlight_sw:/
            // rearrightlight_sw:/rearleftend_sw:/rearrightend_sw:) - resolves which physical end
            // to toggle from the currently active cab, unlike light() above (a fixed-end direct
            // override for debugging). p_light is the MMD label's own suffix, e.g. "upper",
            // "left", "leftend", "rearupper", "rearleftend".
            virtual void light_switch(const String &p_light, bool p_enabled) = 0;
    };
} // namespace godot
