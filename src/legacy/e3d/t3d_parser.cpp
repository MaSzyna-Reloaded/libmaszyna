#include "legacy/e3d/t3d_parser.hpp"
#include "legacy/parsers/maszyna_parser.hpp"
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <algorithm>
#include <array>
#include <deque>
#include <string>
#include <unordered_map>

namespace godot {
    /// readColor(), Model3d.cpp:213
    static constexpr float COLOR_SCALE = 255.0F;
    /// "selfillum: true" and "false", Model3d.cpp:346-348
    static constexpr float ALWAYS_LIT = 2.0F;
    static constexpr float NEVER_LIT = -1.0F;
    /// Opacity above 1 (or below -1) is given in percent, Model3d.cpp:399-406
    static constexpr float PERCENT = 0.01F;
    /// Drawn in the translucent phase below this opacity, Model3d.cpp:421-455
    static constexpr float OPAQUE_OPACITY = 0.999F;
    /// iFlags render phases, Model3d.cpp:416, 455
    static constexpr uint32_t FLAG_OPAQUE = 0x10;
    static constexpr uint32_t FLAG_TRANSLUCENT = 0x20;
    /// "map: -1" .. "-4", Model3d.cpp:423-442
    static constexpr int REPLACEABLE_SKIN_COUNT = 4;
    /// "hotspotpower:" is a percentage of the light's diffuse, Model3d.cpp:372
    static constexpr float HOTSPOT_POWER_SCALE = 100.0F;
    /// No max distance means 15 km, Model3d.cpp:496
    static constexpr float DEFAULT_MAX_DISTANCE = 15000.0F;
    static constexpr int MATRIX_SIZE = 16;
    /// x y z w, Model3d.cpp:569
    static constexpr int TANGENT_SIZE = 4;
    /// A triangle mask that gives every vertex its own normal, Model3d.cpp:593
    static constexpr int64_t EXPLICIT_NORMALS_MASK = -1;
    /// An edge longer than this drops the triangle, Model3d.cpp:633
    static constexpr float MAX_EDGE_LENGTH = 1000.0F;
    /// A face turned this far from the normal summed so far is not smoothed into it, Model3d.cpp:675
    static constexpr float OPPOSITE_FACE_DOT = -0.99F;
    /// UserdataParse(), Model3d.cpp:226
    static constexpr int USERDATA_TOKENS = 4;
    /// Position, colour and a discarded token per star, Model3d.cpp:740
    static constexpr int STAR_TOKENS = 5;

    void T3DParser::_bind_methods() {
        ClassDB::bind_method(D_METHOD("parse", "file"), &T3DParser::parse);
    }

    Ref<E3DModel> T3DParser::parse(const Ref<FileAccess> &p_file) const {
        // the original's TAnimType as the .t3d names it (Model3d.cpp:300-327); anything else is
        // at_Undefined
        static const std::unordered_map<std::string, E3DSubModel::AnimationType> animations = {
                {"seconds_jump", E3DSubModel::ANIMATION_JUMP_SECONDS},
                {"minutes_jump", E3DSubModel::ANIMATION_JUMP_MINUTES},
                {"hours_jump", E3DSubModel::ANIMATION_JUMP_HOURS},
                {"hours24_jump", E3DSubModel::ANIMATION_JUMP_HOURS24},
                {"seconds", E3DSubModel::ANIMATION_SECONDS},
                {"minutes", E3DSubModel::ANIMATION_MINUTES},
                {"hours", E3DSubModel::ANIMATION_HOURS},
                {"hours24", E3DSubModel::ANIMATION_HOURS24},
                {"billboard", E3DSubModel::ANIMATION_BILLBOARD},
                {"wind", E3DSubModel::ANIMATION_WIND},
                {"sky", E3DSubModel::ANIMATION_SKY},
                {"digital", E3DSubModel::ANIMATION_DIGITAL},
                {"digiclk", E3DSubModel::ANIMATION_DIGICLK},
        };

        Ref<MaszynaParser> parser;
        parser.instantiate();
        parser->initialize(p_file->get_buffer(static_cast<int64_t>(p_file->get_length())), Array());

        const auto read_float = [&parser]() { return static_cast<float>(parser->next_token().to_float()); };
        std::vector<SubModelData> submodels;
        int first_root = -1;
        String token = parser->next_token();
        while (token.to_lower() == "parent:") {
            const String parent_name = parser->next_token();
            if (parent_name.is_empty()) {
                break;
            }

            if (!(parser->next_token().to_lower() == "type:")) {
                // the submodel is skipped up to the next one, Model3d.cpp:242-256
                UtilityFunctions::push_warning(
                        "Bad model: expected submodel type definition not found in " + p_file->get_path());
                token = parser->next_token();
                while (!parser->eof_reached() && !(token.to_lower() == "parent:")) {
                    token = parser->next_token();
                }
                continue;
            }

            SubModelData submodel{};
            submodel.next_idx = -1;
            submodel.first_child_idx = -1;
            submodel.type = E3DSubModel::SUBMODEL_TRANSFORM;
            const String type = parser->next_token().to_lower();
            if (type == "mesh") {
                submodel.type = E3DSubModel::SUBMODEL_GL_TRIANGLES;
            } else if (type == "point") {
                submodel.type = E3DSubModel::SUBMODEL_GL_POINTS;
            } else if (type == "freespotlight") {
                submodel.type = E3DSubModel::SUBMODEL_FREE_SPOTLIGHT;
            } else if (type == "stars") {
                submodel.type = E3DSubModel::SUBMODEL_STARS;
            }
            const bool has_geometry = submodel.type < E3DSubModel::SUBMODEL_TRANSFORM;
            const bool is_spotlight = submodel.type == E3DSubModel::SUBMODEL_FREE_SPOTLIGHT;

            parser->next_token(); // "name:"
            submodel.name = parser->next_token();
            parser->next_token(); // "anim:"
            if (const String anim = parser->next_token().to_lower(); !(anim == "false")) {
                const auto animation = animations.find(anim.utf8().get_data());
                submodel.anim = animation == animations.end() ? E3DSubModel::ANIMATION_UNDEFINED : animation->second;
            }

            if (has_geometry) {
                parser->get_tokens(4); // "ambient:" r g b
            }
            parser->next_token(); // "diffuse:"
            const float diffuse_r = read_float() / COLOR_SCALE;
            const float diffuse_g = read_float() / COLOR_SCALE;
            const float diffuse_b = read_float() / COLOR_SCALE;
            submodel.diffuse_color = Color(diffuse_r, diffuse_g, diffuse_b);
            if (has_geometry) {
                parser->get_tokens(4); // "specular:" r g b
            }
            parser->next_token(); // "selfillum:"
            const String light = parser->next_token().to_lower();
            if (light == "true") {
                submodel.lights_on_threshold = ALWAYS_LIT;
            } else if (light == "false") {
                submodel.lights_on_threshold = NEVER_LIT;
            } else {
                submodel.lights_on_threshold = static_cast<float>(light.to_float());
            }
            submodel.selfillum_color = Color(1, 1, 1, 1); // f4Emision, Model3d.h:116 - the text has none

            if (is_spotlight) {
                // Model3d.cpp:352-392
                parser->next_token(); // "nearattenstart:"
                submodel.near_attenuation_start = read_float();
                parser->next_token(); // "nearattenend:"
                submodel.near_attenuation_end = read_float();
                parser->next_token(); // "usenearatten:"
                submodel.use_near_attenuation = parser->as_bool(parser->next_token());
                parser->next_token(); // "farattendecay:"
                submodel.far_attenuation_decay = static_cast<int>(parser->next_token().to_int());
                parser->next_token(); // "fardecayradius:"
                submodel.light_range = read_float();
                parser->next_token(); // "falloffangle:"
                float cos_falloff = read_float();
                parser->next_token(); // "hotspotangle:"
                submodel.cos_hotspot_angle = read_float();
                submodel.light_energy = 1.0F;
                // "hotspotpower:" is optional, so the next key is read here either way
                if (parser->next_token().to_lower() == "hotspotpower:") {
                    submodel.light_energy = read_float() / HOTSPOT_POWER_SCALE;
                    parser->next_token(); // "maxdistance:"
                }
                // cone angles above 1 are the full angle in degrees
                if (cos_falloff > 1.0F) {
                    cos_falloff = Math::cos(Math::deg_to_rad(0.5F * cos_falloff));
                }
                if (submodel.cos_hotspot_angle > 1.0F) {
                    submodel.cos_hotspot_angle = Math::cos(Math::deg_to_rad(0.5F * submodel.cos_hotspot_angle));
                }
                // as E3DParser::_read_submodel() derives them from fCosFalloffAngle
                submodel.light_angle = Math::rad_to_deg(Math::acos(Math::clamp(cos_falloff, -1.0F, 1.0F)));
                submodel.light_attenuation = 1.0F;
            } else if (has_geometry) {
                // Model3d.cpp:393-457
                parser->get_tokens(4); // "wire:" x "wiresize:" x
                parser->next_token();  // "opacity:"
                float opacity = read_float();
                if (opacity > 1.0F) {
                    opacity = std::min(1.0F, opacity * PERCENT);
                }
                if (opacity < -1.0F) {
                    opacity = std::max(-1.0F, opacity * PERCENT);
                }
                const bool translucent = opacity < OPAQUE_OPACITY;
                parser->next_token(); // "map:"
                const String material = parser->next_token().to_lower().replace("\\", "/");
                // "replacableskin" is the first replaceable skin (McZapkie-060702, Model3d.cpp:418)
                int64_t skin = 0;
                if (material.contains("replacableskin")) {
                    skin = -1;
                } else if (material.is_valid_int()) {
                    skin = material.to_int();
                }
                if (material == "none") {
                    submodel.material = "colored";
                    submodel.is_material_colored = true;
                    submodel.flags = FLAG_OPAQUE;
                } else if (skin < 0 && skin >= -REPLACEABLE_SKIN_COUNT) {
                    // replaceable skin n is translucent through flag bit n-1
                    submodel.material_idx = static_cast<int>(skin);
                    submodel.flags = translucent ? 1U << (-skin - 1) : FLAG_OPAQUE;
                } else {
                    submodel.material = material;
                    submodel.flags = translucent ? FLAG_TRANSLUCENT : FLAG_OPAQUE;
                }
            }

            if (!is_spotlight) {
                parser->next_token(); // "maxdistance:"
            }
            // kept squared, as an .e3d stores them (Model3d.cpp:493-499)
            float max_distance = read_float();
            parser->next_token(); // "mindistance:"
            const float min_distance = read_float();
            if (max_distance <= 0.0F) {
                max_distance = DEFAULT_MAX_DISTANCE;
            }
            submodel.lod_max_distance = max_distance * max_distance;
            submodel.lod_min_distance = min_distance * min_distance;

            parser->next_token(); // "transform:"
            std::array<float, MATRIX_SIZE> matrix{};
            for (float &value: matrix) {
                value = read_float();
            }
            submodel.matrix = E3DModelBuilder::make_matrix(matrix);

            if (has_geometry) {
                // Model3d.cpp:519-721
                String key = parser->next_token().to_lower();
                bool has_userdata = false;
                if (key == "userdata:") {
                    has_userdata = parser->as_bool(parser->next_token());
                    key = parser->next_token().to_lower();
                }
                if (key == "numindices:") {
                    const int64_t index_count = parser->next_token().to_int();
                    for (int64_t i = 0; i < index_count; ++i) {
                        submodel.indices.append(static_cast<int32_t>(parser->next_token().to_int()));
                    }
                    key = parser->next_token().to_lower();
                }
                if (key == "numverts:" || key == "numverts") {
                    const int vertex_count = static_cast<int>(parser->next_token().to_int());
                    if (submodel.indices.is_empty() && vertex_count % 3 != 0) {
                        UtilityFunctions::push_error(
                                "Bad model: incomplete triangle encountered in submodel \"" + submodel.name + "\" of " +
                                p_file->get_path());
                        return {};
                    }
                    if (!submodel.indices.is_empty()) {
                        // indexed geometry comes with its normals and tangents
                        for (int i = 0; i < vertex_count; ++i) {
                            submodel.vertices.append(Vector3{read_float(), read_float(), read_float()});
                            submodel.normals.append(Vector3{read_float(), read_float(), read_float()});
                            submodel.uvs.append(Vector2{read_float(), read_float()});
                            for (int t = 0; t < TANGENT_SIZE; ++t) {
                                submodel.tangents.append(read_float());
                            }
                        }
                    } else {
                        // legacy triangle list: a smoothing group mask before each triangle,
                        // normals given only under mask -1, Model3d.cpp:576-695
                        std::vector<Vector3> positions(vertex_count);
                        std::vector<Vector3> normals(vertex_count);
                        std::vector<Vector2> uvs(vertex_count);
                        std::vector<uint32_t> masks(vertex_count / 3);
                        // which vertex's normal this one takes, -1 while it is not computed
                        std::vector<int> normal_sources(vertex_count, -1);
                        int kept_count = vertex_count;
                        bool explicit_normals = false;
                        for (int i = 0; i < kept_count; ++i) {
                            if (i % 3 == 0) {
                                const int64_t mask = parser->next_token().to_int();
                                explicit_normals = mask == EXPLICIT_NORMALS_MASK;
                                masks[i / 3] = explicit_normals ? 0 : static_cast<uint32_t>(mask);
                            }
                            positions[i] = Vector3{read_float(), read_float(), read_float()};
                            normal_sources[i] = -1;
                            if (explicit_normals) {
                                const Vector3 normal{read_float(), read_float(), read_float()};
                                normals[i] = normal.length_squared() > 0.0 ? normal.normalized() : normal;
                                normal_sources[i] = i;
                            }
                            uvs[i] = Vector2{read_float(), read_float()};

                            if (i % 3 == 2) {
                                const Vector3 &a = positions[i];
                                const Vector3 &b = positions[i - 1];
                                const Vector3 &c = positions[i - 2];
                                const bool degenerate = (b - a).cross(c - a).length_squared() == 0.0;
                                const real_t max_edge_squared = MAX_EDGE_LENGTH * MAX_EDGE_LENGTH;
                                const bool too_large = a.distance_squared_to(b) > max_edge_squared ||
                                                       b.distance_squared_to(c) > max_edge_squared ||
                                                       c.distance_squared_to(a) > max_edge_squared;
                                if (degenerate || too_large) {
                                    // the next triangle is read into its place
                                    kept_count -= 3;
                                    i -= 3;
                                }
                            }
                        }

                        const int face_count = kept_count / 3;
                        std::vector<Vector3> face_normals(face_count);
                        for (size_t f = 0; f < face_normals.size(); ++f) {
                            const Vector3 &a = positions[3 * f];
                            const Vector3 face_normal = (a - positions[(3 * f) + 1]).cross(a - positions[(3 * f) + 2]);
                            face_normals[f] = face_normal.length_squared() > 0.0 ? face_normal.normalized() : Vector3();
                        }
                        for (int i = 0; i < kept_count; ++i) {
                            if (normal_sources[i] >= 0) {
                                normals[i] = normals[normal_sources[i]];
                                continue;
                            }
                            const int face = i / 3;
                            Vector3 normal;
                            int adjacent = i;
                            while (adjacent >= 0) {
                                if (normal.dot(face_normals[adjacent / 3]) > OPPOSITE_FACE_DOT) {
                                    normal_sources[adjacent] = i;
                                    normal += face_normals[adjacent / 3];
                                }
                                // the same position in a later face of a shared smoothing group
                                // (TSubModel::SeekFaceNormal(), Model3d.cpp:184)
                                int next = -1;
                                for (int f = (adjacent / 3) + 1; f < face_count && next < 0; ++f) {
                                    if ((masks[f] & masks[face]) == 0) {
                                        continue;
                                    }
                                    for (int corner = 3 * f; corner < (3 * f) + 3; ++corner) {
                                        if (positions[corner] == positions[i]) {
                                            next = corner;
                                            break;
                                        }
                                    }
                                }
                                adjacent = next;
                            }
                            normals[i] = normal.length_squared() > 0.0 ? normal.normalized() : face_normals[face];
                        }

                        for (int i = 0; i < kept_count; ++i) {
                            submodel.vertices.append(positions[i]);
                            submodel.normals.append(normals[i]);
                            submodel.uvs.append(uvs[i]);
                        }
                    }
                    if (has_userdata) {
                        parser->get_tokens(static_cast<int>(submodel.vertices.size()) * USERDATA_TOKENS);
                    }
                    if (submodel.vertices.is_empty()) {
                        // a helper submodel with a transform only, Model3d.cpp:716
                        submodel.type = E3DSubModel::SUBMODEL_TRANSFORM;
                    }
                }
            } else if (submodel.type == E3DSubModel::SUBMODEL_STARS) {
                // read past, not drawn - as from an .e3d (Model3d.cpp:723-747)
                parser->next_token(); // "numverts:"
                const int64_t star_count = parser->next_token().to_int();
                for (int64_t i = 0; i < star_count; ++i) {
                    parser->get_tokens(i % 3 == 0 ? STAR_TOKENS + 1 : STAR_TOKENS);
                }
            }

            // TModel3d::AddTo(), Model3d.cpp:1540: the new one goes first among its parent's
            // children, or first in the main chain
            const int index = static_cast<int>(submodels.size());
            const int parent = _find_by_name(submodels, first_root, parent_name);
            if (parent < 0 && !(parent_name == "none")) {
                UtilityFunctions::push_warning(
                        "Bad model: parent for sub-model \"" + submodel.name +
                        "\" doesn't exist or is located later in " + p_file->get_path());
            }
            if (parent < 0) {
                submodel.next_idx = first_root;
                first_root = index;
            } else {
                submodel.next_idx = submodels[parent].first_child_idx;
                submodels[parent].first_child_idx = index;
            }
            submodels.push_back(submodel);
            token = parser->next_token();
        }

        _rotate_to_scenery_frame(submodels, first_root);

        // Numbered as TModel3d::SaveToBinFile() numbers them for an .e3d - each one's next, then
        // its first child, in the order they are reached (get_container_pos(), Model3d.cpp:1666,
        // 1690) - so the lights bind and the roots come as from the converted file
        std::vector<int> order;
        std::vector<int> new_indices(submodels.size(), -1);
        std::deque<int> pending;
        if (first_root >= 0) {
            pending.push_back(first_root);
            new_indices[first_root] = 0;
            order.push_back(first_root);
        }
        while (!pending.empty()) {
            const SubModelData &submodel = submodels[pending.front()];
            pending.pop_front();
            for (const int linked: {submodel.next_idx, submodel.first_child_idx}) {
                if (linked >= 0 && new_indices[linked] < 0) {
                    new_indices[linked] = static_cast<int>(order.size());
                    order.push_back(linked);
                    pending.push_back(linked);
                }
            }
        }
        std::vector<SubModelData> ordered;
        ordered.reserve(order.size());
        for (const int old_index: order) {
            SubModelData &submodel = ordered.emplace_back(submodels[old_index]);
            submodel.next_idx = submodel.next_idx < 0 ? -1 : new_indices[submodel.next_idx];
            submodel.first_child_idx = submodel.first_child_idx < 0 ? -1 : new_indices[submodel.first_child_idx];
        }

        return E3DModelBuilder::build(ordered);
    }

    /// TSubModel::GetFromName(), Model3d.cpp:1092: case-insensitive, the next ones before the children
    int
    T3DParser::_find_by_name(const std::vector<SubModelData> &p_submodels, const int p_index, const String &p_name) {
        if (p_index < 0 || p_name.is_empty()) {
            return -1;
        }
        const SubModelData &submodel = p_submodels[p_index];
        if (submodel.name.nocasecmp_to(p_name) == 0) {
            return p_index;
        }
        if (const int found = _find_by_name(p_submodels, submodel.next_idx, p_name); found >= 0) {
            return found;
        }
        return _find_by_name(p_submodels, submodel.first_child_idx, p_name);
    }

    /// TSubModel::InitialRotate(true), Model3d.cpp:818, for a chain of submodels: an animated one,
    /// or one with its own transform, is turned by the transform and its children stay as they are;
    /// any other has its geometry turned and passes the turn on to its children
    void T3DParser::_rotate_to_scenery_frame(std::vector<SubModelData> &p_submodels, const int p_index) {
        // the scenery frame from the 3ds Max one the text is written in: X negated, Y and Z swapped
        // (float4x4::InitialRotate(), Float3d.h:238); an .e3d is saved already turned
        const Basis scenery_frame(Vector3(-1, 0, 0), Vector3(0, 0, 1), Vector3(0, 1, 0));
        for (int index = p_index; index >= 0; index = p_submodels[index].next_idx) {
            SubModelData &submodel = p_submodels[index];
            const bool animated = submodel.anim != E3DSubModel::ANIMATION_NONE ||
                                  submodel.type == E3DSubModel::SUBMODEL_FREE_SPOTLIGHT;
            if (animated || !(submodel.matrix == Transform3D())) {
                submodel.matrix = Transform3D(scenery_frame, Vector3()) * submodel.matrix;
                continue;
            }
            for (int64_t i = 0; i < submodel.vertices.size(); ++i) {
                submodel.vertices.set(i, scenery_frame.xform(submodel.vertices[i]));
                submodel.normals.set(i, scenery_frame.xform(submodel.normals[i]));
            }
            for (int64_t i = 0; i + 3 < submodel.tangents.size(); i += 4) {
                const Vector3 tangent = scenery_frame.xform(
                        Vector3(submodel.tangents[i], submodel.tangents[i + 1], submodel.tangents[i + 2]));
                submodel.tangents.set(i, tangent.x);
                submodel.tangents.set(i + 1, tangent.y);
                submodel.tangents.set(i + 2, tangent.z);
            }
            _rotate_to_scenery_frame(p_submodels, submodel.first_child_idx);
        }
    }
} // namespace godot
