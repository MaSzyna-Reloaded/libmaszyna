#include "maszyna/scenery/MaszynaBTFTerrainWriter.hpp"

#include "maszyna/parsers/maszyna_parser.hpp"
#include "maszyna/scenery/MaszynaBTFTerrainProvider.hpp"

#include <godot_cpp/classes/dir_access.hpp>
#include <godot_cpp/classes/file_access.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <array>
#include <cfloat>
#include <cmath>
#include <deque>
#include <map>
#include <vector>

namespace godot {
    namespace {
        constexpr double SECTION_SIZE_M = MaszynaBTFTerrainProvider::SECTION_SIZE_M;
        constexpr double CELL_SIZE_M = MaszynaBTFTerrainProvider::CELL_SIZE_M;
        constexpr int REGION_SIDE_SECTION_COUNT =
                static_cast<int>(MaszynaBTFTerrainProvider::REGION_SIDE_SECTION_COUNT);
        constexpr int SECTION_SIDE_CELL_COUNT = static_cast<int>(SECTION_SIZE_M / CELL_SIZE_M);
        constexpr size_t SECTION_CELL_COUNT = static_cast<size_t>(SECTION_SIDE_CELL_COUNT) * SECTION_SIDE_CELL_COUNT;
        /// The original halves its counts in integers (scene.cpp:996, :1594, :1780)
        constexpr int HALF_SECTION_SIDE_CELLS = SECTION_SIDE_CELL_COUNT / 2;
        constexpr int HALF_REGION_SIDE_SECTIONS = REGION_SIDE_SECTION_COUNT / 2;
        /// basic_region::RaTriangleDivider() (scene.cpp:1608): a triangle reaching no more than this
        /// out of its section is left whole [m]
        constexpr double DIVIDER_MARGIN_M = 200.0;
        /// basic_cell::insert() (scene.cpp:285): shapes merge with one whose centre is closer [m]
        constexpr double MERGE_DISTANCE_M = 25.0;
        /// The colours of the material block are 0-255 (shape_node::import(), scenenode.cpp:165)
        constexpr float COLOUR_SCALE = 255.0F;
        /// lighting_data (scenenode.h:22-24): diffuse, ambient and specular, each RGBA
        constexpr int LIGHTING_FLOATS = 12;
        constexpr std::array<float, LIGHTING_FLOATS> DEFAULT_LIGHTING = {0.8F, 0.8F, 0.8F, 1.0F, 0.2F, 0.2F,
                                                                         0.2F, 1.0F, 0.0F, 0.0F, 0.0F, 1.0F};
        constexpr int DIFFUSE_AT = 0;
        constexpr int AMBIENT_AT = 4;
        constexpr int SPECULAR_AT = 8;
        /// The material names the original's material manager looks up by keep these prefixes
        /// whole (conversion_renderer::Fetch_Material(), sceneterrain.cpp:266)
        constexpr const char *MADE_MATERIAL_PREFIX = "make:";
        constexpr const char *INTERNAL_MATERIAL_PREFIX = "internal_src:";
        /// A vertex of a "triangles" node: position, normal and texture coordinates
        constexpr int VERTEX_TOKENS = 8;
        /// Half the diagonal of a square of side 1: a section's bounding radius to begin with
        constexpr double HALF_DIAGONAL = 0.70710678118654752440;

        struct DVec3 {
                double x = 0.0;
                double y = 0.0;
                double z = 0.0;
        };

        /// world_vertex (vertex.h): its position in double precision, the rest in single
        struct Vertex {
                DVec3 position;
                std::array<float, 3> normal{};
                std::array<float, 2> uv{};
        };

        struct Shape {
                uint32_t material = 0; // 1 the first, as the original's handles
                std::array<float, LIGHTING_FLOATS> lighting = DEFAULT_LIGHTING;
                double range_squared_min = 0.0;
                double range_squared_max = DBL_MAX;
                std::vector<Vertex> vertices;
                DVec3 centre;
                float radius = -1.0F;
        };

        struct Cell {
                DVec3 centre;
                std::vector<Shape> shapes;
        };

        struct Section {
                DVec3 centre;
                /// bounding_area of basic_section (scene.h:418), grown by its shapes
                float radius = static_cast<float>(HALF_DIAGONAL * SECTION_SIZE_M);
                std::array<Cell, SECTION_CELL_COUNT> cells;
        };
    } // namespace

    static double length_squared(const DVec3 &p_from, const DVec3 &p_to) {
        const double x = p_to.x - p_from.x;
        const double y = p_to.y - p_from.y;
        const double z = p_to.z - p_from.z;
        return (x * x) + (y * y) + (z * z);
    }

    /// shape_node::compute_radius() (scenenode.cpp:466), kept until the shape changes
    static float shape_radius(Shape &p_shape) {
        if (p_shape.radius < 0.0F) {
            double squared = 0.0;
            for (const Vertex &vertex: p_shape.vertices) {
                squared = MAX(squared, length_squared(p_shape.centre, vertex.position));
            }
            p_shape.radius = static_cast<float>(std::sqrt(squared));
        }
        return p_shape.radius;
    }

    /// degenerate() (utilities.h:355): the triangle has no area
    static bool degenerate(const DVec3 &p_a, const DVec3 &p_b, const DVec3 &p_c) {
        const DVec3 ab{p_b.x - p_a.x, p_b.y - p_a.y, p_b.z - p_a.z};
        const DVec3 ac{p_c.x - p_a.x, p_c.y - p_a.y, p_c.z - p_a.z};
        const DVec3 cross{(ab.y * ac.z) - (ab.z * ac.y), (ab.z * ac.x) - (ab.x * ac.z), (ab.x * ac.y) - (ab.y * ac.x)};
        return length_squared(DVec3(), cross) == 0.0;
    }

    /// world_vertex::lerp() (vertex.h:20): a * (1 - factor) + b * factor - the position in double
    /// precision, the normal and the texture coordinates in single, as glm scales them
    static Vertex vertex_lerp(const Vertex &p_a, const Vertex &p_b, const double p_factor) {
        const double rest = 1.0 - p_factor;
        const auto rest_single = static_cast<float>(rest);
        const auto factor_single = static_cast<float>(p_factor);
        Vertex vertex;
        vertex.position = {
                (p_a.position.x * rest) + (p_b.position.x * p_factor),
                (p_a.position.y * rest) + (p_b.position.y * p_factor),
                (p_a.position.z * rest) + (p_b.position.z * p_factor)};
        for (size_t index = 0; index < vertex.normal.size(); index++) {
            vertex.normal[index] = (p_a.normal[index] * rest_single) + (p_b.normal[index] * factor_single);
        }
        for (size_t index = 0; index < vertex.uv.size(); index++) {
            vertex.uv[index] = (p_a.uv[index] * rest_single) + (p_b.uv[index] * factor_single);
        }
        return vertex;
    }

    static DVec3 triangle_centre(const Shape &p_shape) {
        const DVec3 &a = p_shape.vertices[0].position;
        const DVec3 &b = p_shape.vertices[1].position;
        const DVec3 &c = p_shape.vertices[2].position;
        return {(a.x + b.x + c.x) / 3.0, (a.y + b.y + c.y) / 3.0, (a.z + b.z + c.z) / 3.0};
    }

    /// basic_region::RaTriangleDivider() (scene.cpp:1601): a single triangle reaching more than
    /// DIVIDER_MARGIN_M out of its 1 km square is cut in two where one of its sides crosses the
    /// square's edge, the second part added to p_shapes; false when it is left as it is
    static bool divide_triangle(const size_t p_index, std::deque<Shape> &p_shapes) {
        if (p_shapes[p_index].vertices.size() != 3) {
            return false;
        }
        const Shape &shape = p_shapes[p_index];
        double x0 = (SECTION_SIZE_M * std::floor(shape.centre.x / SECTION_SIZE_M)) - DIVIDER_MARGIN_M;
        double x1 = x0 + SECTION_SIZE_M + (DIVIDER_MARGIN_M * 2);
        double z0 = (SECTION_SIZE_M * std::floor(shape.centre.z / SECTION_SIZE_M)) - DIVIDER_MARGIN_M;
        double z1 = z0 + SECTION_SIZE_M + (DIVIDER_MARGIN_M * 2);
        bool inside = true;
        for (const Vertex &vertex: shape.vertices) {
            inside = inside && vertex.position.x >= x0 && vertex.position.x <= x1 && vertex.position.z >= z0 &&
                     vertex.position.z <= z1;
        }
        if (inside) {
            return false;
        }
        x0 += DIVIDER_MARGIN_M;
        x1 -= DIVIDER_MARGIN_M;
        z0 += DIVIDER_MARGIN_M;
        z1 -= DIVIDER_MARGIN_M;
        // the side crossing an edge the most: 0 AB, 1 BC, 2 CA; +4 an edge along x, +8 the far one
        constexpr int ALONG_X = 4;
        constexpr int FAR_EDGE = 8;
        int divide = -1;
        double least = 0.0;
        const auto x = [&shape](const int p_vertex) { return shape.vertices[p_vertex % 3].position.x; };
        const auto z = [&shape](const int p_vertex) { return shape.vertices[p_vertex % 3].position.z; };
        const std::array<double, 4> edges = {x0, x1, z0, z1};
        const std::array<int, 4> kinds = {0, FAR_EDGE, ALONG_X, ALONG_X | FAR_EDGE};
        for (size_t edge = 0; edge < edges.size(); edge++) {
            const bool along_x = (kinds[edge] & ALONG_X) != 0;
            for (int side = 0; side < 3; side++) {
                const double first = along_x ? z(side) : x(side);
                const double second = along_x ? z(side + 1) : x(side + 1);
                const double product = (first - edges[edge]) * (second - edges[edge]);
                if (product < least) {
                    least = product;
                    divide = kinds[edge] | side;
                }
            }
        }
        p_shapes.push_back(p_shapes[p_index]);
        Shape &cut = p_shapes[p_index];
        Shape &added = p_shapes.back();
        const bool far_edge = (divide & FAR_EDGE) != 0;
        double edge = far_edge ? x1 : x0;
        if ((divide & ALONG_X) != 0) {
            edge = far_edge ? z1 : z0;
        }
        const auto split = [&](const Vertex &p_from, const Vertex &p_to) {
            const bool along_x = (divide & ALONG_X) != 0;
            const double from = along_x ? p_from.position.z : p_from.position.x;
            const double to = along_x ? p_to.position.z : p_to.position.x;
            return vertex_lerp(p_from, p_to, (edge - from) / (to - from));
        };
        constexpr int SIDE_MASK = 3;
        switch (divide & SIDE_MASK) {
            case 0: // AB: ADC and DBC
                added.vertices[2] = cut.vertices[2];
                added.vertices[1] = cut.vertices[1];
                cut.vertices[1] = split(cut.vertices[0], cut.vertices[1]);
                added.vertices[0] = cut.vertices[1];
                break;
            case 1: // BC: ABD and ADC
                added.vertices[0] = cut.vertices[0];
                added.vertices[2] = cut.vertices[2];
                cut.vertices[2] = split(cut.vertices[1], cut.vertices[2]);
                added.vertices[1] = cut.vertices[2];
                break;
            case 2: // CA: ABD and DBC
                added.vertices[1] = cut.vertices[1];
                added.vertices[2] = cut.vertices[2];
                cut.vertices[2] = split(cut.vertices[2], cut.vertices[0]);
                added.vertices[0] = cut.vertices[2];
                break;
            default:
                break;
        }
        cut.centre = triangle_centre(cut);
        added.centre = triangle_centre(added);
        return true;
    }

    /// basic_region::section() (scene.cpp:1778): the section holding p_location, made with its
    /// centre and its cells' the first time
    static uint32_t section_index(const DVec3 &p_location, std::map<uint32_t, Section> &p_sections) {
        constexpr int HALF = HALF_REGION_SIDE_SECTIONS;
        const int column = CLAMP(
                static_cast<int>(std::floor((p_location.x / SECTION_SIZE_M) + HALF)), 0, REGION_SIDE_SECTION_COUNT - 1);
        const int row = CLAMP(
                static_cast<int>(std::floor((p_location.z / SECTION_SIZE_M) + HALF)), 0, REGION_SIDE_SECTION_COUNT - 1);
        const auto index = static_cast<uint32_t>((row * REGION_SIDE_SECTION_COUNT) + column);
        if (p_sections.find(index) == p_sections.end()) {
            Section &section = p_sections[index];
            const double corner = (-HALF * SECTION_SIZE_M) + (SECTION_SIZE_M / 2);
            section.centre = {corner + (column * SECTION_SIZE_M), 0.0, corner + (row * SECTION_SIZE_M)};
            // basic_section::center() (scene.cpp:996)
            const double cell_corner = (-HALF_SECTION_SIDE_CELLS * CELL_SIZE_M) + (CELL_SIZE_M / 2);
            for (int cell = 0; cell < static_cast<int>(SECTION_CELL_COUNT); cell++) {
                section.cells[cell].centre = {
                        section.centre.x + cell_corner + ((cell % SECTION_SIDE_CELL_COUNT) * CELL_SIZE_M), 0.0,
                        section.centre.z + cell_corner +
                                (static_cast<int>(cell / SECTION_SIDE_CELL_COUNT) * CELL_SIZE_M)};
            }
        }
        return index;
    }

    /// shape_node::merge() (scenenode.cpp:436): of one look, the centre weighted by the vertices
    static bool merge(Shape &p_target, const Shape &p_shape) {
        if (p_target.material != p_shape.material || p_target.lighting != p_shape.lighting) {
            return false;
        }
        const double weight = static_cast<double>(p_shape.vertices.size()) /
                              static_cast<double>(p_shape.vertices.size() + p_target.vertices.size());
        p_target.centre = {
                Math::lerp(p_target.centre.x, p_shape.centre.x, weight),
                Math::lerp(p_target.centre.y, p_shape.centre.y, weight),
                Math::lerp(p_target.centre.z, p_shape.centre.z, weight)};
        p_target.vertices.insert(p_target.vertices.end(), p_shape.vertices.begin(), p_shape.vertices.end());
        p_target.radius = -1.0F;
        return true;
    }

    /// basic_section::insert() (scene.cpp:884) of a shape the conversion makes translucent - every
    /// shape goes to a cell - and basic_cell::insert() (scene.cpp:264)
    static void section_insert(Section &p_section, Shape &p_shape) {
        p_section.radius =
                MAX(p_section.radius,
                    static_cast<float>(
                            std::sqrt(length_squared(p_section.centre, p_shape.centre)) + shape_radius(p_shape)));
        // basic_section::cell() (scene.cpp:1091)
        const int column =
                CLAMP(static_cast<int>(std::floor(
                              (p_shape.centre.x - (p_section.centre.x - (SECTION_SIZE_M / 2))) / CELL_SIZE_M)),
                      0, SECTION_SIDE_CELL_COUNT - 1);
        const int row =
                CLAMP(static_cast<int>(std::floor(
                              (p_shape.centre.z - (p_section.centre.z - (SECTION_SIZE_M / 2))) / CELL_SIZE_M)),
                      0, SECTION_SIDE_CELL_COUNT - 1);
        Cell &cell = p_section.cells[(row * SECTION_SIDE_CELL_COUNT) + column];
        for (Shape &target: cell.shapes) {
            if (p_shape.range_squared_min == target.range_squared_min &&
                p_shape.range_squared_max == target.range_squared_max &&
                length_squared(p_shape.centre, target.centre) < MERGE_DISTANCE_M * MERGE_DISTANCE_M &&
                merge(target, p_shape)) {
                return;
            }
        }
        cell.shapes.push_back(p_shape);
    }

    /// The name the original's material manager looks a material up by
    /// (conversion_renderer::Fetch_Material(), sceneterrain.cpp:262)
    static String material_name(const String &p_token) {
        String name = p_token.replace("\\", "/");
        if (name.begins_with(MADE_MATERIAL_PREFIX) || name.begins_with(INTERNAL_MATERIAL_PREFIX)) {
            return name;
        }
        if (name.contains("|")) {
            name = name.substr(0, name.find("|"));
        }
        // erase_extension() (utilities.cpp:768): not the dots of a "../"
        const int64_t extension = name.rfind(".");
        if (extension >= 0 && extension != name.rfind("..") + 1) {
            name = name.substr(0, extension);
        }
        while (name.begins_with("/")) {
            name = name.substr(1);
        }
        return name;
    }

    void MaszynaBTFTerrainWriter::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaBTFTerrainWriter", D_METHOD("checksum", "data"), &MaszynaBTFTerrainWriter::checksum);
        ClassDB::bind_static_method(
                "MaszynaBTFTerrainWriter", D_METHOD("write", "text_path", "binary_path"),
                &MaszynaBTFTerrainWriter::write);
    }

    uint32_t MaszynaBTFTerrainWriter::checksum(const PackedByteArray &p_data) {
        constexpr uint32_t POLYNOMIAL = 0xEDB88320U;
        constexpr int TABLE_SIZE = 256;
        constexpr int BYTE_BITS = 8;
        constexpr uint32_t BYTE_MASK = 0xFFU;
        static const std::array<uint32_t, TABLE_SIZE> table = [] {
            std::array<uint32_t, TABLE_SIZE> values{};
            for (uint32_t index = 0; index < TABLE_SIZE; index++) {
                uint32_t value = index;
                for (int bit = 0; bit < BYTE_BITS; bit++) {
                    value = (value & 1U) != 0 ? POLYNOMIAL ^ (value >> 1U) : value >> 1U;
                }
                values[index] = value;
            }
            return values;
        }();
        uint32_t crc = UINT32_MAX;
        for (const uint8_t byte: p_data) {
            crc = table[(crc ^ byte) & BYTE_MASK] ^ (crc >> static_cast<uint32_t>(BYTE_BITS));
        }
        return crc ^ UINT32_MAX;
    }

    bool MaszynaBTFTerrainWriter::write(const String &p_text_path, const String &p_binary_path) {
        const PackedByteArray text = FileAccess::get_file_as_bytes(p_text_path);
        ERR_FAIL_COND_V_MSG(
                text.is_empty() && FileAccess::get_open_error() != OK, false,
                "Bad file: can't read terrain file \"" + p_text_path + "\"");
        const auto fail = [&p_text_path](const String &p_reason) {
            UtilityFunctions::push_error(
                    "Bad file: terrain file \"" + p_text_path + "\" wasn't converted to binary format, " + p_reason);
            return false;
        };

        Ref<MaszynaParser> parser;
        parser.instantiate();
        parser->initialize(text, Array());
        HashMap<String, uint32_t> material_handles;
        PackedStringArray materials;
        std::map<uint32_t, Section> sections;
        for (String token = parser->next_token().to_lower(); !token.is_empty();
             token = parser->next_token().to_lower()) {
            if (token != "node") {
                return fail("\"" + token + "\" can't be a part of binary terrain");
            }
            // node_data, shape_node::import() (scenenode.cpp:146)
            const double range_max = parser->next_token().to_float();
            const double range_min = parser->next_token().to_float();
            const String name = parser->next_token();
            const String type = parser->next_token().to_lower();
            if (type != "triangles") {
                return fail("node of type \"" + type + "\" can't be a part of binary terrain");
            }
            Shape shape;
            shape.range_squared_min = range_min * range_min;
            shape.range_squared_max = range_max >= 0.0 ? range_max * range_max : DBL_MAX;
            token = parser->next_token().to_lower();
            if (token == "material") {
                for (token = parser->next_token().to_lower(); !token.is_empty() && token != "endmaterial";
                     token = parser->next_token().to_lower()) {
                    int at = -1;
                    if (token == "ambient:") {
                        at = AMBIENT_AT;
                    } else if (token == "diffuse:") {
                        at = DIFFUSE_AT;
                    } else if (token == "specular:") {
                        at = SPECULAR_AT;
                    }
                    if (at < 0) {
                        continue;
                    }
                    for (int channel = 0; channel < 3; channel++) {
                        shape.lighting[at + channel] =
                                static_cast<float>(parser->next_token().to_float()) / COLOUR_SCALE;
                    }
                    shape.lighting[at + 3] = 1.0F;
                }
                token = parser->next_token().to_lower();
            }
            const String material = material_name(token);
            if (!material_handles.has(material)) {
                materials.push_back(material);
                material_handles[material] = static_cast<uint32_t>(materials.size());
            }
            shape.material = material_handles[material];

            // the vertices three by three, a triangle without an area left out
            std::array<Vertex, 3> triangle;
            int count = 0;
            for (;;) {
                std::array<double, VERTEX_TOKENS> values{};
                for (double &value: values) {
                    value = parser->next_token().to_float();
                }
                Vertex &vertex = triangle[count];
                vertex.position = {values[0], values[1], values[2]};
                vertex.normal = {
                        static_cast<float>(values[3]), static_cast<float>(values[4]), static_cast<float>(values[5])};
                vertex.uv = {static_cast<float>(values[6]), static_cast<float>(values[7])};
                if (++count == 3) {
                    count = 0;
                    if (degenerate(triangle[0].position, triangle[1].position, triangle[2].position)) {
                        UtilityFunctions::push_warning(
                                "Bad geometry: degenerate triangle encountered in node \"" + name + "\"");
                    } else {
                        shape.vertices.insert(shape.vertices.end(), triangle.begin(), triangle.end());
                    }
                }
                token = parser->next_token().to_lower();
                if (token == "endtri" || token.is_empty()) {
                    break;
                }
            }
            if (shape.vertices.empty()) {
                continue;
            }

            // basic_region::insert() (scene.cpp:1357): the centre of its vertices, cut along the
            // sections, each part in its section
            for (const Vertex &vertex: shape.vertices) {
                shape.centre.x += vertex.position.x;
                shape.centre.y += vertex.position.y;
                shape.centre.z += vertex.position.z;
            }
            const auto vertex_count = static_cast<double>(shape.vertices.size());
            shape.centre = {
                    shape.centre.x / vertex_count, shape.centre.y / vertex_count, shape.centre.z / vertex_count};
            std::deque<Shape> parts{shape};
            for (size_t index = 0; index < parts.size(); index++) {
                while (divide_triangle(index, parts)) {}
            }
            constexpr double BOUNDARY = HALF_REGION_SIDE_SECTIONS * SECTION_SIZE_M;
            for (Shape &part: parts) {
                part.radius = -1.0F;
                if (part.centre.x <= -BOUNDARY || part.centre.x >= BOUNDARY || part.centre.z <= -BOUNDARY ||
                    part.centre.z >= BOUNDARY) {
                    UtilityFunctions::push_error(
                            "Bad scenario: shape node \"" + name + "\" placed in location outside region bounds");
                    continue;
                }
                section_insert(sections[section_index(part.centre, sections)], part);
            }
        }

        // terrain_file::write(): the header, the materials, the directory and the sections
        const String temporary_path = p_binary_path + String(".tmp");
        DirAccess::make_dir_recursive_absolute(p_binary_path.get_base_dir());
        const Ref<FileAccess> file = FileAccess::open(temporary_path, FileAccess::WRITE);
        if (file.is_null()) {
            return fail("can't write to file \"" + temporary_path + "\"");
        }
        file->store_32(MaszynaBTFTerrainProvider::FILE_HEADER);
        file->store_32(MaszynaBTFTerrainProvider::FILE_VERSION);
        file->store_64(static_cast<uint64_t>(text.size()));
        file->store_32(checksum(text));
        file->store_32(MaszynaBTFTerrainProvider::SECTION_SIZE_M);
        file->store_32(MaszynaBTFTerrainProvider::CELL_SIZE_M);
        file->store_32(MaszynaBTFTerrainProvider::REGION_SIDE_SECTION_COUNT);
        file->store_32(static_cast<uint32_t>(materials.size()));
        file->store_32(static_cast<uint32_t>(sections.size()));
        const uint64_t vertex_count_at = file->get_position();
        file->store_64(0); // the vertex count, once known
        const uint64_t extra_at = file->get_position();
        file->store_64(0); // the text that is no geometry, none: its offset once known, its size
        file->store_32(0);
        for (const String &material: materials) {
            file->store_buffer(material.to_ascii_buffer());
            file->store_8(0);
        }
        // a section's index, offset, size and radius, written once the sections are
        const uint64_t directory_at = file->get_position();
        constexpr uint64_t DIRECTORY_ENTRY_BYTES =
                sizeof(uint32_t) + sizeof(uint64_t) + sizeof(uint32_t) + sizeof(float);
        for (size_t entry = 0; entry < sections.size() * DIRECTORY_ENTRY_BYTES; entry++) {
            file->store_8(0);
        }
        std::vector<std::array<uint64_t, 2>> placed; // offset and size of each section
        uint64_t vertex_count = 0;
        for (auto &[index, section]: sections) {
            const uint64_t offset = file->get_position();
            file->store_32(MaszynaBTFTerrainProvider::SECTION_BLOCK);
            file->store_32(index);
            uint32_t shape_count = 0;
            for (const Cell &cell: section.cells) {
                shape_count += static_cast<uint32_t>(cell.shapes.size());
            }
            file->store_32(shape_count);
            for (size_t cell_index = 0; cell_index < section.cells.size(); cell_index++) {
                for (Shape &shape: section.cells[cell_index].shapes) {
                    const float radius = shape_radius(shape);
                    file->store_32(shape.material - 1);
                    file->store_32(static_cast<uint32_t>(cell_index));
                    file->store_double(shape.range_squared_min);
                    file->store_double(shape.range_squared_max);
                    for (const float value: shape.lighting) {
                        file->store_float(value);
                    }
                    file->store_double(shape.centre.x);
                    file->store_double(shape.centre.y);
                    file->store_double(shape.centre.z);
                    file->store_float(radius);
                    file->store_32(static_cast<uint32_t>(shape.vertices.size()));
                    for (const Vertex &vertex: shape.vertices) {
                        file->store_float(static_cast<float>(vertex.position.x - section.centre.x));
                        file->store_float(static_cast<float>(vertex.position.y - section.centre.y));
                        file->store_float(static_cast<float>(vertex.position.z - section.centre.z));
                        for (const float value: vertex.normal) {
                            file->store_float(value);
                        }
                        for (const float value: vertex.uv) {
                            file->store_float(value);
                        }
                    }
                    vertex_count += shape.vertices.size();
                }
            }
            const uint64_t size = file->get_position() - offset;
            if (size > UINT32_MAX) {
                file->close();
                DirAccess::remove_absolute(temporary_path);
                return fail("too much geometry in a single section of the scene");
            }
            placed.push_back({offset, size});
        }
        const uint64_t end = file->get_position();
        file->seek(extra_at);
        file->store_64(end);
        file->seek(directory_at);
        size_t entry = 0;
        for (const auto &[index, section]: sections) {
            file->store_32(index);
            file->store_64(placed[entry][0]);
            file->store_32(static_cast<uint32_t>(placed[entry][1]));
            file->store_float(section.radius);
            entry++;
        }
        file->seek(vertex_count_at);
        file->store_64(vertex_count);
        const Error error = file->get_error();
        file->close();
        // the finished file replaces the previous one in one go
        if (error != OK || DirAccess::rename_absolute(temporary_path, p_binary_path) != OK) {
            DirAccess::remove_absolute(temporary_path);
            return fail("can't write to file \"" + p_binary_path + "\"");
        }
        return true;
    }
} // namespace godot
