#include "legacy/e3d/e3d_parser.hpp"
#include <godot_cpp/variant/utility_functions.hpp>

#include <array>

namespace godot {

    void E3DParser::_bind_methods() {
        ClassDB::bind_method(D_METHOD("parse", "file"), &E3DParser::parse);
    }

    E3DParser::ChunkHeader E3DParser::_read_chunk_header(const Ref<FileAccess> &p_file) const {
        if (!p_file.is_valid()) {
            return {};
        }
        const String chunk_id = p_file->get_buffer(4).get_string_from_ascii();
        const uint32_t chunk_len = p_file->get_32();
        return {chunk_id, chunk_len, chunk_len - 8};
    }

    int E3DParser::u32s(const uint32_t p_value) const {
        return static_cast<int>(
                (static_cast<int64_t>(p_value) + max_31_b) % max_32_b - max_31_b); // NOLINT(*-math-missing-parentheses)
    }


    E3DParser::SubModelData E3DParser::_read_submodel(const Ref<FileAccess> &p_file, const int p_chunk_size) const {
        SubModelData result;
        result.next_idx = u32s(p_file->get_32());         // offset=0
        result.first_child_idx = u32s(p_file->get_32());  // offset=4
        result.type = p_file->get_32();                   // offset=8
        result.name_idx = u32s(p_file->get_32());         // offset=12
        result.anim = u32s(p_file->get_32());             // offset=16
        result.flags = p_file->get_32() & 0xFFFF;         // offset=20
        result.matrix_idx = u32s(p_file->get_32());       // offset=24
        result.vertex_count = u32s(p_file->get_32());     // offset=28
        result.first_vertex_idx = u32s(p_file->get_32()); // offset=32
        result.material_idx = u32s(p_file->get_32());     // offset=36
        result.is_material_colored = (result.material_idx == 0);
        // ReSharper disable once CppExpressionWithoutSideEffects
        p_file->get_float();                              // offset 40 UNUSED
        result.lights_on_threshold = p_file->get_float(); // offset 44
        // result.visibility_light_threshold = p_file->get_float();
        // ReSharper disable once CppExpressionWithoutSideEffects
        p_file->get_buffer(16); // skip unused RGBA ambient
        const float diffuse_r = p_file->get_float();
        const float diffuse_g = p_file->get_float();
        const float diffuse_b = p_file->get_float();
        const float diffuse_a = p_file->get_float();
        Color diffuse_color = Color(diffuse_r, diffuse_g, diffuse_b, diffuse_a);
        // ReSharper disable once CppExpressionWithoutSideEffects
        p_file->get_buffer(16); // skip unused RGBA specular
        const float selfillum_r = p_file->get_float();
        const float selfillum_g = p_file->get_float();
        const float selfillum_b = p_file->get_float();
        const float selfillum_a = p_file->get_float();
        Color selfillum_color = Color(selfillum_r, selfillum_g, selfillum_b, selfillum_a);
        if (const auto transparent = result.flags & SUBMODEL_FLAG_TRANSLUCENT; transparent == 0u) {
            diffuse_color.a = 1.0;
        }

        result.selfillum_color = selfillum_color;
        result.diffuse_color = diffuse_color;
        result.gl_lines_size = p_file->get_float(); // offset=112

        result.lod_max_distance = p_file->get_float();                     // offset=116
        result.lod_min_distance = p_file->get_float();                     // offset=120
        result.near_attenuation_start = p_file->get_float();               // offset=124
        result.near_attenuation_end = p_file->get_float();                 // offset=128
        result.use_near_attenuation = p_file->get_32() != 0;               // offset=132
        result.far_attenuation_decay = static_cast<int>(p_file->get_32()); // offset=136
        result.light_range = p_file->get_float();                          // fFarDecayRadius  // offset=140
        const float cos_falloff = p_file->get_float();                     // fCosFalloffAngle // offset=144
        result.cos_hotspot_angle = p_file->get_float();                    // offset=148
        result.cos_view_angle = p_file->get_float();                       // offset=152

        result.light_angle = Math::rad_to_deg(Math::acos(Math::clamp(cos_falloff, -1.0f, 1.0f)));
        // spot_attenuation in Godot controls BOTH distance and angular attenuation (softness).
        // Since E3D has iFarAttenDecay for distance and a hotspot/falloff for angle,
        // we'll leave it at 1.0 here and let the instancer decide based on all params.
        result.light_attenuation = 1.0f;
        result.diffuse_color = diffuse_color;

        result.index_count = p_file->get_32();     // offset=156
        result.first_index_idx = p_file->get_32(); // Offset 160
        result.light_energy = p_file->get_float(); // Offset 164

        p_file->get_buffer(p_chunk_size - SUBMODEL_READ_SIZE); // dev/unused data


        result.vertices = PackedVector3Array();
        result.normals = PackedVector3Array();
        result.uvs = PackedVector2Array();
        result.indices = PackedInt32Array();
        return result;
    }

    std::vector<E3DParser::SubModelData> E3DParser::_parse_file(const Ref<FileAccess> &p_file) const {
        ChunkHeader chunk_header = _read_chunk_header(p_file);
        std::vector<SubModelData> submodels;
        if (chunk_header.id != "E3D0") {
            UtilityFunctions::push_error("Incorrect header of E3D file: " + p_file->get_path());
            return submodels;
        }
        std::vector<String> submodel_names;
        std::vector<String> material_names;
        std::vector<Transform3D> matrices;
        while (!p_file->eof_reached()) {
            const ChunkHeader chunk = _read_chunk_header(p_file);
            if (chunk.id == "SUB0") {
                const int submodels_count = static_cast<int>(chunk.data_len) / SUB0_SUBMODEL_SIZE;
                for (int i = 0; i < submodels_count; i++) {
                    submodels.emplace_back(_read_submodel(p_file, SUB0_SUBMODEL_SIZE));
                }
            } else if (chunk.id == "SUB1") {
                const int submodels_count = static_cast<int>(chunk.data_len) / SUB1_SUBMODEL_SIZE;
                for (int i = 0; i < submodels_count; i++) {
                    submodels.emplace_back(_read_submodel(p_file, SUB1_SUBMODEL_SIZE));
                }
            } else if (chunk.id == "NAM0") {
                submodel_names = _buffer_to_strings(p_file->get_buffer(chunk.data_len));
            } else if (chunk.id == "TEX0") {
                material_names = _buffer_to_strings(p_file->get_buffer(chunk.data_len));
            } else if (chunk.id == "TRA0") {
                const int matrix_count = static_cast<int>(chunk.data_len) / TRA0_MATRIX_SIZE;
                for (int i = 0; i < matrix_count; i++) {
                    std::array<float, 16> m{};
                    for (float &row: m) {
                        row = p_file->get_float();
                    }
                    matrices.emplace_back(E3DModelBuilder::make_matrix(m));
                }
            } else if (chunk.id == "IDX1") {
                const uint64_t pos = p_file->get_position();
                for (SubModelData &submodel: submodels) {
                    if (submodel.index_count <= 0) {
                        continue;
                    }
                    p_file->seek(pos + submodel.first_index_idx);
                    PackedInt32Array indices;
                    for (int j = 0; j < submodel.index_count; j++) {
                        indices.append(p_file->get_8());
                    }

                    submodel.indices.append_array(indices);
                }

                p_file->seek(pos + chunk.data_len);
            } else if (chunk.id == "IDX2") {
                const uint64_t pos = p_file->get_position();
                for (SubModelData &submodel: submodels) {
                    p_file->seek(pos + (static_cast<uint64_t>(submodel.first_index_idx) * 2));
                    PackedInt32Array indices;
                    for (int j = 0; j < submodel.index_count; j++) {
                        indices.append(p_file->get_16());
                    }

                    submodel.indices.append_array(indices);
                }

                p_file->seek(pos + chunk.data_len);
            } else if (chunk.id == "IDX4") {
                const uint64_t pos = p_file->get_position();
                for (SubModelData &submodel: submodels) {
                    p_file->seek(pos + (static_cast<uint64_t>(submodel.first_index_idx) * 4));
                    PackedInt32Array indices;
                    for (int j = 0; j < submodel.index_count; j++) {
                        indices.append(p_file->get_32());
                    }

                    submodel.indices.append_array(indices);
                }

                p_file->seek(pos + chunk.data_len);
            } else if (chunk.id == "VNT0" || chunk.id == "VNT2") {
                // VNT0 is the legacy layout without tangents, VNT2 carries them (Model3d.cpp:2011)
                const bool has_tangents = chunk.id == "VNT2";
                const uint64_t vertex_size = has_tangents ? VNT2_VERTEX_SIZE : VNT0_VERTEX_SIZE;
                const uint64_t pos = p_file->get_position();
                for (SubModelData &submodel: submodels) {
                    p_file->seek(pos + (static_cast<uint64_t>(submodel.first_vertex_idx) * vertex_size));

                    PackedVector3Array vertices;
                    PackedVector3Array normals;
                    PackedVector2Array uvs;
                    PackedFloat64Array tangents;

                    for (int j = 0; j < submodel.vertex_count; ++j) {
                        const float x = p_file->get_float();
                        const float y = p_file->get_float();
                        const float z = p_file->get_float();
                        const float nx = p_file->get_float();
                        const float ny = p_file->get_float();
                        const float nz = p_file->get_float();
                        const float u = p_file->get_float();
                        const float v = p_file->get_float();

                        vertices.append(Vector3(x, y, z));
                        normals.append(Vector3(nx, ny, nz));
                        uvs.append(Vector2(u, v));
                        if (has_tangents) {
                            const float tx = p_file->get_float();
                            const float ty = p_file->get_float();
                            const float tz = p_file->get_float();
                            const float tw = p_file->get_float();
                            tangents.push_back(tx);
                            tangents.push_back(ty);
                            tangents.push_back(tz);
                            tangents.push_back(tw);
                        }
                    }
                    submodel.vertices = vertices;
                    submodel.normals = normals;
                    submodel.uvs = uvs;
                    submodel.tangents = tangents;
                }
                p_file->seek(pos + chunk.data_len);
            } else {
                if (!chunk.id.is_empty()) {
                    UtilityFunctions::push_warning("Skipping unsupported chunk: " + chunk.id);
                }

                if (chunk.data_len > 0) {
                    // ReSharper disable once CppExpressionWithoutSideEffects
                    p_file->get_buffer(chunk.data_len);
                }
            }
        }

        for (SubModelData &submodel: submodels) {
            // might be better to use operator[]

            if (submodel.name_idx >= 0 && submodel.name_idx < submodel_names.size()) {
                submodel.name = String(submodel_names[submodel.name_idx]);
            }

            if (submodel.material_idx >= 0 && submodel.material_idx < material_names.size()) {
                submodel.material = String(material_names[submodel.material_idx]);
                // Original engine: "colored" is the built-in diffuse-color material, same as texture
                // index 0 (Model3d.cpp:369, 2084 - Fetch_Material("colored")), not a texture.
                if (submodel.material.to_lower() == "colored") {
                    submodel.is_material_colored = true;
                }
            }

            if (submodel.matrix_idx >= 0 && submodel.matrix_idx < matrices.size()) {
                submodel.matrix = Transform3D(matrices[submodel.matrix_idx]);
            }
        }

        return submodels;
    }

    std::vector<String> E3DParser::_buffer_to_strings(const PackedByteArray &p_buffer) const {
        std::vector<String> output;
        String tmp;

        for (const unsigned char i: p_buffer) {
            if (i == 0) {
                output.emplace_back(tmp);
                tmp = String();
            } else {
                tmp += String::chr(i);
            }
        }

        return output;
    }

    Ref<E3DModel> E3DParser::parse(const Ref<FileAccess> &p_file) const {
        std::vector<SubModelData> submodels = _parse_file(p_file);
        return E3DModelBuilder::build(submodels);
    }
} // namespace godot
