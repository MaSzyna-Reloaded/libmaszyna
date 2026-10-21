#pragma once
#include "vehicles/rail/RailVehicleAppearance.hpp"

#include <godot_cpp/variant/packed_float64_array.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>

namespace godot {
    /* What a MaSzyna vehicle looks like beyond its models and running gear: the submodels of its
     * exterior model that MaszynaE3DRailVehicleRenderingServer animates - pantograph arms, wipers,
     * mirrors, doors, steps, pendulums - and how its low-poly interior is lit, as its MMD names
     * them (MaszynaRailVehicle3DInstancer). */
    class MaszynaRailVehicleAppearance : public RailVehicleAppearance {
            GDCLASS(MaszynaRailVehicleAppearance, RailVehicleAppearance)

        public:
            /* How bright the low-poly interior glows with the roof light on [emission energy], and
             * how fast it follows it [s] - the wrapper's own */
            static constexpr double DEFAULT_LOW_POLY_EMISSION_ENERGY = 0.2;
            static constexpr double DEFAULT_LOW_POLY_EMISSION_FADE_TIME = 0.2;

        private:
            PackedStringArray pantograph_front_arms;
            PackedStringArray pantograph_rear_arms;
            PackedStringArray wiper_arms;
            PackedStringArray mirrors;
            PackedStringArray doors;
            PackedStringArray door_steps;
            PackedStringArray pendulums;
            PackedFloat64Array pantograph_factors;
            double pendulum_amplitude = 0.0;
            double low_poly_emission_energy = DEFAULT_LOW_POLY_EMISSION_ENERGY;
            double low_poly_emission_fade_time = DEFAULT_LOW_POLY_EMISSION_FADE_TIME;
            bool joint_cabs = false;

        protected:
            static void _bind_methods();

        public:
            /* Lower arm, its pair, upper arm, its pair, slider (TAnimPant, DynObj.cpp:5414) */
            void set_pantograph_front_arms(const PackedStringArray &p_value);
            PackedStringArray get_pantograph_front_arms() const;
            void set_pantograph_rear_arms(const PackedStringArray &p_value);
            PackedStringArray get_pantograph_rear_arms() const;
            /* Arm 1, arm 2 and blade of every wiper (DynObj.cpp:5838-5870) */
            void set_wiper_arms(const PackedStringArray &p_value);
            PackedStringArray get_wiper_arms() const;
            /* In the original's order, odd on the left, even on the right (DynObj.cpp:5887-5910) */
            void set_mirrors(const PackedStringArray &p_value);
            PackedStringArray get_mirrors() const;
            /* Every door of animdoorprefix:, numbered from 1 - odd on the left, even on the right - each
             * with its first submodel below and that one's, which a folding door turns as well; a
             * name the model has not got is empty (DynObj.cpp:592-622, 5721-5760) */
            void set_doors(const PackedStringArray &p_value);
            PackedStringArray get_doors() const;
            /* Every door step of animstepprefix:, numbered as the doors (DynObj.cpp:5763-5790) */
            void set_door_steps(const PackedStringArray &p_value);
            PackedStringArray get_door_steps() const;
            /* pantfactors: - the first and the second pantograph's position along the vehicle and
             * slider height, for one the model cannot be measured by (DynObj.cpp:5577-5633); empty
             * without the key */
            void set_pantograph_factors(const PackedFloat64Array &p_value);
            PackedFloat64Array get_pantograph_factors() const;
            /* animpendulumprefix: 1 to 4, swinging about their x by pendulumamplitude: [deg] times
             * the cosine of the engine's turn (DynObj.cpp:1121-1125, 5702-5719) */
            void set_pendulums(const PackedStringArray &p_value);
            PackedStringArray get_pendulums() const;
            void set_pendulum_amplitude(double p_value);
            double get_pendulum_amplitude() const;
            void set_low_poly_emission_energy(double p_value);
            double get_low_poly_emission_energy() const;
            void set_low_poly_emission_fade_time(double p_value);
            double get_low_poly_emission_fade_time() const;
            /* One low-poly cab for both ends (jointcabs:, DynObj.cpp:2236-2250) */
            void set_joint_cabs(bool p_value);
            bool get_joint_cabs() const;
    };
} // namespace godot
