#include "maszyna/terrain/MaszynaHeightmapTerrainLook.hpp"

namespace godot {
    void MaszynaHeightmapTerrainLook::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_shader", "shader"), &MaszynaHeightmapTerrainLook::set_shader);
        ClassDB::bind_method(D_METHOD("get_shader"), &MaszynaHeightmapTerrainLook::get_shader);
        ClassDB::bind_method(
                D_METHOD("set_diffuse_layers", "layers"), &MaszynaHeightmapTerrainLook::set_diffuse_layers);
        ClassDB::bind_method(D_METHOD("get_diffuse_layers"), &MaszynaHeightmapTerrainLook::get_diffuse_layers);
        ClassDB::bind_method(D_METHOD("set_normal_layers", "layers"), &MaszynaHeightmapTerrainLook::set_normal_layers);
        ClassDB::bind_method(D_METHOD("get_normal_layers"), &MaszynaHeightmapTerrainLook::get_normal_layers);
        ClassDB::bind_method(
                D_METHOD("set_specgloss_layers", "layers"), &MaszynaHeightmapTerrainLook::set_specgloss_layers);
        ClassDB::bind_method(D_METHOD("get_specgloss_layers"), &MaszynaHeightmapTerrainLook::get_specgloss_layers);
        ClassDB::bind_method(D_METHOD("set_palette", "palette"), &MaszynaHeightmapTerrainLook::set_palette);
        ClassDB::bind_method(D_METHOD("get_palette"), &MaszynaHeightmapTerrainLook::get_palette);
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "shader", PROPERTY_HINT_RESOURCE_TYPE, "Shader"), "set_shader",
                "get_shader");
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "diffuse_layers", PROPERTY_HINT_RESOURCE_TYPE, "Texture2DArray"),
                "set_diffuse_layers", "get_diffuse_layers");
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "normal_layers", PROPERTY_HINT_RESOURCE_TYPE, "Texture2DArray"),
                "set_normal_layers", "get_normal_layers");
        ADD_PROPERTY(
                PropertyInfo(Variant::OBJECT, "specgloss_layers", PROPERTY_HINT_RESOURCE_TYPE, "Texture2DArray"),
                "set_specgloss_layers", "get_specgloss_layers");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "palette"), "set_palette", "get_palette");
        BIND_CONSTANT(PALETTE_FLOATS);
    }

    void MaszynaHeightmapTerrainLook::set_shader(const Ref<Shader> &p_shader) {
        shader = p_shader;
    }

    Ref<Shader> MaszynaHeightmapTerrainLook::get_shader() const {
        return shader;
    }

    void MaszynaHeightmapTerrainLook::set_diffuse_layers(const Ref<Texture2DArray> &p_layers) {
        diffuse_layers = p_layers;
    }

    Ref<Texture2DArray> MaszynaHeightmapTerrainLook::get_diffuse_layers() const {
        return diffuse_layers;
    }

    void MaszynaHeightmapTerrainLook::set_normal_layers(const Ref<Texture2DArray> &p_layers) {
        normal_layers = p_layers;
    }

    Ref<Texture2DArray> MaszynaHeightmapTerrainLook::get_normal_layers() const {
        return normal_layers;
    }

    void MaszynaHeightmapTerrainLook::set_specgloss_layers(const Ref<Texture2DArray> &p_layers) {
        specgloss_layers = p_layers;
    }

    Ref<Texture2DArray> MaszynaHeightmapTerrainLook::get_specgloss_layers() const {
        return specgloss_layers;
    }

    void MaszynaHeightmapTerrainLook::set_palette(const PackedFloat32Array &p_palette) {
        palette = p_palette;
    }

    PackedFloat32Array MaszynaHeightmapTerrainLook::get_palette() const {
        return palette;
    }
} // namespace godot
