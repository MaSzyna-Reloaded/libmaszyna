#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class RailVehicleSecuritySystem : public RailVehicleComponent {
            GDCLASS(RailVehicleSecuritySystem, RailVehicleComponent)

        public:
            /* The alerter's lamp blinking, its buzzer sounding (state: bool) */
            static const char *blinking_changed_signal;
            static const char *beeping_changed_signal;

            int get_component_type() const override {
                return RailVehicleComponentType::COMPONENT_SECURITY;
            }

        private:
            static void _bind_methods();

        protected:
            void _register_commands() override;
            void _unregister_commands() override;

        public:
            /* Live state, read straight from the backend - nothing is stored. */
            virtual bool get_beeping() const = 0;
            virtual bool get_blinking() const = 0;
            virtual bool get_radiostop_available() const = 0;
            virtual bool get_vigilance_blinking() const = 0;
            virtual bool get_cabsignal_blinking() const = 0;
            virtual bool get_cabsignal_beeping() const = 0;
            virtual bool get_braking() const = 0;
            virtual bool get_engine_blocked() const = 0;
            virtual bool get_separate_acknowledge() const = 0;
            enum EmergencySignal {
                EMERGENCY_SIGNAL_SIREN_LOW_TONE,
                EMERGENCY_SIGNAL_SIREN_HIGH_TONE,
                EMERGENCY_SIGNAL_WHISTLE
            };
            virtual void security_acknowledge(bool p_enabled) = 0;
            virtual void security_cabsignal_acknowledge() = 0;
            /* The track's cab signal magnet reached the vehicle (SHP/Indusi; the scenery's
             * `CabSignal` command, Mover.cpp:12649-12654) */
            virtual void security_cabsignal_trigger() = 0;
            /* The Radio-Stop emergency braking switched on or off from outside the cab (the
             * scenery's `Emergency_brake` command, Mover.cpp:12624-12630) */
            virtual void security_radiostop(bool p_enabled) = 0;

        private:
            bool aware_system_active = false;
            bool aware_system_cabsignal = false;
            bool aware_system_separate_acknowledge = false;
            bool aware_system_sifa = false;

        public:
            MAKE_GS(bool, aware_system_active);
            MAKE_GS(bool, aware_system_cabsignal);
            MAKE_GS(bool, aware_system_separate_acknowledge);
            MAKE_GS(bool, aware_system_sifa);
            /* AwareMinSpeed= absent: 10% of the vehicle's Vmax (TSecuritySystem::load, Mover.cpp:247) */
            static constexpr double AWARE_MIN_SPEED_FROM_MAX_VELOCITY = -1.0;
            static constexpr double AWARE_MIN_SPEED_MAX_VELOCITY_SHARE = 0.1;

        private:
            /* TSecuritySystem::load's defaults (MOVER.h:1147-1152, 1173) */
            double aware_delay = 30.0;
            double aware_min_speed = AWARE_MIN_SPEED_FROM_MAX_VELOCITY;
            double emergency_brake_delay = 5.0;
            bool radio_stop_enabled = false;
            double sound_signal_delay = 5.0;
            double shp_magnet_distance = 0.0;
            double ca_max_hold_time = 1.5;
            bool cab_dependent = false;
            EmergencySignal emergency_signal = EmergencySignal::EMERGENCY_SIGNAL_SIREN_HIGH_TONE;

        public:
            MAKE_GS(double, aware_delay);
            MAKE_GS(double, aware_min_speed);
            MAKE_GS(double, emergency_brake_delay);
            MAKE_GS(bool, radio_stop_enabled);
            MAKE_GS(double, sound_signal_delay);
            MAKE_GS(double, shp_magnet_distance);
            MAKE_GS(double, ca_max_hold_time);
            MAKE_GS(bool, cab_dependent);
            MAKE_GS_NR(EmergencySignal, emergency_signal);
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleSecuritySystem::EmergencySignal)
