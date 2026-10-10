#include "legacy/e3d/E3DModelBuilder.hpp"
#include "legacy/e3d/E3DModelLightDefinition.hpp"
#include "legacy/e3d/E3DModelSmokeSourceDefinition.hpp"
#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <cmath>
#include <unordered_map>
#include <unordered_set>

namespace godot {
    static constexpr const char *LIGHT_ON_SUFFIX = "_on";
    static constexpr const char *LIGHT_ON_ALT_SUFFIX = "_xon";
    static constexpr const char *LIGHT_OFF_SUFFIX = "_off";
    static constexpr const char *LIGHT_ON_PREFIX = "light_on";
    static constexpr const char *LIGHT_OFF_PREFIX = "light_off";
    static constexpr const char *SMOKE_SOURCE_PREFIX = "smokesource_";

    Transform3D E3DModelBuilder::make_matrix(const std::array<float, 16> &p_m) {
        return {Basis(Vector3(p_m[0], p_m[1], p_m[2]), Vector3(p_m[4], p_m[5], p_m[6]),
                      Vector3(p_m[8], p_m[9], p_m[10])),
                Vector3(p_m[12], p_m[13], p_m[14])};
    }

    PackedVector3Array
    E3DModelBuilder::_calculate_normals(const PackedVector3Array &p_vertices, const PackedInt32Array &p_indices) {
        PackedVector3Array normals;
        for (int i = 0; i < p_vertices.size(); i++) {
            normals.append(Vector3(0, 0, 0));
        }

        for (int i = 0; i < p_indices.size(); i += 3) {
            Indices indices;
            Vertices vertices;
            Edges edges;
            indices.i1 = p_indices.get(i);
            indices.i2 = p_indices.get(i + 1);
            indices.i3 = p_indices.get(i + 2);

            vertices.v1 = p_vertices.get(indices.i1);
            vertices.v2 = p_vertices.get(indices.i2);
            vertices.v3 = p_vertices.get(indices.i3);

            edges.e1 = vertices.v2 - vertices.v1;
            edges.e2 = vertices.v3 - vertices.v1;

            const Vector3 normal = edges.e1.cross(edges.e2).normalized();

            normals.set(indices.i1, normals.get(indices.i1) + normal);
            normals.set(indices.i2, normals.get(indices.i2) + normal);
            normals.set(indices.i3, normals.get(indices.i3) + normal);
        }

        for (int i = 0; i < normals.size(); i++) {
            normals.set(i, normals.get(i).normalized());
        }

        return normals;
    }

    Ref<E3DSubModel> E3DModelBuilder::_create_submodel(SubModelData &p_submodel) {
        Ref<E3DSubModel> submodel;
        submodel.instantiate();

        const std::unordered_map<int, E3DSubModel::SubModelType> type_map = {
                {0, E3DSubModel::SubModelType::SUBMODEL_GL_POINTS},
                {1, E3DSubModel::SubModelType::SUBMODEL_GL_LINES},
                {2, E3DSubModel::SubModelType::SUBMODEL_GL_LINE_STRIP},
                {3, E3DSubModel::SubModelType::SUBMODEL_GL_LINE_LOOP},
                {4, E3DSubModel::SubModelType::SUBMODEL_GL_TRIANGLES},
                {5, E3DSubModel::SubModelType::SUBMODEL_GL_TRIANGLE_STRIP},
                {6, E3DSubModel::SubModelType::SUBMODEL_GL_TRIANGLE_FAN},
                {7, E3DSubModel::SubModelType::SUBMODEL_GL_QUADS},
                {8, E3DSubModel::SubModelType::SUBMODEL_GL_QUAD_STRIP},
                {9, E3DSubModel::SubModelType::SUBMODEL_GL_POLYGON},
                {256, E3DSubModel::SubModelType::SUBMODEL_TRANSFORM},
                {257, E3DSubModel::SubModelType::SUBMODEL_FREE_SPOTLIGHT},
                {258, E3DSubModel::SubModelType::SUBMODEL_STARS}};

        const std::unordered_map<int, E3DSubModel::AnimationType> anim_map = {
                {0, E3DSubModel::AnimationType::ANIMATION_NONE},
                {1, E3DSubModel::AnimationType::ANIMATION_ROTATE_VEC},
                {2, E3DSubModel::AnimationType::ANIMATION_ROTATE_XYZ},
                {3, E3DSubModel::AnimationType::ANIMATION_MOVE},
                {4, E3DSubModel::AnimationType::ANIMATION_JUMP_SECONDS},
                {5, E3DSubModel::AnimationType::ANIMATION_JUMP_MINUTES},
                {6, E3DSubModel::AnimationType::ANIMATION_JUMP_HOURS},
                {7, E3DSubModel::AnimationType::ANIMATION_JUMP_HOURS24},
                {8, E3DSubModel::AnimationType::ANIMATION_SECONDS},
                {9, E3DSubModel::AnimationType::ANIMATION_MINUTES},
                {10, E3DSubModel::AnimationType::ANIMATION_HOURS},
                {11, E3DSubModel::AnimationType::ANIMATION_HOURS24},
                {12, E3DSubModel::AnimationType::ANIMATION_BILLBOARD},
                {13, E3DSubModel::AnimationType::ANIMATION_WIND},
                {14, E3DSubModel::AnimationType::ANIMATION_SKY},
                {15, E3DSubModel::AnimationType::ANIMATION_DIGITAL},
                {16, E3DSubModel::AnimationType::ANIMATION_DIGICLK},
                {17, E3DSubModel::AnimationType::ANIMATION_UNDEFINED},
                {256, E3DSubModel::AnimationType::ANIMATION_IK},
                {257, E3DSubModel::AnimationType::ANIMATION_IK1},
                {258, E3DSubModel::AnimationType::ANIMATION_IK2},
                {-1, E3DSubModel::AnimationType::ANIMATION_UNKNOWN}};

        if (const std::unordered_map<int, E3DSubModel::SubModelType>::const_iterator type_it =
                    type_map.find(static_cast<int>(p_submodel.type));
            type_it != type_map.end()) {
            submodel->set_submodel_type(type_it->second);
        } else {
            UtilityFunctions::push_warning("Unknown submodel type: " + String::num_int64(p_submodel.type));
            submodel->set_submodel_type(E3DSubModel::SubModelType::SUBMODEL_GL_TRIANGLES);
        }

        submodel->set_visible(true);
        submodel->set_skip_rendering(false);
        const String submodel_name = p_submodel.name;
        if (!submodel_name.is_empty()) {
            submodel->set_name(submodel_name);

            if (submodel_name.to_lower().begins_with(LIGHT_ON_PREFIX)) {
                submodel->set_visible(false);
            } else if (submodel_name.to_lower().ends_with(LIGHT_ON_SUFFIX)) {
                submodel->set_dynamic_hidden(true);
            } else if (submodel_name.to_lower().ends_with(LIGHT_ON_ALT_SUFFIX)) {
                submodel->set_dynamic_hidden(true);
            } else if (submodel_name == "cien") {
                submodel->set_visible(false);
                submodel->set_skip_rendering(true);
            }
        }

        switch (p_submodel.type) {
            case E3DSubModel::SubModelType::SUBMODEL_TRANSFORM:
                if (submodel_name.is_empty()) {
                    submodel->set_name("banan");
                }

                submodel->set_transform(p_submodel.matrix);
                return submodel;
            case E3DSubModel::SubModelType::SUBMODEL_GL_TRIANGLES: {
                const int64_t vertices_count = p_submodel.vertices.size();
                const String mat_name = p_submodel.material != "" ? p_submodel.material.split(":").get(0) : "";
                if (const std::unordered_map<int, E3DSubModel::AnimationType>::const_iterator anim_it =
                            anim_map.find(p_submodel.anim);
                    anim_it != anim_map.end()) {
                    submodel->set_animation(anim_it->second);
                } else {
                    submodel->set_animation(E3DSubModel::AnimationType::ANIMATION_NONE);
                }

                if (p_submodel.material_idx < 0) {
                    submodel->set_dynamic_material(true);
                    submodel->set_dynamic_material_index(abs(p_submodel.material_idx) - 1);
                }

                submodel->set_material_name(mat_name);
                submodel->set_material_transparent((p_submodel.flags & (1 << 5)) != 0);
                // the translucent skins' bits 1, 2, 4, 8 (Model3d.cpp:421-441)
                static constexpr uint32_t TRANSLUCENT_SKIN_FLAGS = 0x0F;
                submodel->set_skin_translucent(
                        p_submodel.material_idx < 0 && (p_submodel.flags & TRANSLUCENT_SKIN_FLAGS) != 0);
                submodel->set_material_colored(p_submodel.is_material_colored);
                submodel->set_visibility_range_begin(std::sqrt(p_submodel.lod_min_distance));
                submodel->set_visibility_range_end(std::sqrt(p_submodel.lod_max_distance));
                submodel->set_visibility_light(p_submodel.visibility_light_threshold);
                submodel->set_lights_on_threshold(p_submodel.lights_on_threshold);
                submodel->set_diffuse_color(p_submodel.diffuse_color);
                submodel->set_self_illumination(p_submodel.selfillum_color);

                if (vertices_count > 0) {
                    Ref<ArrayMesh> mesh;
                    mesh.instantiate();
                    Array triangles;
                    triangles.resize(ArrayMesh::ARRAY_MAX);
                    triangles.set(ArrayMesh::ARRAY_VERTEX, p_submodel.vertices);
                    // The data is wound CCW (opengl33renderer.cpp:419), Godot's front face is CW. A
                    // submodel without an IDX chunk is a plain triangle list and needs the same flip.
                    const bool indexed = p_submodel.indices.size() > 0;
                    const int64_t corner_count = indexed ? p_submodel.indices.size() : vertices_count;
                    PackedInt32Array cw_indices;
                    for (int32_t i = 0; i + 2 < corner_count; i += 3) {
                        const int32_t i1 = static_cast<int32_t>(indexed ? p_submodel.indices.get(i) : i);
                        const int32_t i2 = static_cast<int32_t>(indexed ? p_submodel.indices.get(i + 1) : i + 1);
                        const int32_t i3 = static_cast<int32_t>(indexed ? p_submodel.indices.get(i + 2) : i + 2);
                        cw_indices.append_array(PackedInt32Array({i1, i3, i2}));
                    }

                    p_submodel.indices = cw_indices;
                    if (p_submodel.normals.is_empty()) {
                        p_submodel.normals = _calculate_normals(p_submodel.vertices, p_submodel.indices);
                    }

                    if (p_submodel.indices.size() > 0) {
                        triangles.set(ArrayMesh::ARRAY_INDEX, p_submodel.indices);
                    }

                    if (p_submodel.normals.size() > 0) {
                        triangles.set(ArrayMesh::ARRAY_NORMAL, p_submodel.normals);
                    }

                    if (p_submodel.tangents.size() > 0) {
                        triangles.set(ArrayMesh::ARRAY_TANGENT, p_submodel.tangents);
                    }

                    triangles.set(ArrayMesh::ARRAY_TEX_UV, p_submodel.uvs);
                    mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, triangles);
                    submodel->set_mesh(mesh);
                }

                submodel->set_transform(p_submodel.matrix);
                return submodel;
            }
            case E3DSubModel::SubModelType::SUBMODEL_FREE_SPOTLIGHT:
                // drawn only within its range, and its point's size depends on it
                // (opengl33renderer.cpp:4292,4450)
                submodel->set_visibility_range_begin(std::sqrt(p_submodel.lod_min_distance));
                submodel->set_visibility_range_end(std::sqrt(p_submodel.lod_max_distance));
                // fLight: the glare shows below this light level (opengl33renderer.cpp:4383)
                submodel->set_lights_on_threshold(p_submodel.lights_on_threshold);
                submodel->set_light_range(p_submodel.light_range);
                submodel->set_light_attenuation(p_submodel.light_attenuation);
                submodel->set_light_angle(p_submodel.light_angle);
                submodel->set_near_attenuation_start(p_submodel.near_attenuation_start);
                submodel->set_near_attenuation_end(p_submodel.near_attenuation_end);
                submodel->set_use_near_attenuation(p_submodel.use_near_attenuation);
                submodel->set_far_attenuation_decay(p_submodel.far_attenuation_decay);
                submodel->set_cos_hotspot_angle(p_submodel.cos_hotspot_angle);
                submodel->set_cos_view_angle(p_submodel.cos_view_angle);
                submodel->set_transform(p_submodel.matrix);
                submodel->set_diffuse_color(p_submodel.diffuse_color);
                return submodel;
            default:
                UtilityFunctions::push_error(
                        "Unsupported submodel (name: " + p_submodel.name + ", type: " + String::num(p_submodel.type) +
                        ")"); //@TODO Display type name based on p_submodel.type
                return submodel;
        }
    }

    Ref<E3DModel> E3DModelBuilder::build(std::vector<SubModelData> &p_submodels) {
        // Build a list of submodels first using a simple vector to avoid Variant conversions.
        std::vector<Ref<E3DSubModel>> submodels;
        std::vector<int> parent_indices(p_submodels.size(), -1);
        submodels.reserve(p_submodels.size());

        for (SubModelData &i: p_submodels) {
            submodels.push_back(_create_submodel(i));
        }

        // Track parentage to avoid duplication in E3DModel
        std::vector has_parent(submodels.size(), false);

        // Apply parent/child relationships using references to actual stored elements
        for (size_t i = 0; i < p_submodels.size(); i++) {
            const SubModelData &meta = p_submodels.at(i);
            const Ref<E3DSubModel> &parent = submodels.at(i);

            int child_idx = meta.first_child_idx;
            while (child_idx > -1 && static_cast<size_t>(child_idx) < submodels.size()) {
                const Ref<E3DSubModel> &child = submodels.at(child_idx);
                parent->add_child(child);
                parent_indices.at(child_idx) = static_cast<int>(i);
                has_parent.at(child_idx) = true;
                child_idx = p_submodels.at(child_idx).next_idx;
            }
        }

        // Create the model and add only root-level submodels
        Ref<E3DModel> model;
        model.instantiate();
        for (size_t i = 0; i < submodels.size(); i++) {
            if (!has_parent.at(i)) {
                model->add_child(submodels.at(i));
            }
        }

        _register_lights(model, submodels, parent_indices);
        // Particle emitters: a transform submodel whose name starts with "smokesource_", the whole
        // name being the parameter file the original reads from data/ (TSubModel::is_emitter(),
        // Model3d.cpp:1417, TSubModel::find_smoke_sources(), Model3d.cpp:962). Matching is
        // case-insensitive like is_emitter(); the original's own discovery compares the raw name and
        // so misses a capitalised one.
        for (size_t i = 0; i < submodels.size(); i++) {
            const Ref<E3DSubModel> &sm = submodels[i];
            if (sm->get_submodel_type() != E3DSubModel::SUBMODEL_TRANSFORM) {
                continue;
            }
            const String sm_name = sm->get_name();
            if (!sm_name.to_lower().begins_with(SMOKE_SOURCE_PREFIX)) {
                continue;
            }

            Ref<E3DModelSmokeSourceDefinition> entry;
            entry.instantiate();
            entry->set_template_name(sm_name.to_lower());
            entry->set_submodel_path(_build_submodel_path(submodels, parent_indices, static_cast<int>(i)));
            model->register_smoke_source(entry);
        }

        return model;
    }

    // Built from the submodels' final names (not the raw E3D name table), so unnamed transforms
    // renamed to "banan" in _create_submodel() resolve the same way E3DModel::get_node_or_null() does.
    NodePath E3DModelBuilder::_build_submodel_path(
            const std::vector<Ref<E3DSubModel>> &p_submodels, const std::vector<int> &p_parent_indices, int p_index) {
        if (p_index < 0 || static_cast<size_t>(p_index) >= p_submodels.size()) {
            return NodePath();
        }

        std::vector<String> reversed_segments;
        int current_index = p_index;
        while (current_index > -1 && static_cast<size_t>(current_index) < p_submodels.size()) {
            const String segment = p_submodels.at(current_index)->get_name();
            if (segment.is_empty()) {
                return NodePath();
            }

            reversed_segments.push_back(segment);
            current_index = p_parent_indices.at(current_index);
        }

        String path;
        for (auto it = reversed_segments.rbegin(); it != reversed_segments.rend(); ++it) {
            if (!path.is_empty()) {
                path += "/";
            }
            path += *it;
        }

        return NodePath(path);
    }

    void E3DModelBuilder::_register_lights(
            const Ref<E3DModel> &p_model, const std::vector<Ref<E3DSubModel>> &p_submodels,
            const std::vector<int> &p_parent_indices) {

        std::unordered_set<size_t> used_off;

        for (size_t i = 0; i < p_submodels.size(); i++) {
            const Ref<E3DSubModel> &sm = p_submodels[i];
            String sm_name = sm->get_name();
            if (sm_name.is_empty()) {
                continue;
            }

            String sm_name_lower = sm_name.to_lower();
            String base_name;
            String off_name;

            if (sm_name_lower.ends_with(LIGHT_ON_SUFFIX)) {
                base_name = sm_name.substr(0, sm_name.length() - String(LIGHT_ON_SUFFIX).length());
                off_name = base_name + LIGHT_OFF_SUFFIX;
            } else if (sm_name_lower.ends_with(LIGHT_ON_ALT_SUFFIX)) {
                base_name = sm_name.substr(0, sm_name.length() - String(LIGHT_ON_ALT_SUFFIX).length());
                off_name = base_name + LIGHT_OFF_SUFFIX;
            } else if (sm_name_lower.begins_with(LIGHT_ON_PREFIX)) {
                base_name = sm_name.substr(String(LIGHT_ON_PREFIX).length());
                off_name = String(LIGHT_OFF_PREFIX) + base_name.to_lower();
            } else {
                continue;
            }

            if (base_name.is_empty()) {
                continue;
            }

            String base_name_lower = base_name.to_lower();
            if (std::find(
                        NON_LIGHTS_TO_EXCLUDE.begin(), NON_LIGHTS_TO_EXCLUDE.end(),
                        std::string_view(base_name_lower.utf8().get_data())) != NON_LIGHTS_TO_EXCLUDE.end()) {
                continue;
            }

            String light_name = base_name;

            // "<name>_on" and "<name>_xon" are the same light, lit and dimmed (TButton::Init(),
            // Button.cpp:32-40); the first of each in tree order is bound, as in the original
            // (GetFromName()) - other copies keep the default of any "_on" submodel (Model3d.cpp:2221).
            const bool dimmed = sm_name_lower.ends_with(LIGHT_ON_ALT_SUFFIX);
            Ref<E3DModelLightDefinition> entry = p_model->get_lights().get(light_name, Ref<E3DModelLightDefinition>());
            const bool registered = entry.is_valid();
            if (!registered) {
                entry.instantiate();
            }
            const NodePath path = _build_submodel_path(p_submodels, p_parent_indices, static_cast<int>(i));
            const NodePath bound_path = dimmed ? entry->get_xon_submodel_path() : entry->get_on_submodel_path();
            if (!bound_path.is_empty()) {
                // A copy nested under the bound one is part of that light: the original switches
                // only the bound submodel, and its children show with it (sem/karzelki/ktmnb.e3d
                // keeps the lens under a light_on00 transform of the same name)
                if (String(path).begins_with(String(bound_path) + "/")) {
                    sm->set_visible(true);
                    continue;
                }
                UtilityFunctions::push_warning(
                        "[E3DModelBuilder]: Duplicate light name (first one is used, as in the original): " + sm_name);
                continue;
            }
            if (dimmed) {
                entry->set_xon_submodel_path(path);
            } else {
                entry->set_on_submodel_path(path);
            }

            if (entry->get_off_submodel_path().is_empty()) {
                String off_name_lower = off_name.to_lower();
                for (size_t j = 0; j < p_submodels.size(); j++) {
                    if (used_off.find(j) == used_off.end() && p_submodels[j]->get_name().to_lower() == off_name_lower) {
                        used_off.insert(j);
                        entry->set_off_submodel_path(
                                _build_submodel_path(p_submodels, p_parent_indices, static_cast<int>(j)));
                        break;
                    }
                }
            }

            if (!registered) {
                p_model->register_light(light_name, entry);
            }
        }
    }
} // namespace godot
