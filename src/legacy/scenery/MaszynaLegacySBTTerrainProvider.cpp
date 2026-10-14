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

    /// The vectors of a chunk of terrain, relative to its origin, as floats, and its groups: each
    /// one shape or one texture of a section (MaszynaTrianglesChunkGeometry)
    struct ShapeGeometry {
            std::vector<float> vertices;
            std::vector<float> normals;
            std::vector<float> uvs;
            PackedVector3Array group_centres;
            PackedInt32Array group_ends;
    };

    struct ShapeVertex {
            Vector3 position;
            Vector3 normal;
            Vector2 uv;
    };

    /// One shape as the file holds it, in world space
    struct Shape {
            String texture;
            float range_min = 0.0F;
            float range_max = 0.0F;
            /// What the original measures the shape's range from (bounding_area, scenenode.cpp:41)
            Vector3 centre;
            std::vector<ShapeVertex> vertices;
    };

    static PackedFloat32Array to_packed(const std::vector<float> &p_values) {
        PackedFloat32Array packed;
        packed.resize(static_cast<int64_t>(p_values.size()));
        std::copy(p_values.begin(), p_values.end(), packed.ptrw());
        return packed;
    }

    /// shape_node::deserialize() (scenenode.cpp:139, :99); false for a shape not drawn
    static bool read_shape(SectionCursor &p_cursor, Shape &p_shape) {
        p_cursor.string(); // name
        p_shape.centre = p_cursor.dvec3();
        p_cursor.f32(); // radius
        const double range_squared_min = p_cursor.f64();
        const double range_squared_max = p_cursor.f64();
        const bool visible = p_cursor.boolean();
        p_cursor.boolean(); // translucent - the material says it
        const bool has_userdata = p_cursor.boolean();
        p_shape.texture = p_cursor.string();
        p_cursor.skip(LIGHTING_BYTES);
        const Vector3 origin = p_cursor.dvec3();
        const uint32_t vertex_count = p_cursor.u32();
        p_shape.vertices.clear();
        p_shape.vertices.reserve(vertex_count);
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
            p_shape.vertices.push_back(vertex);
        }
        if (p_cursor.overrun || !visible || p_shape.vertices.empty()) {
            return false;
        }
        p_shape.texture = p_shape.texture.trim_prefix(TEXTURES_PREFIX);
        p_shape.range_min = static_cast<float>(Math::sqrt(range_squared_min));
        p_shape.range_max =
                range_squared_max >= NO_RANGE_SQUARED ? NO_RANGE : static_cast<float>(Math::sqrt(range_squared_max));
        return true;
    }

    /// The shape's triangles relative to p_origin, turned around as MaszynaTrianglesImporter turns a
    /// "triangles" node
    static void append_shape(ShapeGeometry &p_geometry, const Shape &p_shape, const Vector3 &p_origin) {
        for (auto vertex = p_shape.vertices.rbegin(); vertex != p_shape.vertices.rend(); ++vertex) {
            const Vector3 position = vertex->position - p_origin;
            p_geometry.vertices.insert(
                    p_geometry.vertices.end(),
                    {static_cast<float>(position.x), static_cast<float>(position.y), static_cast<float>(position.z)});
            p_geometry.normals.insert(
                    p_geometry.normals.end(),
                    {static_cast<float>(vertex->normal.x), static_cast<float>(vertex->normal.y),
                     static_cast<float>(vertex->normal.z)});
            p_geometry.uvs.insert(
                    p_geometry.uvs.end(), {static_cast<float>(vertex->uv.x), static_cast<float>(vertex->uv.y)});
        }
    }

    static Ref<MaszynaTrianglesChunkGeometry> to_geometry(
            const String &p_texture, const Vector3 &p_origin, const float p_range_min, const float p_range_max,
            const ShapeGeometry &p_geometry) {
        Ref<MaszynaTrianglesChunkGeometry> geometry;
        geometry.instantiate();
        geometry->set_texture(p_texture);
        geometry->set_origin(p_origin);
        geometry->set_range_min(p_range_min);
        geometry->set_range_max(p_range_max);
        geometry->set_vertices(to_packed(p_geometry.vertices));
        geometry->set_normals(to_packed(p_geometry.normals));
        geometry->set_uvs(to_packed(p_geometry.uvs));
        geometry->set_group_centres(p_geometry.group_centres);
        geometry->set_group_ends(p_geometry.group_ends);
        return geometry;
    }

    /// The shape appended as a group of its own, its centre relative to p_origin
    static void append_group(ShapeGeometry &p_geometry, const Shape &p_shape, const Vector3 &p_origin) {
        append_shape(p_geometry, p_shape, p_origin);
        p_geometry.group_centres.push_back(p_shape.centre - p_origin);
        p_geometry.group_ends.push_back(
                static_cast<int32_t>(p_geometry.vertices.size() / MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS));
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
    /// section, drawn as the original draws it:
    /// - the section's own shapes merged by texture, their range dropped - the original draws them
    ///   whenever the section is in view (Render(shape, true), opengl33renderer.cpp:2495);
    /// - a cell's shapes, merged already when the file was written (basic_cell::insert(),
    ///   scene.cpp:282-291), in one chunk per texture and range around the cell's centre, each shape a
    ///   group of it whose range is measured from its own centre (opengl33renderer.cpp:2814)
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
        const Vector3 section_origin = SceneryTrianglesSink::cell_get_origin(p_cell);
        std::vector<String> section_textures;
        HashMap<String, ShapeGeometry> section_geometries;
        Shape shape;
        cursor.skip(AREA_BYTES);
        for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
            if (!read_shape(cursor, shape)) {
                continue;
            }
            if (!section_geometries.has(shape.texture)) {
                section_textures.push_back(shape.texture);
            }
            append_shape(section_geometries[shape.texture], shape, section_origin);
        }
        for (int cell = 0; cell < SECTION_CELL_COUNT && !cursor.overrun; cell++) {
            // a cell's shapes by texture and range - one chunk each - around the cell's centre
            const Vector3 cell_origin = cursor.dvec3();
            cursor.f32(); // radius
            std::vector<String> cell_keys;
            HashMap<String, ShapeGeometry> cell_geometries;
            HashMap<String, Shape> cell_shapes; // the texture and range of each key, without its vertices
            // opaque shapes, then translucent ones
            for (int kind = 0; kind < 2; kind++) {
                for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
                    if (!read_shape(cursor, shape)) {
                        continue;
                    }
                    const String key =
                            shape.texture + "|" + String::num(shape.range_min) + "|" + String::num(shape.range_max);
                    if (!cell_geometries.has(key)) {
                        cell_keys.push_back(key);
                        Shape &described = cell_shapes[key];
                        described.texture = shape.texture;
                        described.range_min = shape.range_min;
                        described.range_max = shape.range_max;
                    }
                    append_group(cell_geometries[key], shape, cell_origin);
                }
            }
            for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
                skip_lines(cursor);
            }
            for (const String &key: cell_keys) {
                const Shape &first = cell_shapes[key];
                geometries.append_array(
                        to_geometry(first.texture, cell_origin, first.range_min, first.range_max, cell_geometries[key])
                                ->split());
            }
        }
        ERR_FAIL_COND_V_MSG(cursor.overrun, Array(), "Bad file: a region section ends early");
        for (const String &texture: section_textures) {
            ShapeGeometry &geometry = section_geometries[texture];
            // a section's texture is one group, drawn whatever its range (Render(shape, true),
            // opengl33renderer.cpp:2495)
            geometry.group_centres.push_back(Vector3());
            geometry.group_ends.push_back(
                    static_cast<int32_t>(geometry.vertices.size() / MaszynaTrianglesChunkGeometry::VECTOR3_FLOATS));
            geometries.append_array(to_geometry(texture, section_origin, 0.0F, NO_RANGE, geometry)->split());
        }
        return geometries;
    }
} // namespace godot
