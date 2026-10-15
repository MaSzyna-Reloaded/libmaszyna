#include "legacy/scenery/MaszynaLinesGeometry.hpp"

#include "scenery/SceneryTrianglesSink.hpp"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/math.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/packed_color_array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>

#include <array>

namespace godot {
    void MaszynaLinesGeometry::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaLinesGeometry", D_METHOD("position_get_section_origin", "position"),
                &MaszynaLinesGeometry::position_get_section_origin);
        ClassDB::bind_method(D_METHOD("set_origin", "origin"), &MaszynaLinesGeometry::set_origin);
        ClassDB::bind_method(D_METHOD("get_origin"), &MaszynaLinesGeometry::get_origin);
        ClassDB::bind_method(
                D_METHOD("add_lines", "points", "color", "width", "range_min", "range_max"),
                &MaszynaLinesGeometry::add_lines);
        ClassDB::bind_method(D_METHOD("get_node_count"), &MaszynaLinesGeometry::get_node_count);
        ClassDB::bind_method(D_METHOD("build_mesh"), &MaszynaLinesGeometry::build_mesh);
        ClassDB::bind_method(D_METHOD("get_range_end"), &MaszynaLinesGeometry::get_range_end);
        BIND_CONSTANT(QUAD_VERTICES);
        BIND_CONSTANT(QUAD_INDICES);

        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "origin"), "set_origin", "get_origin");
    }

    Vector3 MaszynaLinesGeometry::position_get_section_origin(const Vector3 &p_position) {
        return SceneryTrianglesSink::cell_get_origin(Vector2i(
                static_cast<int>(Math::floor(p_position.x / SECTION_SIZE_M)),
                static_cast<int>(Math::floor(p_position.z / SECTION_SIZE_M))));
    }

    void MaszynaLinesGeometry::set_origin(const Vector3 &p_origin) {
        origin = p_origin;
    }

    Vector3 MaszynaLinesGeometry::get_origin() const {
        return origin;
    }

    /// basic_section::cell() (scene.cpp:1028): a place outside the section is in its nearest cell
    int MaszynaLinesGeometry::_get_cell(const Vector3 &p_position) const {
        constexpr int CELLS_PER_SIDE = static_cast<int>(SECTION_SIZE_M / CELL_SIZE_M);
        const double corner_x = origin.x - (0.5 * SECTION_SIZE_M);
        const double corner_z = origin.z - (0.5 * SECTION_SIZE_M);
        const int column =
                CLAMP(static_cast<int>(Math::floor((p_position.x - corner_x) / CELL_SIZE_M)), 0, CELLS_PER_SIDE - 1);
        const int row =
                CLAMP(static_cast<int>(Math::floor((p_position.z - corner_z) / CELL_SIZE_M)), 0, CELLS_PER_SIDE - 1);
        return (row * CELLS_PER_SIDE) + column;
    }

    void MaszynaLinesGeometry::add_lines(
            const PackedVector3Array &p_points, const Color &p_color, const float p_width, const float p_range_min,
            const float p_range_max) {
        const int64_t count = p_points.size() - (p_points.size() % 2);
        if (count == 0) {
            return;
        }
        Vector3 centre;
        for (int64_t index = 0; index < count; index++) {
            centre += p_points[index];
        }
        centre /= static_cast<double>(count);
        const int cell = _get_cell(centre);

        LinesNode *target = nullptr;
        for (LinesNode &node: nodes) {
            // the ranges and the distance first, then the look (basic_cell::insert(), lines_node::merge())
            if (node.cell == cell && node.range_min == p_range_min && node.range_max == p_range_max &&
                node.centre.distance_squared_to(centre) < MERGE_DISTANCE_M * MERGE_DISTANCE_M &&
                node.width == p_width && node.color == p_color) {
                target = &node;
                break;
            }
        }
        if (target == nullptr) {
            LinesNode &node = nodes.emplace_back();
            node.color = p_color;
            node.width = p_width;
            node.range_min = p_range_min;
            node.range_max = p_range_max;
            node.cell = cell;
            target = &node;
        }
        const double merged = static_cast<double>(target->points.size());
        target->centre =
                target->centre.lerp(centre, static_cast<double>(count) / (merged + static_cast<double>(count)));
        for (int64_t index = 0; index < count; index++) {
            target->points.push_back(p_points[index]);
        }
        double radius_squared = 0.0;
        for (const Vector3 &point: target->points) {
            radius_squared = MAX(radius_squared, point.distance_squared_to(target->centre));
        }
        target->radius = static_cast<float>(Math::sqrt(radius_squared));
    }

    int64_t MaszynaLinesGeometry::get_node_count() const {
        return static_cast<int64_t>(nodes.size());
    }

    Ref<ArrayMesh> MaszynaLinesGeometry::build_mesh() const {
        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        int64_t line_count = 0;
        for (const LinesNode &node: nodes) {
            line_count += static_cast<int64_t>(node.points.size() / 2);
        }
        if (line_count == 0) {
            return mesh;
        }
        constexpr int CUSTOM_FLOATS = 4;
        PackedVector3Array vertices;
        PackedFloat32Array other_ends;
        PackedFloat32Array centres;
        PackedFloat32Array looks;
        PackedColorArray colors;
        PackedInt32Array indices;
        vertices.resize(line_count * QUAD_VERTICES);
        other_ends.resize(line_count * QUAD_VERTICES * CUSTOM_FLOATS);
        centres.resize(line_count * QUAD_VERTICES * CUSTOM_FLOATS);
        looks.resize(line_count * QUAD_VERTICES * CUSTOM_FLOATS);
        colors.resize(line_count * QUAD_VERTICES);
        indices.resize(line_count * QUAD_INDICES);
        // the quad's corners: an end, and the side it is pushed to as seen from that end towards the
        // other - so the second end's sides read the other way round, and the corners go around
        constexpr std::array<int, QUAD_VERTICES> CORNER_ENDS = {0, 0, 1, 1};
        constexpr std::array<float, QUAD_VERTICES> CORNER_SIDES = {-1.0F, 1.0F, -1.0F, 1.0F};
        constexpr std::array<int, QUAD_INDICES> QUAD_CORNERS = {0, 1, 2, 0, 2, 3};
        int64_t vertex = 0;
        int64_t index = 0;
        for (const LinesNode &node: nodes) {
            const Vector3 centre = node.centre - origin;
            for (size_t line = 0; line + 1 < node.points.size(); line += 2) {
                const std::array<Vector3, 2> ends = {node.points[line] - origin, node.points[line + 1] - origin};
                for (const int quad_index: QUAD_CORNERS) {
                    indices.set(index++, static_cast<int32_t>(vertex + quad_index));
                }
                for (int corner = 0; corner < QUAD_VERTICES; corner++) {
                    const Vector3 &end = ends[CORNER_ENDS[corner]];
                    const Vector3 &other = ends[1 - CORNER_ENDS[corner]];
                    const int64_t custom = vertex * CUSTOM_FLOATS;
                    vertices.set(vertex, end);
                    other_ends.set(custom, static_cast<float>(other.x));
                    other_ends.set(custom + 1, static_cast<float>(other.y));
                    other_ends.set(custom + 2, static_cast<float>(other.z));
                    other_ends.set(custom + 3, CORNER_SIDES[corner]);
                    centres.set(custom, static_cast<float>(centre.x));
                    centres.set(custom + 1, static_cast<float>(centre.y));
                    centres.set(custom + 2, static_cast<float>(centre.z));
                    centres.set(custom + 3, node.radius);
                    looks.set(custom, node.width);
                    looks.set(custom + 1, node.range_min);
                    looks.set(custom + 2, node.range_max);
                    looks.set(custom + 3, 0.0F);
                    colors.set(vertex, node.color);
                    vertex++;
                }
            }
        }
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = vertices;
        arrays[Mesh::ARRAY_CUSTOM0] = other_ends;
        arrays[Mesh::ARRAY_CUSTOM1] = centres;
        arrays[Mesh::ARRAY_CUSTOM2] = looks;
        arrays[Mesh::ARRAY_COLOR] = colors;
        arrays[Mesh::ARRAY_INDEX] = indices;
        const BitField<Mesh::ArrayFormat> format = (Mesh::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM0_SHIFT) |
                                                   (Mesh::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM1_SHIFT) |
                                                   (Mesh::ARRAY_CUSTOM_RGBA_FLOAT << Mesh::ARRAY_FORMAT_CUSTOM2_SHIFT);
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays, Array(), Dictionary(), format);
        return mesh;
    }

    /// The mesh is streamed by the centre of its box (SceneryMeshRenderingServer), a node's range is
    /// measured from the node's own centre
    float MaszynaLinesGeometry::get_range_end() const {
        AABB box;
        bool first = true;
        for (const LinesNode &node: nodes) {
            for (const Vector3 &point: node.points) {
                if (first) {
                    box.position = point;
                    first = false;
                } else {
                    box.expand_to(point);
                }
            }
        }
        const Vector3 box_centre = box.get_center();
        float range_end = 0.0F;
        for (const LinesNode &node: nodes) {
            if (node.range_max <= 0.0F) {
                return 0.0F;
            }
            range_end = MAX(range_end, node.range_max + static_cast<float>(node.centre.distance_to(box_centre)));
        }
        return range_end;
    }
} // namespace godot
