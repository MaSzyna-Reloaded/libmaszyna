#pragma once
#include "legacy/e3d/E3DModel.hpp"
#include "legacy/e3d/E3DSubModel.hpp"

#include <array>
#include <string_view>
#include <vector>

namespace godot {
    /// Builds an E3DModel from submodels read by a model parser (the binary E3D or the text T3D
    /// one), all given in the E3D's own terms: its type and animation codes, flags, squared
    /// distances and the scenery-aligned frame (the original's post-Init() state, Model3d.cpp:2470)
    class E3DModelBuilder {
        public:
            struct SubModelData {
                    int next_idx;
                    int first_child_idx;
                    uint32_t type;
                    int name_idx;
                    String name;
                    int anim;
                    uint32_t flags;
                    int matrix_idx;
                    String material;
                    int vertex_count;
                    int first_vertex_idx;
                    int material_idx;
                    bool is_material_colored;
                    float lights_on_threshold;
                    float visibility_light_threshold;
                    Color diffuse_color;
                    Color selfillum_color;
                    float gl_lines_size;
                    float lod_max_distance;
                    float lod_min_distance;
                    float light_range;
                    float light_attenuation;
                    float light_angle;
                    float near_attenuation_start;
                    float near_attenuation_end;
                    bool use_near_attenuation;
                    int far_attenuation_decay;
                    float cos_hotspot_angle;
                    float cos_view_angle;
                    uint32_t index_count;
                    uint32_t first_index_idx;
                    PackedVector3Array vertices;
                    PackedVector3Array normals;
                    PackedVector2Array uvs;
                    PackedInt32Array indices;
                    PackedFloat64Array tangents;
                    Transform3D matrix;
                    float light_energy;
            };

            /// A transform from the 16 floats of a float4x4, in the order E3D and T3D store them
            static Transform3D make_matrix(const std::array<float, 16> &p_m);
            /// Links the submodels by `first_child_idx`/`next_idx` and registers lights and smoke
            /// sources; the meshes are made from the data, so it is consumed
            static Ref<E3DModel> build(std::vector<SubModelData> &p_submodels);

        private:
            static constexpr std::array<std::string_view, 19> NON_LIGHTS_TO_EXCLUDE = {
                    "coupler1",      "coupler2",    "cctrl1",       "cctrl2",      "cpass1",
                    "cpass2",        "cpneumatic1", "cpneumatic1r", "cpneumatic2", "cpneumatic2r",
                    "external_only", "mechanik1",   "mechanik2",    "pneumatic1",  "pneumatic1r",
                    "pneumatic2",    "pneumatic2r", "shutters1",    "shutters2",
            };

            struct Indices {
                    uint32_t i1;
                    uint32_t i2;
                    uint32_t i3;
            };

            struct Vertices {
                    Vector3 v1;
                    Vector3 v2;
                    Vector3 v3;
            };

            struct Edges {
                    Vector3 e1;
                    Vector3 e2;
            };

            static PackedVector3Array
            _calculate_normals(const PackedVector3Array &p_vertices, const PackedInt32Array &p_indices);
            static Ref<E3DSubModel> _create_submodel(SubModelData &p_submodel);
            static void _register_lights(
                    const Ref<E3DModel> &p_model, const std::vector<Ref<E3DSubModel>> &p_submodels,
                    const std::vector<int> &p_parent_indices);
            static NodePath _build_submodel_path(
                    const std::vector<Ref<E3DSubModel>> &p_submodels, const std::vector<int> &p_parent_indices,
                    int p_index);
    };
} // namespace godot
