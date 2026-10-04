#include "legacy/scenery/MaszynaTrianglesChunkGeometry.hpp"

#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {
    void MaszynaTrianglesChunkGeometry::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_texture", "texture"), &MaszynaTrianglesChunkGeometry::set_texture);
        ClassDB::bind_method(D_METHOD("get_texture"), &MaszynaTrianglesChunkGeometry::get_texture);
        ClassDB::bind_method(D_METHOD("set_cell", "cell"), &MaszynaTrianglesChunkGeometry::set_cell);
        ClassDB::bind_method(D_METHOD("get_cell"), &MaszynaTrianglesChunkGeometry::get_cell);
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
        ClassDB::bind_method(D_METHOD("to_mesh_arrays"), &MaszynaTrianglesChunkGeometry::to_mesh_arrays);
        ClassDB::bind_method(D_METHOD("split"), &MaszynaTrianglesChunkGeometry::split);
        BIND_CONSTANT(MAX_VERTICES);

        ADD_PROPERTY(PropertyInfo(Variant::STRING, "texture"), "set_texture", "get_texture");
        ADD_PROPERTY(PropertyInfo(Variant::VECTOR2I, "cell"), "set_cell", "get_cell");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_min"), "set_range_min", "get_range_min");
        ADD_PROPERTY(PropertyInfo(Variant::FLOAT, "range_max"), "set_range_max", "get_range_max");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "vertices"), "set_vertices", "get_vertices");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "normals"), "set_normals", "get_normals");
        ADD_PROPERTY(PropertyInfo(Variant::PACKED_FLOAT32_ARRAY, "uvs"), "set_uvs", "get_uvs");
    }

    void MaszynaTrianglesChunkGeometry::set_texture(const String &p_texture) {
        texture = p_texture;
    }

    String MaszynaTrianglesChunkGeometry::get_texture() const {
        return texture;
    }

    void MaszynaTrianglesChunkGeometry::set_cell(const Vector2i &p_cell) {
        cell = p_cell;
    }

    Vector2i MaszynaTrianglesChunkGeometry::get_cell() const {
        return cell;
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

    Array MaszynaTrianglesChunkGeometry::to_mesh_arrays() const {
        const int64_t vertex_count = vertices.size() / VECTOR3_FLOATS;
        PackedVector3Array mesh_vertices;
        PackedVector3Array mesh_normals;
        PackedVector2Array mesh_uvs;
        mesh_vertices.resize(vertex_count);
        mesh_normals.resize(vertex_count);
        mesh_uvs.resize(vertex_count);
        for (int64_t index = 0; index < vertex_count; index++) {
            const int64_t vector3 = index * VECTOR3_FLOATS;
            const int64_t vector2 = index * VECTOR2_FLOATS;
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
        for (int64_t first = 0; first < vertex_count; first += piece_vertices) {
            const int64_t last = MIN(first + piece_vertices, vertex_count);
            Ref<MaszynaTrianglesChunkGeometry> piece;
            piece.instantiate();
            piece->set_texture(texture);
            piece->set_cell(cell);
            piece->set_range_min(range_min);
            piece->set_range_max(range_max);
            piece->set_vertices(vertices.slice(first * VECTOR3_FLOATS, last * VECTOR3_FLOATS));
            piece->set_normals(normals.slice(first * VECTOR3_FLOATS, last * VECTOR3_FLOATS));
            piece->set_uvs(uvs.slice(first * VECTOR2_FLOATS, last * VECTOR2_FLOATS));
            pieces.append(piece);
        }
        return pieces;
    }
} // namespace godot
