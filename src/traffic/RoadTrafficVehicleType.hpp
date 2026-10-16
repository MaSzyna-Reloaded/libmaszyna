#pragma once
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /* A kind of road vehicle RoadTrafficServer drives: the model it is drawn with, its size, how
     * fast it goes and how much of the traffic it makes. Whoever knows the vehicles (a game reading
     * its dynamic/ directory) makes one per vehicle and skin. */
    class RoadTrafficVehicleType : public Resource {
            GDCLASS(RoadTrafficVehicleType, Resource)

        private:
            /* Where the model and its textures are, as a `dynamic` names it */
            String data_path;
            String model_filename;
            /* By number of the model's replaceable materials (E3DRenderingServer.instance_set_options()) */
            PackedStringArray skins;
            /* Dim.L, Dim.W [m] */
            double length = 0.0;
            double width = 0.0;
            /* Vmax [km/h]; 0 for none - the road's limit then */
            double max_velocity = 0.0;
            /* Its share of the traffic, against the other types' */
            double weight = 1.0;

        protected:
            static void _bind_methods();

        public:
            void set_data_path(const String &p_value);
            String get_data_path() const;
            void set_model_filename(const String &p_value);
            String get_model_filename() const;
            void set_skins(const PackedStringArray &p_value);
            PackedStringArray get_skins() const;
            void set_length(double p_value);
            double get_length() const;
            void set_width(double p_value);
            double get_width() const;
            void set_max_velocity(double p_value);
            double get_max_velocity() const;
            void set_weight(double p_value);
            double get_weight() const;
    };
} // namespace godot
