#include "legacy/scenery/MaszynaTrianglesChunkGeometry.hpp"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {
    void MaszynaTrianglesChunkGeometry::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_texture", "texture"), &MaszynaTrianglesChunkGeometry::set_texture);
        ClassDB::bind_method(D_METHOD("get_texture"), &MaszynaTrianglesChunkGeometry::get_texture);
        ClassDB::bind_method(D_METHOD("set_origin", "origin"), &MaszynaTrianglesChunkGeometry::set_origin);
        ClassDB::bind_method(D_METHOD("get_origin"), &MaszynaTrianglesChunkGeometry::get_origin);
        ClassDB::bind_method(D_METHOD("set_range_min", "range_min"), &MaszynaTrianglesChunkGeometry::set_range_min);
        ClassDB::bind_method(D_METHOD("get_range_min"), &MaszynaTrianglesChunkGeometry::get_range_min);
        ClassDB::bind_method(D_METHOD("set_range_max", "range_max"), &MaszynaTrianglesChunkGeometry::set_range_max);
        ClassDB::bind_method(D_METHOD("get_range_max"), &MaszynaTrianglesChunkGeometry::get_range_max);
        ClassDB::bind_method(D_METHOD("set_vertices", "vertices"), &MaszynaTrianglesChunkGeometry::set_vertices);
        ClassDB::bind_method(D_METHOD("get_vertices"), &MaszynaTrianglesChunkGeometry::get_vertices);
        ClassDB::bind_method(D_METHOD("set_normals", "normals"), &MaszynaTrianglesChunkGeometry::set_normals);
        ClassDB::bind_method(D_METHOD("get_normals"), &MaszynaTrianglesChunkGeometry::get_normals);
        ClassDB::bind_method(D_METHOD("set_uvs", "uvs"), &MaszynaTrianglesChunkGeometry::set_uvs);
        ClassDB::bind_method(D_METHOD("get_uvs"), &MaszynaTrianglesChunkGeometry::get_uvs);
        ClassDB::bind_method(
                D_METHOD("set_group_centres", "group_centres"), &MaszynaTrianglesChunkGeometry::set_group_centres);
        ClassDB::bind_method(D_METHOD("get_group_centres"), &MaszynaTrianglesChunkGeometry::get_group_centres);
        ClassDB::bind_method(D_METHOD("set_group_ends", "group_ends"), &MaszynaTrianglesChunkGeometry::set_group_ends);
        ClassDB::bind_method(D_METHOD("get_group_ends"), &MaszynaTrianglesChunkGeometry::get_group_ends);
        ClassDB::bind_method(D_METHOD("get_group_count"), &MaszynaTrianglesChunkGeometry::get_group_count);
        ClassDB::bind_method(
                D_METHOD("get_group_mesh_arrays", "group"), &MaszynaTrianglesChunkGeometry::get_group_mesh_arrays);
        ClassDB::bind_method(
                D_METHOD("get_group_visibility_aabb", "group"),
                &MaszynaTrianglesChunkGeometry::get_group_visibility_aabb);
        ClassDB::bind_method(D_METHOD("split"), &MaszynaTrianglesChunkGeometry::split);
        BIND_CONSTANT(MAX_VERTICES);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "texture"), "set_texture", "get_texture");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR3, "origin"), "set_origin", "get_origin");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_min"), "set_range_min", "get_range_min");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_max"), "set_range_max", "get_range_max");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "vertices"), "set_vertices", "get_vertices");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "normals"), "set_normals", "get_normals");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "uvs"), "set_uvs", "get_uvs");
        ADD_PROPERTY(
                PropertyInfo(Variant::PACKED_VECTOR3_ARRAY, "group_centres"), "set_group_centres", "get_group_centres");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_INT32_ARRAY, "group_ends"), "set_group_ends", "get_group_ends");
    }

    void MaszynaTrianglesChunkGeometry::set_texture(const String &p_texture) {
        texture = p_texture;
    }

    String MaszynaTrianglesChunkGeometry::get_texture() const {
        return texture;
    }

    void MaszynaTrianglesChunkGeometry::set_origin(const Vector3 &p_origin) {
        origin = p_origin;
    }

    Vector3 MaszynaTrianglesChunkGeometry::get_origin() const {
        return origin;
    }

    void MaszynaTrianglesChunkGeometry::set_range_min(const float p_range_min) {
        range_min = p_range_min;
    }

    float MaszynaTrianglesChunkGeometry::get_range_min() const {
        return range_min;
    }

    void MaszynaTrianglesChunkGeometry::set_range_max(const float p_range_max) {
        range_max = p_range_max;
    }

    float MaszynaTrianglesChunkGeometry::get_range_max() const {
        return range_max;
    }

    void MaszynaTrianglesChunkGeometry::set_vertices(const PackedFloat32Array &p_vertices) {
        vertices = p_vertices;
    }

    PackedFloat32Array MaszynaTrianglesChunkGeometry::get_vertices() const {
        return vertices;
    }

    void MaszynaTrianglesChunkGeometry::set_normals(const PackedFloat32Array &p_normals) {
        normals = p_normals;
    }

    PackedFloat32Array MaszynaTrianglesChunkGeometry::get_normals() const {
        return normals;
    }

    void MaszynaTrianglesChunkGeometry::set_uvs(const PackedFloat32Array &p_uvs) {
        uvs = p_uvs;
    }

    PackedFloat32Array MaszynaTrianglesChunkGeometry::get_uvs() const {
        return uvs;
    }

    void MaszynaTrianglesChunkGeometry::set_group_centres(const PackedVector3Array &p_group_centres) {
        group_centres = p_group_centres;
    }

    PackedVector3Array MaszynaTrianglesChunkGeometry::get_group_centres() const {
        return group_centres;
    }

    void MaszynaTrianglesChunkGeometry::set_group_ends(const PackedInt32Array &p_group_ends) {
        group_ends = p_group_ends;
    }

    PackedInt32Array MaszynaTrianglesChunkGeometry::get_group_ends() const {
        return group_ends;
    }

    int64_t MaszynaTrianglesChunkGeometry::get_group_count() const {
        return group_ends.size();
    }

    int64_t MaszynaTrianglesChunkGeometry::_get_group_first(const int64_t p_group) const {
        return p_group > 0 ? group_ends[p_group - 1] : 0;
    }

    Array MaszynaTrianglesChunkGeometry::get_group_mesh_arrays(const int64_t p_group) const {
        ERR_FAIL_INDEX_V(p_group, group_ends.size(), Array());
        const int64_t first = _get_group_first(p_group);
        const int64_t vertex_count = group_ends[p_group] - first;
        PackedVector3Array mesh_vertices;
        PackedVector3Array mesh_normals;
        PackedVector2Array mesh_uvs;
        mesh_vertices.resize(vertex_count);
        mesh_normals.resize(vertex_count);
        mesh_uvs.resize(vertex_count);
        for (int64_t index = 0; index < vertex_count; index++) {
            const int64_t vector3 = (first + index) * VECTOR3_FLOATS;
            const int64_t vector2 = (first + index) * VECTOR2_FLOATS;
            mesh_vertices.set(index, Vector3(vertices[vector3], vertices[vector3 + 1], vertices[vector3 + 2]));
            mesh_normals.set(index, Vector3(normals[vector3], normals[vector3 + 1], normals[vector3 + 2]));
            mesh_uvs.set(index, Vector2(uvs[vector2], uvs[vector2 + 1]));
        }
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = mesh_vertices;
        arrays[Mesh::ARRAY_NORMAL] = mesh_normals;
        arrays[Mesh::ARRAY_TEX_UV] = mesh_uvs;
        return arrays;
    }

    /// As E3DInstanceBackend::_visibility_aabb(): Godot measures a visibility range to the centre of
    /// the instance's box, the original from the shape's centre (opengl33renderer.cpp:2814)
    AABB MaszynaTrianglesChunkGeometry::get_group_visibility_aabb(const int64_t p_group) const {
        ERR_FAIL_INDEX_V(p_group, group_ends.size(), AABB());
        const Vector3 centre = group_centres[p_group];
        Vector3 extent;
        for (int64_t index = _get_group_first(p_group); index < group_ends[p_group]; index++) {
            const int64_t first = index * VECTOR3_FLOATS;
            extent = extent.max((Vector3(vertices[first], vertices[first + 1], vertices[first + 2]) - centre).abs());
        }
        return {centre - extent, extent * 2.0};
    }

    /// A piece takes whole groups while they fit; a group larger than a piece goes as runs of whole
    /// triangles, each a group of the same centre
    TypedArray<MaszynaTrianglesChunkGeometry> MaszynaTrianglesChunkGeometry::split() {
        TypedArray<MaszynaTrianglesChunkGeometry> pieces;
        const int64_t vertex_count = vertices.size() / VECTOR3_FLOATS;
        if (vertex_count <= MAX_VERTICES) {
            pieces.append(Ref<MaszynaTrianglesChunkGeometry>(this));
            return pieces;
        }
        // MAX_VERTICES is a power of two; a whole number of triangles below it
        constexpr int64_t TRIANGLE_VERTICES = 3;
        const int64_t piece_vertices = MAX_VERTICES - (MAX_VERTICES % TRIANGLE_VERTICES);
        Ref<MaszynaTrianglesChunkGeometry> piece;
        int64_t piece_first = 0;
        const auto close_piece = [&](const int64_t p_last) {
            if (piece.is_null()) {
                return;
            }
            piece->set_vertices(vertices.slice(piece_first * VECTOR3_FLOATS, p_last * VECTOR3_FLOATS));
            piece->set_normals(normals.slice(piece_first * VECTOR3_FLOATS, p_last * VECTOR3_FLOATS));
            piece->set_uvs(uvs.slice(piece_first * VECTOR2_FLOATS, p_last * VECTOR2_FLOATS));
            pieces.append(piece);
            piece.unref();
        };
        const auto open_piece = [&](const int64_t p_first) {
            piece.instantiate();
            piece->set_texture(texture);
            piece->set_origin(origin);
            piece->set_range_min(range_min);
            piece->set_range_max(range_max);
            piece_first = p_first;
        };
        for (int64_t group = 0; group < group_ends.size(); group++) {
            for (int64_t first = _get_group_first(group); first < group_ends[group];) {
                if (piece.is_null()) {
                    open_piece(first);
                }
                const int64_t room = piece_vertices - (first - piece_first);
                const int64_t last = MIN(group_ends[group], first + room);
                PackedInt32Array ends = piece->get_group_ends();
                ends.push_back(static_cast<int32_t>(last - piece_first));
                piece->set_group_ends(ends);
                PackedVector3Array centres = piece->get_group_centres();
                centres.push_back(group_centres[group]);
                piece->set_group_centres(centres);
                if (last - piece_first >= piece_vertices) {
                    close_piece(last);
                }
                first = last;
            }
        }
        close_piece(vertex_count);
        return pieces;
    }
} // namespace godot
