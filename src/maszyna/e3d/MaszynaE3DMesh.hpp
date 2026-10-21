#pragma once

#include <godot_cpp/classes/material.hpp>
#include <godot_cpp/classes/mesh.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string_name.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The one triangle surface of an E3D submodel, its arrays kept in memory beside the
    /// RenderingServer's mesh made of them.
    ///
    /// An ArrayMesh keeps its data only in the RenderingServer: saving one, getting its faces or its
    /// arrays reads every buffer back from the GPU, a stall of every frame in flight
    /// (`RD::buffer_get_data`), and off the main thread a wait for it. This mesh answers all of that
    /// from its own arrays, so a model is saved to the cache on the thread that loaded it, and
    /// picking, the cab's mirrors, lamps and screens read no GPU.
    ///
    /// Floats, not vectors: the project is built in double precision, which would double every vertex
    /// kept in memory (as MaszynaTrianglesChunkGeometry).
    class MaszynaE3DMesh : public Mesh {
            GDCLASS(MaszynaE3DMesh, Mesh)

        public:
            /// Keys of the surface Dictionary
            static constexpr const char *SURFACE_VERTICES = "vertices";
            static constexpr const char *SURFACE_NORMALS = "normals";
            static constexpr const char *SURFACE_TANGENTS = "tangents";
            static constexpr const char *SURFACE_UVS = "uvs";
            static constexpr const char *SURFACE_INDICES = "indices";
            /// Floats per vertex in vertices and normals, in tangents, and in uvs
            static constexpr int VECTOR3_FLOATS = 3;
            static constexpr int TANGENT_FLOATS = 4;
            static constexpr int VECTOR2_FLOATS = 2;

        private:
            PackedFloat32Array vertices;
            PackedFloat32Array normals;
            PackedFloat32Array tangents;
            PackedFloat32Array uvs;
            PackedInt32Array indices;
            AABB aabb;
            Ref<Material> material;
            RID mesh;

            int64_t _get_vertex_count() const;

        protected:
            static void _bind_methods();

        public:
            ~MaszynaE3DMesh() override;

            /* Sets the arrays and makes the RenderingServer's mesh of them, on the calling thread -
             * the parse's or the cache's loader. Keys: vertices, normals, tangents (optional), uvs
             * (PackedFloat32Array), indices (PackedInt32Array, the triangles clockwise) */
            void set_surface(const Dictionary &p_surface);
            Dictionary get_surface() const;

            RID _get_rid() const override;
            int32_t _get_surface_count() const override;
            int32_t _surface_get_array_len(int32_t p_index) const override;
            int32_t _surface_get_array_index_len(int32_t p_index) const override;
            Array _surface_get_arrays(int32_t p_index) const override;
            TypedArray<Array> _surface_get_blend_shape_arrays(int32_t p_index) const override;
            Dictionary _surface_get_lods(int32_t p_index) const override;
            uint32_t _surface_get_format(int32_t p_index) const override;
            uint32_t _surface_get_primitive_type(int32_t p_index) const override;
            void _surface_set_material(int32_t p_index, const Ref<Material> &p_material) override;
            Ref<Material> _surface_get_material(int32_t p_index) const override;
            int32_t _get_blend_shape_count() const override;
            StringName _get_blend_shape_name(int32_t p_index) const override;
            void _set_blend_shape_name(int32_t p_index, const StringName &p_name) override;
            AABB _get_aabb() const override;
    };
} // namespace godot
