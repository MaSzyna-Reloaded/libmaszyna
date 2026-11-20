#include "maszyna/scenery/MaszynaBTFTerrainProvider.hpp"

#include "maszyna/scenery/MaszynaTerrainShapes.hpp"
#include "maszyna/scenery/MaszynaTrianglesSink.hpp"

#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/packed_byte_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <vector>

namespace godot {
    namespace {
        /// The cells of a section along its side (EU07_SECTIONSIZE / EU07_CELLSIZE, scene.h:34-35)
        constexpr int SECTION_SIDE_CELL_COUNT =
                static_cast<int>(MaszynaBTFTerrainProvider::SECTION_SIZE_M / MaszynaBTFTerrainProvider::CELL_SIZE_M);
        /// A shape's colours: diffuse, ambient and specular vec4 (terrain_file::write(),
        /// sceneterrain.cpp:548)
        constexpr int64_t LIGHTING_BYTES = static_cast<int64_t>(3 * 4) * sizeof(float);
    } // namespace

    /// The header up to the material names (read_header(), sceneterrain.cpp:221): the text's
    /// length, or -1 when the file is no terrain file of the version and layout written
    static int64_t read_header(const Ref<FileAccess> &p_file, uint32_t &p_material_count, uint32_t &p_section_count) {
        if (p_file->get_32() != MaszynaBTFTerrainProvider::FILE_HEADER ||
            p_file->get_32() != MaszynaBTFTerrainProvider::FILE_VERSION) {
            return -1;
        }
        const int64_t source_length = static_cast<int64_t>(p_file->get_64());
        p_file->get_32(); // the text's checksum
        // the content is arranged for the layout of the original's scene, and fits no other
        if (p_file->get_32() != MaszynaBTFTerrainProvider::SECTION_SIZE_M ||
            p_file->get_32() != MaszynaBTFTerrainProvider::CELL_SIZE_M ||
            p_file->get_32() != MaszynaBTFTerrainProvider::REGION_SIDE_SECTION_COUNT) {
            return -1;
        }
        p_material_count = p_file->get_32();
        p_section_count = p_file->get_32();
        p_file->get_64(); // vertex count
        p_file->get_64(); // the offset of the text that is no geometry ("extra")
        p_file->get_32(); // and its size
        return p_file->eof_reached() ? -1 : source_length;
    }

    /// A name up to its zero, as MaszynaSectionCursor::string() reads one
    static String read_name(const Ref<FileAccess> &p_file) {
        String name;
        for (uint8_t byte = p_file->get_8(); byte != 0 && !p_file->eof_reached(); byte = p_file->get_8()) {
            name += static_cast<char32_t>(byte);
        }
        return name;
    }

    /// The original's section and its centre, by the section's index (terrain_file::attach(),
    /// sceneterrain.cpp:1000) - the centre is MaszynaTrianglesSink::cell_get_origin() of the cell
    static Vector2i section_cell(const uint32_t p_index) {
        constexpr int HALF = static_cast<int>(MaszynaBTFTerrainProvider::REGION_SIDE_SECTION_COUNT / 2);
        return {static_cast<int>(p_index % MaszynaBTFTerrainProvider::REGION_SIDE_SECTION_COUNT) - HALF,
                static_cast<int>(p_index / MaszynaBTFTerrainProvider::REGION_SIDE_SECTION_COUNT) - HALF};
    }

    /// The centre of a cell of a section, by the cell's index (basic_section::center(),
    /// scene.cpp:996)
    static Vector3 cell_centre(const Vector3 &p_section_centre, const uint32_t p_cell) {
        constexpr double CORNER =
                (-0.5 * MaszynaBTFTerrainProvider::SECTION_SIZE_M) + (0.5 * MaszynaBTFTerrainProvider::CELL_SIZE_M);
        // cells run along rows of the section (basic_section::center())
        const uint32_t column = p_cell % static_cast<uint32_t>(SECTION_SIDE_CELL_COUNT);
        const uint32_t row = p_cell / static_cast<uint32_t>(SECTION_SIDE_CELL_COUNT);
        return p_section_centre +
               Vector3(static_cast<real_t>(CORNER + (column * MaszynaBTFTerrainProvider::CELL_SIZE_M)), 0.0,
                       static_cast<real_t>(CORNER + (row * MaszynaBTFTerrainProvider::CELL_SIZE_M)));
    }

    void MaszynaBTFTerrainProvider::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaBTFTerrainProvider", D_METHOD("get_source_length", "path"),
                &MaszynaBTFTerrainProvider::get_source_length);
        ClassDB::bind_method(D_METHOD("open", "path", "translucency"), &MaszynaBTFTerrainProvider::open);
    }

    int64_t MaszynaBTFTerrainProvider::get_source_length(const String &p_path) {
        const Ref<FileAccess> terrain = FileAccess::open(p_path, FileAccess::READ);
        if (terrain.is_null()) {
            return -1;
        }
        uint32_t material_count = 0;
        uint32_t section_count = 0;
        return read_header(terrain, material_count, section_count);
    }

    /// The file's materials and sections are listed, not read: their index, place, size and reach
    /// (terrain_file::attach(), sceneterrain.cpp:973)
    bool MaszynaBTFTerrainProvider::open(const String &p_path, const Callable &p_translucency) {
        MutexLock lock(mutex);
        sections.clear();
        materials.clear();
        translucency = p_translucency;
        file = FileAccess::open(p_path, FileAccess::READ);
        if (file.is_null()) {
            return false;
        }
        uint32_t material_count = 0;
        uint32_t section_count = 0;
        if (read_header(file, material_count, section_count) < 0) {
            UtilityFunctions::push_error("Bad file: \"" + p_path + "\" is of either unrecognized type or version");
            file.unref();
            return false;
        }
        for (uint32_t index = 0; index < material_count && !file->eof_reached(); index++) {
            materials.push_back(read_name(file));
        }
        // the radius a section starts with (scene.h:353); one grown past it holds a shape reaching out
        const double section_radius = 0.5 * Math::sqrt(2.0) * MaszynaTrianglesSink::CHUNK_SIZE_M;
        for (uint32_t entry = 0; entry < section_count && !file->eof_reached(); entry++) {
            const uint32_t index = file->get_32();
            Section section;
            section.offset = file->get_64();
            section.size = static_cast<int64_t>(file->get_32());
            section.overhang = MAX(0.0F, static_cast<float>(file->get_float() - section_radius));
            sections[section_cell(index)] = section;
        }
        if (file->eof_reached() || static_cast<uint32_t>(materials.size()) != material_count) {
            UtilityFunctions::push_error("Bad file: \"" + p_path + "\" is damaged, part of its content may be missing");
        }
        return true;
    }

    TypedArray<Vector2i> MaszynaBTFTerrainProvider::get_chunk_cells() const {
        TypedArray<Vector2i> cells;
        MutexLock lock(mutex);
        for (const KeyValue<Vector2i, Section> &item: sections) {
            cells.append(item.key);
        }
        return cells;
    }

    float MaszynaBTFTerrainProvider::chunk_get_overhang(const Vector2i &p_cell) const {
        MutexLock lock(mutex);
        const Section *section = sections.getptr(p_cell);
        return section != nullptr ? section->overhang : 0.0F;
    }

    PackedInt32Array MaszynaBTFTerrainProvider::get_content_kinds() const {
        PackedInt32Array kinds;
        kinds.push_back(CONTENT_TERRAIN);
        return kinds;
    }

    /// terrain_file::load() (sceneterrain.cpp:1040) of one section, drawn as the original draws it:
    /// - a translucent shape, or one seen only within a range, in one chunk per texture and range
    ///   around its cell's centre, each shape a group of it whose range is measured from its own
    ///   centre (opengl33renderer.cpp:2814);
    /// - the other shapes merged by texture over the whole section, drawn whenever the section is in
    ///   view (Render(shape, true), opengl33renderer.cpp:2495)
    Array MaszynaBTFTerrainProvider::chunk_load(const Vector2i &p_cell, const ContentKind p_kind) {
        Array geometries;
        if (p_kind != CONTENT_TERRAIN) {
            return geometries;
        }
        PackedByteArray data;
        PackedStringArray shape_materials;
        Callable is_translucent;
        {
            MutexLock lock(mutex);
            const Section *section = sections.getptr(p_cell);
            if (section == nullptr || file.is_null()) {
                return geometries;
            }
            file->seek(section->offset);
            data = file->get_buffer(section->size);
            shape_materials = materials;
            is_translucent = translucency;
        }
        MaszynaSectionCursor cursor(data);
        ERR_FAIL_COND_V_MSG(
                cursor.u32() != SECTION_BLOCK, geometries,
                "Bad file: a terrain section is not where its directory says");
        cursor.u32(); // the section's index
        const Vector3 section_origin = MaszynaTrianglesSink::cell_get_origin(p_cell);

        std::vector<String> section_textures;
        HashMap<String, MaszynaTerrainGeometry> section_geometries;
        std::vector<String> cell_keys;
        HashMap<String, MaszynaTerrainGeometry> cell_geometries;
        HashMap<String, MaszynaTerrainShape> cell_shapes; // the texture, range and cell of each key, without vertices
        HashMap<String, Vector3> cell_origins;
        HashMap<String, bool> translucent_materials;
        MaszynaTerrainShape shape;
        for (uint32_t count = cursor.u32(); count > 0 && !cursor.overrun; count--) {
            const uint32_t material = cursor.u32();
            const uint32_t cell = cursor.u32();
            const double range_squared_min = cursor.f64();
            const double range_squared_max = cursor.f64();
            cursor.skip(LIGHTING_BYTES);
            shape.centre = cursor.dvec3();
            cursor.f32(); // radius
            const uint32_t vertex_count = cursor.u32();
            shape.vertices.clear();
            shape.vertices.reserve(vertex_count);
            for (uint32_t index = 0; index < vertex_count && !cursor.overrun; index++) {
                MaszynaTerrainVertex vertex;
                vertex.position = section_origin + cursor.vec3();
                vertex.normal = cursor.vec3();
                const float u = cursor.f32();
                const float v = cursor.f32();
                vertex.uv = Vector2(u, v);
                shape.vertices.push_back(vertex);
            }
            if (cursor.overrun || material >= static_cast<uint32_t>(shape_materials.size()) ||
                cell >= static_cast<uint32_t>(SECTION_SIDE_CELL_COUNT * SECTION_SIDE_CELL_COUNT)) {
                cursor.overrun = true;
                break;
            }
            if (shape.vertices.empty()) {
                continue;
            }
            shape.texture = shape_materials[material];
            shape.set_ranges_squared(range_squared_min, range_squared_max);
            if (!translucent_materials.has(shape.texture)) {
                translucent_materials[shape.texture] = bool(is_translucent.call(shape.texture));
            }
            if (!MaszynaTrianglesSink::shape_is_ranged(
                        translucent_materials[shape.texture], shape.range_min, shape.range_max)) {
                if (!section_geometries.has(shape.texture)) {
                    section_textures.push_back(shape.texture);
                }
                section_geometries[shape.texture].append_shape(shape, section_origin);
                continue;
            }
            const String key = String::num_int64(cell) + "|" + shape.texture + "|" + String::num(shape.range_min) +
                               "|" + String::num(shape.range_max);
            if (!cell_geometries.has(key)) {
                cell_keys.push_back(key);
                MaszynaTerrainShape &described = cell_shapes[key];
                described.texture = shape.texture;
                described.range_min = shape.range_min;
                described.range_max = shape.range_max;
                cell_origins[key] = cell_centre(section_origin, cell);
            }
            cell_geometries[key].append_group(shape, cell_origins[key]);
        }
        ERR_FAIL_COND_V_MSG(cursor.overrun, Array(), "Bad file: a terrain section is damaged");
        for (const String &key: cell_keys) {
            const MaszynaTerrainShape &described = cell_shapes[key];
            geometries.append_array(
                    cell_geometries[key]
                            .to_chunk_geometry(
                                    described.texture, cell_origins[key], described.range_min, described.range_max)
                            ->split());
        }
        for (const String &texture: section_textures) {
            MaszynaTerrainGeometry &geometry = section_geometries[texture];
            geometry.close_group();
            geometries.append_array(
                    geometry.to_chunk_geometry(texture, section_origin, 0.0F, MaszynaTerrainShape::NO_RANGE)->split());
        }
        return geometries;
    }
} // namespace godot
