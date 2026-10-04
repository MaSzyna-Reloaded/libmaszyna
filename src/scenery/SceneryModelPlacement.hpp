#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/packed_string_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/transform3d.hpp>

namespace godot {
    /// A model a SceneryStreamingProvider places (SceneryStreamingProvider.CONTENT_MODELS): drawn
    /// while its chunk is supplied, and named by nothing - no event or signal reaches it
    class SceneryModelPlacement : public Resource {
            GDCLASS(SceneryModelPlacement, Resource)

        private:
            String data_path;
            String model_filename;
            PackedStringArray skins;
            Transform3D transform;
            float range_min = 0.0F;
            float range_max = 0.0F;

        protected:
            static void _bind_methods();

        public:
            /// Where the model and its textures are, and the model file without its extension
            void set_data_path(const String &p_value);
            String get_data_path() const;
            void set_model_filename(const String &p_value);
            String get_model_filename() const;
            /// The model's replaceable materials, by number
            void set_skins(const PackedStringArray &p_value);
            PackedStringArray get_skins() const;
            /// Where the model stands in the world
            void set_transform(const Transform3D &p_value);
            Transform3D get_transform() const;
            /// The distances it is drawn between [m]; a range_max of 0 or less is no limit
            void set_range_min(float p_value);
            float get_range_min() const;
            void set_range_max(float p_value);
            float get_range_max() const;
    };
} // namespace godot
