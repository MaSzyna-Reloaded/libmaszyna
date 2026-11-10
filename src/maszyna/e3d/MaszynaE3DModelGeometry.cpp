#include "MaszynaE3DModelGeometry.hpp"

#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {
    static PackedVector3Array to_vectors3(const PackedFloat32Array &p_floats) {
        PackedVector3Array vectors;
        vectors.resize(p_floats.size() / MaszynaE3DModelGeometry::VECTOR3_FLOATS);
        for (int64_t index = 0; index < vectors.size(); index++) {
            const int64_t first = index * MaszynaE3DModelGeometry::VECTOR3_FLOATS;
            vectors.set(index, Vector3(p_floats[first], p_floats[first + 1], p_floats[first + 2]));
        }
        return vectors;
    }

    static PackedVector2Array to_vectors2(const PackedFloat32Array &p_floats) {
        PackedVector2Array vectors;
        vectors.resize(p_floats.size() / MaszynaE3DModelGeometry::VECTOR2_FLOATS);
        for (int64_t index = 0; index < vectors.size(); index++) {
            const int64_t first = index * MaszynaE3DModelGeometry::VECTOR2_FLOATS;
            vectors.set(index, Vector2(p_floats[first], p_floats[first + 1]));
        }
        return vectors;
    }

    void MaszynaE3DModelGeometry::_bind_methods() {
        BIND_PROPERTY_W_HINT(
                MaszynaE3DModelGeometry, Variant::ARRAY, surfaces, PROPERTY_HINT_ARRAY_TYPE,
                Variant::get_type_name(Variant::DICTIONARY));
        ClassDB::bind_method(D_METHOD("mesh_create", "index"), &MaszynaE3DModelGeometry::mesh_create);
        BIND_CONSTANT(FORMAT_VERSION);
    }

    Ref<ArrayMesh> MaszynaE3DModelGeometry::mesh_create(const int p_index) const {
        ERR_FAIL_INDEX_V(p_index, surfaces.size(), Ref<ArrayMesh>());
        const Dictionary surface = surfaces[p_index];
        const PackedFloat32Array vertices = surface.get(SURFACE_VERTICES, PackedFloat32Array());
        if (vertices.is_empty()) {
            return {};
        }
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = to_vectors3(vertices);
        if (const PackedFloat32Array normals = surface.get(SURFACE_NORMALS, PackedFloat32Array());
            !normals.is_empty()) {
            arrays[Mesh::ARRAY_NORMAL] = to_vectors3(normals);
        }
        if (const PackedFloat32Array tangents = surface.get(SURFACE_TANGENTS, PackedFloat32Array());
            !tangents.is_empty()) {
            arrays[Mesh::ARRAY_TANGENT] = tangents;
        }
        arrays[Mesh::ARRAY_TEX_UV] = to_vectors2(surface.get(SURFACE_UVS, PackedFloat32Array()));
        if (const PackedInt32Array indices = surface.get(SURFACE_INDICES, PackedInt32Array()); !indices.is_empty()) {
            arrays[Mesh::ARRAY_INDEX] = indices;
        }
        Ref<ArrayMesh> mesh;
        mesh.instantiate();
        mesh->add_surface_from_arrays(Mesh::PRIMITIVE_TRIANGLES, arrays);
        return mesh;
    }
} // namespace godot
