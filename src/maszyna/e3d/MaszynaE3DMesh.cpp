#include "MaszynaE3DMesh.hpp"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>

namespace godot {
    static PackedVector3Array to_vectors3(const PackedFloat32Array &p_floats) {
        PackedVector3Array vectors;
        vectors.resize(p_floats.size() / MaszynaE3DMesh::VECTOR3_FLOATS);
        for (int64_t index = 0; index < vectors.size(); index++) {
            const int64_t first = index * MaszynaE3DMesh::VECTOR3_FLOATS;
            vectors.set(index, Vector3(p_floats[first], p_floats[first + 1], p_floats[first + 2]));
        }
        return vectors;
    }

    static PackedVector2Array to_vectors2(const PackedFloat32Array &p_floats) {
        PackedVector2Array vectors;
        vectors.resize(p_floats.size() / MaszynaE3DMesh::VECTOR2_FLOATS);
        for (int64_t index = 0; index < vectors.size(); index++) {
            const int64_t first = index * MaszynaE3DMesh::VECTOR2_FLOATS;
            vectors.set(index, Vector2(p_floats[first], p_floats[first + 1]));
        }
        return vectors;
    }

    void MaszynaE3DMesh::_bind_methods() {
        ClassDB::bind_method(D_METHOD("set_surface", "surface"), &MaszynaE3DMesh::set_surface);
        ClassDB::bind_method(D_METHOD("get_surface"), &MaszynaE3DMesh::get_surface);
        ADD_PROPERTY(PropertyInfo(Variant::DICTIONARY, "surface"), "set_surface", "get_surface");
    }

    MaszynaE3DMesh::~MaszynaE3DMesh() {
        if (mesh.is_valid()) {
            RenderingServer::get_singleton()->free_rid(mesh);
        }
    }

    int64_t MaszynaE3DMesh::_get_vertex_count() const {
        return vertices.size() / VECTOR3_FLOATS;
    }

    void MaszynaE3DMesh::set_surface(const Dictionary &p_surface) {
        RenderingServer *rendering_server = RenderingServer::get_singleton();
        if (mesh.is_valid()) {
            rendering_server->free_rid(mesh);
            mesh = RID();
        }
        vertices = p_surface.get(SURFACE_VERTICES, PackedFloat32Array());
        normals = p_surface.get(SURFACE_NORMALS, PackedFloat32Array());
        tangents = p_surface.get(SURFACE_TANGENTS, PackedFloat32Array());
        uvs = p_surface.get(SURFACE_UVS, PackedFloat32Array());
        indices = p_surface.get(SURFACE_INDICES, PackedInt32Array());

        aabb = AABB();
        for (int64_t index = 0; index < _get_vertex_count(); index++) {
            const int64_t first = index * VECTOR3_FLOATS;
            const Vector3 vertex(vertices[first], vertices[first + 1], vertices[first + 2]);
            if (index == 0) {
                aabb = AABB(vertex, Vector3());
            } else {
                aabb.expand_to(vertex);
            }
        }
        if (_get_vertex_count() == 0) {
            return;
        }
        mesh = rendering_server->mesh_create();
        rendering_server->mesh_add_surface_from_arrays(
                mesh, RenderingServer::PRIMITIVE_TRIANGLES, _surface_get_arrays(0));
        if (material.is_valid()) {
            rendering_server->mesh_surface_set_material(mesh, 0, material->get_rid());
        }
    }

    Dictionary MaszynaE3DMesh::get_surface() const {
        Dictionary surface;
        surface[SURFACE_VERTICES] = vertices;
        surface[SURFACE_NORMALS] = normals;
        surface[SURFACE_TANGENTS] = tangents;
        surface[SURFACE_UVS] = uvs;
        surface[SURFACE_INDICES] = indices;
        return surface;
    }

    RID MaszynaE3DMesh::_get_rid() const {
        return mesh;
    }

    int32_t MaszynaE3DMesh::_get_surface_count() const {
        return _get_vertex_count() > 0 ? 1 : 0;
    }

    int32_t MaszynaE3DMesh::_surface_get_array_len(const int32_t p_index) const {
        ERR_FAIL_COND_V(p_index != 0, 0);
        return static_cast<int32_t>(_get_vertex_count());
    }

    int32_t MaszynaE3DMesh::_surface_get_array_index_len(const int32_t p_index) const {
        ERR_FAIL_COND_V(p_index != 0, 0);
        return static_cast<int32_t>(indices.size());
    }

    Array MaszynaE3DMesh::_surface_get_arrays(const int32_t p_index) const {
        ERR_FAIL_COND_V(p_index != 0, Array());
        Array arrays;
        arrays.resize(Mesh::ARRAY_MAX);
        arrays[Mesh::ARRAY_VERTEX] = to_vectors3(vertices);
        if (!normals.is_empty()) {
            arrays[Mesh::ARRAY_NORMAL] = to_vectors3(normals);
        }
        if (!tangents.is_empty()) {
            arrays[Mesh::ARRAY_TANGENT] = tangents;
        }
        arrays[Mesh::ARRAY_TEX_UV] = to_vectors2(uvs);
        if (!indices.is_empty()) {
            arrays[Mesh::ARRAY_INDEX] = indices;
        }
        return arrays;
    }

    TypedArray<Array> MaszynaE3DMesh::_surface_get_blend_shape_arrays(const int32_t /*p_index*/) const {
        return {};
    }

    Dictionary MaszynaE3DMesh::_surface_get_lods(const int32_t /*p_index*/) const {
        return {};
    }

    uint32_t MaszynaE3DMesh::_surface_get_format(const int32_t p_index) const {
        ERR_FAIL_COND_V(p_index != 0, 0);
        uint32_t format = Mesh::ARRAY_FORMAT_VERTEX | Mesh::ARRAY_FORMAT_TEX_UV;
        if (!normals.is_empty()) {
            format |= Mesh::ARRAY_FORMAT_NORMAL;
        }
        if (!tangents.is_empty()) {
            format |= Mesh::ARRAY_FORMAT_TANGENT;
        }
        if (!indices.is_empty()) {
            format |= Mesh::ARRAY_FORMAT_INDEX;
        }
        return format;
    }

    uint32_t MaszynaE3DMesh::_surface_get_primitive_type(const int32_t /*p_index*/) const {
        return Mesh::PRIMITIVE_TRIANGLES;
    }

    void MaszynaE3DMesh::_surface_set_material(const int32_t p_index, const Ref<Material> &p_material) {
        ERR_FAIL_COND(p_index != 0);
        material = p_material;
        if (mesh.is_valid()) {
            RenderingServer::get_singleton()->mesh_surface_set_material(
                    mesh, 0, material.is_valid() ? material->get_rid() : RID());
        }
    }

    Ref<Material> MaszynaE3DMesh::_surface_get_material(const int32_t /*p_index*/) const {
        return material;
    }

    int32_t MaszynaE3DMesh::_get_blend_shape_count() const {
        return 0;
    }

    StringName MaszynaE3DMesh::_get_blend_shape_name(const int32_t /*p_index*/) const {
        return {};
    }

    void MaszynaE3DMesh::_set_blend_shape_name(const int32_t /*p_index*/, const StringName & /*p_name*/) {}

    AABB MaszynaE3DMesh::_get_aabb() const {
        return aabb;
    }
} // namespace godot
