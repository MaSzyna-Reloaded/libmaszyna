#pragma once
#include "RailVehicleElectricEngine.hpp"
#include "macros.hpp"
#include "vehicles/rail/RailVehicleRelayListItem.hpp"

namespace godot {
    class VehicleController;

    class RailVehicleElectricSeriesEngine : public RailVehicleElectricEngine {
            GDCLASS(RailVehicleElectricSeriesEngine, RailVehicleElectricEngine)
        public:
            void _fill_state_dictionary(Dictionary &p_state) const override;
            void _fill_config_dictionary(Dictionary &p_config) const override;

            /* Live state, read straight from the implementation - nothing is stored. */
            virtual double get_resistor_fan_rotation() const = 0;
            /* The current the automatic start steps on below (Imin: IminLo, or IminHi switched high) */
            virtual double get_circuit_imin() const = 0;
            /* Whether the automatic start steps on the high current (Imin == IminHi) */
            virtual bool get_circuit_imin_high_enabled() const = 0;
            /* The speed [km/h] above which the next position of the master controller (or, with
             * `p_main_controller` false, of the field shunt) keeps the motor current within its
             * limit and the wheels within their adhesion (TController::ESMVelocity(), Driver.cpp:2344) */
            virtual double get_next_position_velocity(bool p_main_controller) const = 0;

            /* RVent= (Automatic / Yes / No): resistor cooling fan drive mode */
            enum FanType {
                FAN_TYPE_NONE,
                FAN_TYPE_YES,
                FAN_TYPE_AUTOMATIC,
            };

            static void _bind_methods();

        private:
            bool direction_switches_circuit_imin_high = false;

        protected:
            EngineType get_type() const override;

        public:
            MAKE_MEMBER_GS(double, nominal_voltage, 0.0);
            MAKE_MEMBER_GS(double, winding_resistance, 0.0);
            MAKE_MEMBER_GS(double, max_rpm, 0.0);
            MAKE_MEMBER_GS_NR(FanType, resistor_fan_type, FAN_TYPE_NONE);
            MAKE_MEMBER_GS(double, resistor_fan_max_rpm, 1.0);
            MAKE_MEMBER_GS(double, resistor_fan_cutoff_resistance, 0.0);
            MAKE_MEMBER_GS(double, resistor_fan_min_current, 50.0);
            MAKE_MEMBER_GS(double, resistor_fan_speed, 0.5);
            MAKE_MEMBER_GS(double, dynamic_brake_resistance, 5.8);
            MAKE_MEMBER_GS(double, dynamic_brake_resistance_1, 5.8);
            MAKE_MEMBER_GS(double, dynamic_brake_resistance_2, 5.8);
            MAKE_MEMBER_GS_NR_NO_DEF(TypedArray<RailVehicleRelayListItem>, relay_list)

            /* The reverser's step past "forward" switches the automatic start to the high current, and
             * its step back from there to the low one (TMoverParameters::DirectionForward/Backward,
             * Mover.cpp:719, 3250) - the reverser key then has a position more */
            void set_direction_switches_circuit_imin_high(bool p_value);
            bool get_direction_switches_circuit_imin_high() const;
    };
} // namespace godot
VARIANT_ENUM_CAST(RailVehicleElectricSeriesEngine::FanType)
