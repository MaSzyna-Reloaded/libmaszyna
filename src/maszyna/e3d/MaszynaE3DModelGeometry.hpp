#pragma once
#include "macros.hpp"

#include <godot_cpp/classes/array_mesh.hpp>
#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /* The geometry of a model file (.e3d or .t3d) - what of it goes to the GPU - apart from its
     * description (MaszynaE3DModel): the arrays of every submodel of triangles, by its
     * MaszynaE3DSubModel::geometry_index. It is data only, in memory while a model is read and its
     * cache written or read (MaszynaE3DModelManager), and let go once the model's meshes are made of
     * it (MaszynaE3DModel::apply_geometry()): an ArrayMesh keeps nothing of it but in the GPU. Written
     * from the parse's arrays, so writing it reads nothing back from the GPU.
     *
     * Floats, not vectors: the project is built in double precision, which would double every vertex. */
    class MaszynaE3DModelGeometry : public Resource {
            GDCLASS(MaszynaE3DModelGeometry, Resource)

        public:
            /* Must be raised when what the parsers put into the geometry changes: it invalidates the cache */
            static constexpr int FORMAT_VERSION = 20261110;
            /* Keys of a surface Dictionary */
            static constexpr const char *SURFACE_VERTICES = "vertices";
            static constexpr const char *SURFACE_NORMALS = "normals";
            static constexpr const char *SURFACE_TANGENTS = "tangents";
            static constexpr const char *SURFACE_UVS = "uvs";
            static constexpr const char *SURFACE_INDICES = "indices";
            /* Floats per vertex in vertices and normals, and in uvs */
            static constexpr int VECTOR3_FLOATS = 3;
            static constexpr int VECTOR2_FLOATS = 2;

        protected:
            static void _bind_methods();

        private:
            /* One surface a submodel, by its geometry index; empty for one without triangles. Keys:
             * vertices, normals, tangents (optional), uvs (PackedFloat32Array), indices
             * (PackedInt32Array, the triangles clockwise) */
            TypedArray<Dictionary> surfaces;

        public:
            MAKE_GS_NR(TypedArray<Dictionary>, surfaces);

            /* The mesh of the surface p_index, made in the RenderingServer - the GPU; null for none */
            Ref<ArrayMesh> mesh_create(int p_index) const;
    };
} // namespace godot
