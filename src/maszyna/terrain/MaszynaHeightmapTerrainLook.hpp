#pragma once

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/classes/shader.hpp>
#include <godot_cpp/classes/texture2d_array.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>

namespace godot {
    /// What a heightmap terrain is drawn with (opengl33_terrain_materials, opengl33terrain.cpp): its
    /// shader, the textures of every layer of its palette in three arrays - diffuse, normal maps,
    /// specular/gloss maps - and what each layer of the palette is, PALETTE_FLOATS a layer: its index
    /// in the arrays, 1 / metres its textures repeat at, whether it has a normal map and a
    /// specular/gloss map, its reflection, the cosine and the sine of the angle its textures are
    /// turned by, its glossiness.
    class MaszynaHeightmapTerrainLook : public RefCounted {
            GDCLASS(MaszynaHeightmapTerrainLook, RefCounted)

        public:
            static constexpr int PALETTE_FLOATS = 8;

        private:
            Ref<Shader> shader;
            Ref<Texture2DArray> diffuse_layers;
            Ref<Texture2DArray> normal_layers;
            Ref<Texture2DArray> specgloss_layers;
            PackedFloat32Array palette;

        protected:
            static void _bind_methods();

        public:
            void set_shader(const Ref<Shader> &p_shader);
            Ref<Shader> get_shader() const;
            void set_diffuse_layers(const Ref<Texture2DArray> &p_layers);
            Ref<Texture2DArray> get_diffuse_layers() const;
            void set_normal_layers(const Ref<Texture2DArray> &p_layers);
            Ref<Texture2DArray> get_normal_layers() const;
            void set_specgloss_layers(const Ref<Texture2DArray> &p_layers);
            Ref<Texture2DArray> get_specgloss_layers() const;
            void set_palette(const PackedFloat32Array &p_palette);
            PackedFloat32Array get_palette() const;
    };
} // namespace godot
