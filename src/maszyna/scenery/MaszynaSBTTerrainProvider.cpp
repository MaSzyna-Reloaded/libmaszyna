#include "maszyna/scenery/MaszynaSBTTerrainProvider.hpp"

#include "maszyna/scenery/MaszynaTerrainShapes.hpp"
#include "maszyna/scenery/MaszynaTrianglesSink.hpp"
#include "scenery/SceneryLinesGeometry.hpp"

#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/color.hpp>
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

        /// bounding_area::serialize(): its centre and radius (scenenode.cpp:41)
        constexpr int64_t AREA_BYTES = (3 * sizeof(double)) + sizeof(float);
        /// lighting_data::serialize(): diffuse, ambient and specular vec4 (scenenode.cpp:21)
        constexpr int64_t LIGHTING_BYTES = static_cast<int64_t>(3 * 4) * sizeof(float);
        /// basic_vertex::serialize() without a tangent: position, normal, uv (geometrybank.cpp:41)
        constexpr int64_t VERTEX_BYTES = 8 * sizeof(float);
        /// vertex_userdata::serialize(): one vec4 (geometrybank.cpp:460)
        constexpr int64_t USERDATA_BYTES = 4 * sizeof(float);
    } // namespace

    /// shape_node::deserialize() (scenenode.cpp:139, :99); false for a shape not drawn
    static bool read_shape(MaszynaSectionCursor &p_cursor, MaszynaTerrainShape &p_shape) {
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
            MaszynaTerrainVertex vertex;
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
        p_shape.set_ranges_squared(range_squared_min, range_squared_max);
        return true;
    }

    /// shape_node::deserialize() (scenenode.cpp:139, :99) read past - what read_shape() reads
    static void skip_shape(MaszynaSectionCursor &p_cursor) {
        p_cursor.string(); // name
        // area, range min and max, visible, translucent
        p_cursor.skip(AREA_BYTES + (2 * sizeof(double)) + (2 * sizeof(uint16_t)));
        const bool has_userdata = p_cursor.boolean();
        p_cursor.string();                                    // texture
        p_cursor.skip(LIGHTING_BYTES + (3 * sizeof(double))); // lighting, origin
        p_cursor.skip(static_cast<int64_t>(p_cursor.u32()) * (VERTEX_BYTES + (has_userdata ? USERDATA_BYTES : 0)));
    }

    /// lines_node::deserialize() (scenenode.cpp:550, :513) read past, by the terrain
    static void skip_lines(MaszynaSectionCursor &p_cursor) {
        p_cursor.string(); // name
        // area, range min and max, visible, line width, lighting, origin
        p_cursor.skip(
                AREA_BYTES + (2 * sizeof(double)) + sizeof(uint16_t) + sizeof(float) + LIGHTING_BYTES +
                (3 * sizeof(double)));
        p_cursor.skip(static_cast<int64_t>(p_cursor.u32()) * VERTEX_BYTES);
    }

    /// lines_node::deserialize() (scenenode.cpp:550, :513): a node as the original merged it in its
    /// cell, its lines added to p_geometry; a node not drawn is left out
    static void read_lines(MaszynaSectionCursor &p_cursor, const Ref<SceneryLinesGeometry> &p_geometry) {
        p_cursor.string();         // name
        p_cursor.skip(AREA_BYTES); // the points give it again
        const double range_squared_min = p_cursor.f64();
        const double range_squared_max = p_cursor.f64();
        const bool visible = p_cursor.boolean();
        const float width = p_cursor.f32();
        const float red = p_cursor.f32();
        const float green = p_cursor.f32();
        const float blue = p_cursor.f32();
        p_cursor.f32(); // the diffuse's alpha, 1 (lines_node::import(), scenenode.cpp:578)
        p_cursor.skip(LIGHTING_BYTES - (4 * sizeof(float))); // ambient, specular
        const Vector3 origin = p_cursor.dvec3();
        PackedVector3Array points;
        for (uint32_t count = p_cursor.u32(); count > 0 && !p_cursor.overrun; count--) {
            points.push_back(origin + p_cursor.vec3());
            p_cursor.skip(VERTEX_BYTES - (3 * sizeof(float))); // normal, uv
        }
        if (p_cursor.overrun || !visible) {
            return;
        }
        p_geometry->add_lines(
                points, Color(red, green, blue), width, static_cast<float>(Math::sqrt(range_squared_min)),
                range_squared_max >= MaszynaTerrainShape::NO_RANGE_SQUARED
                        ? 0.0F
                        : static_cast<float>(Math::sqrt(range_squared_max)));
    }

    /// The lines of a section's cells (basic_cell::deserialize(), scene.cpp:227), past the shapes:
    /// one geometry around the section's centre
    static Array read_section_lines(MaszynaSectionCursor &p_cursor, const Vector2i &p_cell) {
        Ref<SceneryLinesGeometry> geometry;
        geometry.instantiate();
        geometry->set_origin(MaszynaTrianglesSink::cell_get_origin(p_cell));
        p_cursor.skip(AREA_BYTES);
        for (uint32_t count = p_cursor.u32(); count > 0 && !p_cursor.overrun; count--) {
            skip_shape(p_cursor);
        }
        for (int cell = 0; cell < MaszynaSBTTerrainProvider::SECTION_CELL_COUNT && !p_cursor.overrun; cell++) {
            p_cursor.skip(AREA_BYTES);
            // opaque shapes, then translucent ones
            for (int kind = 0; kind < 2; kind++) {
                for (uint32_t count = p_cursor.u32(); count > 0 && !p_cursor.overrun; count--) {
                    skip_shape(p_cursor);
                }
            }
            for (uint32_t count = p_cursor.u32(); count > 0 && !p_cursor.overrun; count--) {
                read_lines(p_cursor, geometry);
            }
        }
        ERR_FAIL_COND_V_MSG(p_cursor.overrun, Array(), "Bad file: a region section ends early");
        Array geometries;
        if (geometry->get_node_count() > 0) {
            geometries.push_back(geometry);
        }
        return geometries;
    }
    void MaszynaSBTTerrainProvider::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaSBTTerrainProvider", D_METHOD("is_region", "path"), &MaszynaSBTTerrainProvider::is_region);
        ClassDB::bind_method(D_METHOD("open", "path"), &MaszynaSBTTerrainProvider::open);
    }

    bool MaszynaSBTTerrainProvider::is_region(const String &p_path) {
        const Ref<FileAccess> region = FileAccess::open(p_path, FileAccess::READ);
        if (region.is_null()) {
            return false;
        }
        const uint32_t header = region->get_32();
        const uint32_t version = region->get_32();
        return header == FILE_HEADER && version == FILE_VERSION;
    }

    /// The file's sections are listed, not read: their index, place, size and reach (scene.cpp:1213-1220)
    bool MaszynaSBTTerrainProvider::open(const String &p_path) {
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
        const double section_radius = 0.5 * Math::sqrt(2.0) * MaszynaTrianglesSink::CHUNK_SIZE_M;
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

    TypedArray<Vector2i> MaszynaSBTTerrainProvider::get_chunk_cells() const {
        TypedArray<Vector2i> cells;
        MutexLock lock(mutex);
        for (const KeyValue<Vector2i, Section> &item: sections) {
            cells.append(item.key);
        }
        return cells;
    }

    float MaszynaSBTTerrainProvider::chunk_get_overhang(const Vector2i &p_cell) const {
        MutexLock lock(mutex);
        const Section *section = sections.getptr(p_cell);
        return section != nullptr ? section->overhang : 0.0F;
    }

    PackedInt32Array MaszynaSBTTerrainProvider::get_content_kinds() const {
        PackedInt32Array kinds;
        kinds.push_back(CONTENT_TERRAIN);
        kinds.push_back(CONTENT_LINES);
        return kinds;
    }

    /// basic_section::deserialize() (scene.cpp:811) and basic_cell::deserialize() (scene.cpp:227) of one
    /// section, drawn as the original draws it:
    /// - the section's own shapes merged by texture, their range dropped - the original draws them
    ///   whenever the section is in view (Render(shape, true), opengl33renderer.cpp:2495);
    /// - a cell's shapes, merged already when the file was written (basic_cell::insert(),
    ///   scene.cpp:282-291), in one chunk per texture and range around the cell's centre, each shape a
    ///   group of it whose range is measured from its own centre (opengl33renderer.cpp:2814)
    Array MaszynaSBTTerrainProvider::chunk_load(const Vector2i &p_cell, const ContentKind p_kind) {
        Array geometries;
        if (p_kind != CONTENT_TERRAIN && p_kind != CONTENT_LINES) {
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
        MaszynaSectionCursor cursor(data);
        if (p_kind == CONTENT_LINES) {
            return read_section_lines(cursor, p_cell);
        }
        const Vector3 section_origin = MaszynaTrianglesSink::cell_get_origin(p_cell);
        std::vector<String> section_textures;
        HashMap<String, MaszynaTerrainGeometry> section_geometries;
        MaszynaTerrainShape shape;
        cursor.skip(AREA_BYTES);
        for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
            if (!read_shape(cursor, shape)) {
                continue;
            }
            if (!section_geometries.has(shape.texture)) {
                section_textures.push_back(shape.texture);
            }
            section_geometries[shape.texture].append_shape(shape, section_origin);
        }
        for (int cell = 0; cell < SECTION_CELL_COUNT && !cursor.overrun; cell++) {
            // a cell's shapes by texture and range - one chunk each - around the cell's centre
            const Vector3 cell_origin = cursor.dvec3();
            cursor.f32(); // radius
            std::vector<String> cell_keys;
            HashMap<String, MaszynaTerrainGeometry> cell_geometries;
            HashMap<String, MaszynaTerrainShape> cell_shapes; // the texture and range of each key, without its vertices
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
                        MaszynaTerrainShape &described = cell_shapes[key];
                        described.texture = shape.texture;
                        described.range_min = shape.range_min;
                        described.range_max = shape.range_max;
                    }
                    cell_geometries[key].append_group(shape, cell_origin);
                }
            }
            for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
                skip_lines(cursor);
            }
            for (const String &key: cell_keys) {
                const MaszynaTerrainShape &first = cell_shapes[key];
                geometries.append_array(
                        cell_geometries[key]
                                .to_chunk_geometry(first.texture, cell_origin, first.range_min, first.range_max)
                                ->split());
            }
        }
        ERR_FAIL_COND_V_MSG(cursor.overrun, Array(), "Bad file: a region section ends early");
        for (const String &texture: section_textures) {
            MaszynaTerrainGeometry &geometry = section_geometries[texture];
            geometry.close_group();
            geometries.append_array(
                    geometry.to_chunk_geometry(texture, section_origin, 0.0F, MaszynaTerrainShape::NO_RANGE)->split());
        }
        return geometries;
    }

} // namespace godot
