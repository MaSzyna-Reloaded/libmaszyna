#include "legacy/scenery/MaszynaLegacySBTTerrainProvider.hpp"

#include "scenery/SceneryTrianglesSink.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <limits>

namespace godot {
    namespace {
        /// Where the original keeps its materials (paths::textures); a "triangles" node names one
        /// without it
        constexpr const char *TEXTURES_PREFIX = "textures/";

        /// The little-endian values of sn_utils (scene/sn_utils.cpp) read out of one section
        class SectionCursor {
                const PackedByteArray &data;
                int64_t position = 0;

                /// The offset of the next p_bytes, marking an overrun when they are not there
                int64_t take(const int64_t p_bytes) {
                    if (overrun || position + p_bytes > data.size()) {
                        overrun = true;
                        return -1;
                    }
                    const int64_t offset = position;
                    position += p_bytes;
                    return offset;
                }

            public:
                bool overrun = false;

                explicit SectionCursor(const PackedByteArray &p_data) : data(p_data) {}

                void skip(const int64_t p_bytes) {
                    take(p_bytes);
                }

                uint32_t u32() {
                    const int64_t offset = take(sizeof(uint32_t));
                    return offset < 0 ? 0 : static_cast<uint32_t>(data.decode_u32(offset));
                }

                float f32() {
                    const int64_t offset = take(sizeof(float));
                    return offset < 0 ? 0.0F : static_cast<float>(data.decode_float(offset));
                }

                double f64() {
                    const int64_t offset = take(sizeof(double));
                    return offset < 0 ? 0.0 : data.decode_double(offset);
                }

                /// sn_utils::d_bool(): a uint16 that is 1
                bool boolean() {
                    const int64_t offset = take(sizeof(uint16_t));
                    return offset >= 0 && data.decode_u16(offset) == 1;
                }

                /// sn_utils::d_str(): bytes up to a zero, as the scenery parser reads them
                String string() {
                    String text;
                    while (true) {
                        const int64_t offset = take(1);
                        if (offset < 0 || data[offset] == 0) {
                            return text;
                        }
                        text += static_cast<char32_t>(data[offset]);
                    }
                }

                Vector3 dvec3() {
                    const double x = f64();
                    const double y = f64();
                    const double z = f64();
                    return {x, y, z};
                }

                Vector3 vec3() {
                    const float x = f32();
                    const float y = f32();
                    const float z = f32();
                    return {x, y, z};
                }
        };

        /// bounding_area::serialize(): its centre and radius (scenenode.cpp:41)
        constexpr int64_t AREA_BYTES = (3 * sizeof(double)) + sizeof(float);
        /// lighting_data::serialize(): diffuse, ambient and specular vec4 (scenenode.cpp:21)
        constexpr int64_t LIGHTING_BYTES = static_cast<int64_t>(3 * 4) * sizeof(float);
        /// basic_vertex::serialize() without a tangent: position, normal, uv (geometrybank.cpp:41)
        constexpr int64_t VERTEX_BYTES = 8 * sizeof(float);
        /// vertex_userdata::serialize(): one vec4 (geometrybank.cpp:460)
        constexpr int64_t USERDATA_BYTES = 4 * sizeof(float);
        /// A visibility range squared at this or above is no limit (shape_node::import(),
        /// scenenode.cpp:157)
        constexpr double NO_RANGE_SQUARED = std::numeric_limits<double>::max();
        /// The range a "triangles" node without a limit gives the sink
        constexpr float NO_RANGE = -1.0F;

    } // namespace

    /// The shapes of one section sharing a texture and a visibility range: one chunk of terrain
    struct ShapeGroup {
            String texture;
            float range_min = 0.0F;
            float range_max = 0.0F;
            std::vector<float> vertices;
            std::vector<float> normals;
            std::vector<float> uvs;
    };

    struct ShapeVertex {
            Vector3 position;
            Vector3 normal;
            Vector2 uv;
    };

    static PackedFloat32Array to_packed(const std::vector<float> &p_values) {
        PackedFloat32Array packed;
        packed.resize(static_cast<int64_t>(p_values.size()));
        std::copy(p_values.begin(), p_values.end(), packed.ptrw());
        return packed;
    }

    /// shape_node::deserialize() (scenenode.cpp:139, :99): its triangles into the group of its texture
    /// and range, relative to the cell's origin
    static void read_shape(
            SectionCursor &p_cursor, const Vector3 &p_cell_origin, std::vector<ShapeGroup> &p_groups,
            HashMap<String, int> &p_group_indices) {
        p_cursor.string(); // name
        p_cursor.skip(AREA_BYTES);
        const double range_squared_min = p_cursor.f64();
        const double range_squared_max = p_cursor.f64();
        const bool visible = p_cursor.boolean();
        p_cursor.boolean(); // translucent - the material says it
        const bool has_userdata = p_cursor.boolean();
        String texture = p_cursor.string();
        p_cursor.skip(LIGHTING_BYTES);
        const Vector3 origin = p_cursor.dvec3();
        const uint32_t vertex_count = p_cursor.u32();
        std::vector<ShapeVertex> shape_vertices;
        shape_vertices.reserve(vertex_count);
        for (uint32_t index = 0; index < vertex_count && !p_cursor.overrun; index++) {
            ShapeVertex vertex;
            vertex.position = origin + p_cursor.vec3();
            vertex.normal = p_cursor.vec3();
            const float u = p_cursor.f32();
            const float v = p_cursor.f32();
            vertex.uv = Vector2(u, v);
            if (has_userdata) {
                p_cursor.skip(USERDATA_BYTES);
            }
            shape_vertices.push_back(vertex);
        }
        if (p_cursor.overrun || !visible || shape_vertices.empty()) {
            return;
        }
        texture = texture.trim_prefix(TEXTURES_PREFIX);
        const float range_min = static_cast<float>(Math::sqrt(range_squared_min));
        const float range_max =
                range_squared_max >= NO_RANGE_SQUARED ? NO_RANGE : static_cast<float>(Math::sqrt(range_squared_max));
        const String key = texture + "|" + String::num(range_min) + "|" + String::num(range_max);
        const int *found = p_group_indices.getptr(key);
        if (found == nullptr) {
            p_group_indices[key] = static_cast<int>(p_groups.size());
            p_groups.push_back(ShapeGroup{texture, range_min, range_max, {}, {}, {}});
        }
        ShapeGroup &group = p_groups[p_group_indices[key]];
        // turned around, as MaszynaTrianglesImporter turns a "triangles" node
        for (auto vertex = shape_vertices.rbegin(); vertex != shape_vertices.rend(); ++vertex) {
            const Vector3 position = vertex->position - p_cell_origin;
            group.vertices.insert(
                    group.vertices.end(),
                    {static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)});
            group.normals.insert(
                    group.normals.end(), {static_cast<float>(vertex->normal.x), static_cast<float>(vertex->normal.y),
                                          static_cast<float>(vertex->normal.z)});
            group.uvs.insert(group.uvs.end(), {static_cast<float>(vertex->uv.x), static_cast<float>(vertex->uv.y)});
        }
    }

    /// lines_node::deserialize() (scenenode.cpp:550, :513) - read past, as "lines" nodes are
    static void skip_lines(SectionCursor &p_cursor) {
        p_cursor.string(); // name
        // area, range min and max, visible, line width, lighting, origin
        p_cursor.skip(
                AREA_BYTES + (2 * sizeof(double)) + sizeof(uint16_t) + sizeof(float) + LIGHTING_BYTES +
                (3 * sizeof(double)));
        p_cursor.skip(static_cast<int64_t>(p_cursor.u32()) * VERTEX_BYTES);
    }

    void MaszynaLegacySBTTerrainProvider::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaLegacySBTTerrainProvider", D_METHOD("is_region", "path"),
                &MaszynaLegacySBTTerrainProvider::is_region);
        ClassDB::bind_method(D_METHOD("open", "path"), &MaszynaLegacySBTTerrainProvider::open);
    }

    bool MaszynaLegacySBTTerrainProvider::is_region(const String &p_path) {
        const Ref<FileAccess> region = FileAccess::open(p_path, FileAccess::READ);
        if (region.is_null()) {
            return false;
        }
        const uint32_t header = region->get_32();
        const uint32_t version = region->get_32();
        return header == FILE_HEADER && version == FILE_VERSION;
    }

    /// The file's sections are listed, not read: their index, place, size and reach (scene.cpp:1213-1220)
    bool MaszynaLegacySBTTerrainProvider::open(const String &p_path) {
        MutexLock lock(mutex);
        sections.clear();
        file = FileAccess::open(p_path, FileAccess::READ);
        if (file.is_null()) {
            return false;
        }
        const uint32_t header = file->get_32();
        const uint32_t version = file->get_32();
        if (header != FILE_HEADER || version != FILE_VERSION) {
            UtilityFunctions::push_error("Bad file: \"" + p_path + "\" is of either unrecognized type or version");
            file.unref();
            return false;
        }
        // a section's size counts its index and size too (basic_section::serialize(), scene.cpp:784)
        constexpr int64_t SECTION_HEAD_BYTES = 2 * sizeof(uint32_t);
        // the radius a section starts with (scene.h:353); one grown past it holds a shape reaching out
        const double section_radius = 0.5 * Math::sqrt(2.0) * SceneryTrianglesSink::CHUNK_SIZE_M;
        for (uint32_t section_count = file->get_32(); section_count > 0 && !file->eof_reached(); section_count--) {
            const uint32_t index = file->get_32();
            const int64_t size = static_cast<int64_t>(file->get_32()) - SECTION_HEAD_BYTES;
            Section section;
            section.offset = file->get_position();
            section.size = size;
            file->seek(section.offset + (3 * sizeof(double))); // past the bounding area's centre, to its radius
            section.overhang = MAX(0.0F, static_cast<float>(file->get_float() - section_radius));
            file->seek(section.offset + size);
            // the original's section index (basic_region::section(), scene.cpp:1723)
            const Vector2i cell(
                    static_cast<int>(index % REGION_SIDE_SECTION_COUNT) - (REGION_SIDE_SECTION_COUNT / 2),
                    static_cast<int>(index / REGION_SIDE_SECTION_COUNT) - (REGION_SIDE_SECTION_COUNT / 2));
            sections[cell] = section;
        }
        return true;
    }

    TypedArray<Vector2i> MaszynaLegacySBTTerrainProvider::get_chunk_cells() const {
        TypedArray<Vector2i> cells;
        MutexLock lock(mutex);
        for (const KeyValue<Vector2i, Section> &item: sections) {
            cells.append(item.key);
        }
        return cells;
    }

    float MaszynaLegacySBTTerrainProvider::chunk_get_overhang(const Vector2i &p_cell) const {
        MutexLock lock(mutex);
        const Section *section = sections.getptr(p_cell);
        return section != nullptr ? section->overhang : 0.0F;
    }

    PackedInt32Array MaszynaLegacySBTTerrainProvider::get_content_kinds() const {
        PackedInt32Array kinds;
        kinds.push_back(CONTENT_TERRAIN);
        return kinds;
    }

    /// basic_section::deserialize() (scene.cpp:811) and basic_cell::deserialize() (scene.cpp:227) of one
    /// section: its shapes, one MaszynaTrianglesChunkGeometry per texture and range
    Array MaszynaLegacySBTTerrainProvider::chunk_load(const Vector2i &p_cell, const ContentKind p_kind) {
        Array geometries;
        if (p_kind != CONTENT_TERRAIN) {
            return geometries;
        }
        PackedByteArray data;
        {
            MutexLock lock(mutex);
            const Section *section = sections.getptr(p_cell);
            if (section == nullptr || file.is_null()) {
                return geometries;
            }
            file->seek(section->offset);
            data = file->get_buffer(section->size);
        }
        SectionCursor cursor(data);
        const Vector3 cell_origin = SceneryTrianglesSink::cell_get_origin(p_cell);
        std::vector<ShapeGroup> groups;
        HashMap<String, int> group_indices;
        cursor.skip(AREA_BYTES);
        for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
            read_shape(cursor, cell_origin, groups, group_indices);
        }
        for (int cell = 0; cell < SECTION_CELL_COUNT && !cursor.overrun; cell++) {
            cursor.skip(AREA_BYTES);
            // opaque shapes, then translucent ones
            for (int kind = 0; kind < 2; kind++) {
                for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
                    read_shape(cursor, cell_origin, groups, group_indices);
                }
            }
            for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
                skip_lines(cursor);
            }
        }
        ERR_FAIL_COND_V_MSG(cursor.overrun, geometries, "Bad file: a region section ends early");
        for (const ShapeGroup &group: groups) {
            Ref<MaszynaTrianglesChunkGeometry> geometry;
            geometry.instantiate();
            geometry->set_texture(group.texture);
            geometry->set_cell(p_cell);
            geometry->set_range_min(group.range_min);
            geometry->set_range_max(group.range_max);
            geometry->set_vertices(to_packed(group.vertices));
            geometry->set_normals(to_packed(group.normals));
            geometry->set_uvs(to_packed(group.uvs));
            geometries.append_array(geometry->split());
        }
        return geometries;
    }
} // namespace godot
