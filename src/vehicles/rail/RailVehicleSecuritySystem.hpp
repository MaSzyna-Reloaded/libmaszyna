#pragma once
#include "macros.hpp"
#include "vehicles/rail/RailVehicleComponent.hpp"
#include <godot_cpp/classes/node.hpp>

namespace godot {
    class RailVehicleSecuritySystem : public RailVehicleComponent {
            GDCLASS(RailVehicleSecuritySystem, RailVehicleComponent)

        public:
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
            MAKE_MEMBER_GS(bool, aware_system_active, false);
            MAKE_MEMBER_GS(bool, aware_system_cabsignal, false);
            MAKE_MEMBER_GS(bool, aware_system_separate_acknowledge, false);
            MAKE_MEMBER_GS(bool, aware_system_sifa, false);
            /* AwareMinSpeed= absent: 10% of the vehicle's Vmax (TSecuritySystem::load, Mover.cpp:247) */
            static constexpr double AWARE_MIN_SPEED_FROM_MAX_VELOCITY = -1.0;
            static constexpr double AWARE_MIN_SPEED_MAX_VELOCITY_SHARE = 0.1;
            /* TSecuritySystem::load's defaults (MOVER.h:1147-1152, 1173) */
            MAKE_MEMBER_GS(double, aware_delay, 30.0);
            MAKE_MEMBER_GS(double, aware_min_speed, AWARE_MIN_SPEED_FROM_MAX_VELOCITY);
            MAKE_MEMBER_GS(double, emergency_brake_delay, 5.0);
            MAKE_MEMBER_GS(bool, radio_stop_enabled, false);
            MAKE_MEMBER_GS(double, sound_signal_delay, 5.0);
            MAKE_MEMBER_GS(double, shp_magnet_distance, 0.0);
            MAKE_MEMBER_GS(double, ca_max_hold_time, 1.5);
            MAKE_MEMBER_GS(bool, cab_dependent, false);
            MAKE_MEMBER_GS_NR(EmergencySignal, emergency_signal, EmergencySignal::EMERGENCY_SIGNAL_SIREN_HIGH_TONE);
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleSecuritySystem::EmergencySignal)
