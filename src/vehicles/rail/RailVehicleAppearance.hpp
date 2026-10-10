#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /* What a kind of rail vehicle looks like: the models it is drawn with and the submodels of its
     * exterior model that move - bogies, wheels, pantograph arms, wipers, mirrors. Shared by every
     * vehicle of the kind and handed to RailVehicleRenderingServer, which builds and animates the
     * vehicle from it. Submodels are named as the model names them; a missing one is an empty
     * name. */
    class RailVehicleAppearance : public Resource {
            GDCLASS(RailVehicleAppearance, Resource)

        public:
            /* The submodel a head display material is drawn on, for a vehicle put together by hand -
             * the wrapper's own name; a vehicle of the game data takes the submodel with replaceable
             * skin 4, as the original (MaszynaRailVehicle3DInstancer, DynObj.cpp:2539-2545) */
            static constexpr const char *DEFAULT_HEAD_DISPLAY_SUBMODEL = "tablice_relacyjne";
            /* How bright the low-poly interior glows with the roof light on [emission energy], and
             * how fast it follows it [s] - the wrapper's own */
            static constexpr double DEFAULT_LOW_POLY_EMISSION_ENERGY = 0.2;
            static constexpr double DEFAULT_LOW_POLY_EMISSION_FADE_TIME = 0.2;

        private:
            String data_path;
            String model_filename;
            String low_poly_model_filename;
            String passengers_model_filename;
            PackedStringArray attachment_model_filenames;
            PackedStringArray skins;
            Transform3D model_transform;
            String front_bogie;
            String rear_bogie;
            PackedStringArray front_rolling_wheels;
            PackedStringArray powered_wheels;
            PackedStringArray rear_rolling_wheels;
            PackedStringArray pantograph_front_arms;
            PackedStringArray pantograph_rear_arms;
            PackedStringArray wiper_arms;
            PackedStringArray mirrors;
            PackedStringArray doors;
            PackedStringArray door_steps;
            PackedStringArray pendulums;
            PackedFloat64Array pantograph_factors;
            double pendulum_amplitude = 0.0;
            String head_display_submodel = DEFAULT_HEAD_DISPLAY_SUBMODEL;
            double low_poly_emission_energy = DEFAULT_LOW_POLY_EMISSION_ENERGY;
            double low_poly_emission_fade_time = DEFAULT_LOW_POLY_EMISSION_FADE_TIME;
            bool joint_cabs = false;

        protected:
            static void _bind_methods();

        public:
            /* Where the model files are, and the exterior, the low-poly interior seen through the
             * windows and the passengers; skins by number of the model's replaceable materials */
            void set_data_path(const String &p_value);
            String get_data_path() const;
            void set_model_filename(const String &p_value);
            String get_model_filename() const;
            void set_low_poly_model_filename(const String &p_value);
            String get_low_poly_model_filename() const;
            void set_passengers_model_filename(const String &p_value);
            String get_passengers_model_filename() const;
            /* Models drawn with the exterior, in its frame and with its skins (attachments:,
             * DynObj.cpp:5384) */
            void set_attachment_model_filenames(const PackedStringArray &p_value);
            PackedStringArray get_attachment_model_filenames() const;
            void set_skins(const PackedStringArray &p_value);
            PackedStringArray get_skins() const;
            /* Where every model of the vehicle sits in the vehicle's own frame */
            void set_model_transform(const Transform3D &p_value);
            Transform3D get_model_transform() const;
            void set_front_bogie(const String &p_value);
            String get_front_bogie() const;
            void set_rear_bogie(const String &p_value);
            String get_rear_bogie() const;
            void set_front_rolling_wheels(const PackedStringArray &p_value);
            PackedStringArray get_front_rolling_wheels() const;
            void set_powered_wheels(const PackedStringArray &p_value);
            PackedStringArray get_powered_wheels() const;
            void set_rear_rolling_wheels(const PackedStringArray &p_value);
            PackedStringArray get_rear_rolling_wheels() const;
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
            /* animpendulumprefix: 1 to 4, swinging about their x by pendulumamplitude: [deg] times
             * the cosine of the engine's turn (DynObj.cpp:1121-1125, 5702-5719) */
            /* pantfactors: - the first and the second pantograph's position along the vehicle and
             * slider height, for one the model cannot be measured by (DynObj.cpp:5577-5633); empty
             * without the key */
            void set_pantograph_factors(const PackedFloat64Array &p_value);
            PackedFloat64Array get_pantograph_factors() const;
            void set_pendulums(const PackedStringArray &p_value);
            PackedStringArray get_pendulums() const;
            void set_pendulum_amplitude(double p_value);
            double get_pendulum_amplitude() const;
            void set_head_display_submodel(const String &p_value);
            String get_head_display_submodel() const;
            void set_low_poly_emission_energy(double p_value);
            double get_low_poly_emission_energy() const;
            void set_low_poly_emission_fade_time(double p_value);
            double get_low_poly_emission_fade_time() const;
            /* One low-poly cab for both ends (jointcabs:, DynObj.cpp:2236-2250) */
            void set_joint_cabs(bool p_value);
            bool get_joint_cabs() const;
    };
} // namespace godot
