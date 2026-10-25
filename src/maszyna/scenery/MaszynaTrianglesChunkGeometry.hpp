#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/packed_int32_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// The triangles of one terrain chunk - scenery "triangles" nodes merged as the original merges
    /// its shapes (MaszynaTrianglesSink, MaszynaSBTTerrainProvider): a texture in a section of
    /// the 1 km grid, or a texture and a visibility range in a cell of the 250 m grid - a file of its
    /// own, read only while the camera is within the chunk's range (MaszynaSceneryChunkRenderingServer,
    /// ResourceLazyLoader).
    ///
    /// Its triangles are groups, one after another: the shapes the original merges within 25 m
    /// (basic_cell::insert(), scene.cpp:282-291), each drawn by its own range measured from its own
    /// centre (opengl33renderer.cpp:2814); a section's chunk is one group, drawn whatever its range.
    /// Positions and the groups' centres are relative to the origin, the cell's or the section's
    /// centre. Floats, not vectors: the
    /// project is built in double precision, which would double every vertex on disk and in memory;
    /// the mesh takes them as vectors only as it is made (get_group_mesh_arrays()).
    class MaszynaTrianglesChunkGeometry : public Resource {
            GDCLASS(MaszynaTrianglesChunkGeometry, Resource)

        private:
            String texture;
            Vector3 origin;
            float range_min = 0.0;
            float range_max = 0.0;
            PackedFloat32Array vertices;      // x, y, z per vertex
            PackedFloat32Array normals;       // x, y, z per vertex
            PackedFloat32Array uvs;           // u, v per vertex
            PackedVector3Array group_centres; // per group, relative to the origin
            PackedInt32Array group_ends;      // per group, the vertex after its last

            int64_t _get_group_first(int64_t p_group) const;

        protected:
            static void _bind_methods();

        public:
            /// Floats per vertex in vertices and normals, and per vertex in uvs
            static constexpr int VECTOR3_FLOATS = 3;
            static constexpr int VECTOR2_FLOATS = 2;
            /// The most vertices a chunk is built of: its groups are uploaded on the main thread at once,
            /// and a dense section of a region file is ~700 000 of them - past this an upload no
            /// longer fits SceneryStreamingServer's frame budget, and nothing can cut it
            static constexpr int64_t MAX_VERTICES = 65536;

            void set_texture(const String &p_texture);
            String get_texture() const;
            void set_origin(const Vector3 &p_origin);
            Vector3 get_origin() const;
            void set_range_min(float p_range_min);
            float get_range_min() const;
            void set_range_max(float p_range_max);
            float get_range_max() const;
            void set_vertices(const PackedFloat32Array &p_vertices);
            PackedFloat32Array get_vertices() const;
            void set_normals(const PackedFloat32Array &p_normals);
            PackedFloat32Array get_normals() const;
            void set_uvs(const PackedFloat32Array &p_uvs);
            PackedFloat32Array get_uvs() const;
            void set_group_centres(const PackedVector3Array &p_group_centres);
            PackedVector3Array get_group_centres() const;
            void set_group_ends(const PackedInt32Array &p_group_ends);
            PackedInt32Array get_group_ends() const;

            int64_t get_group_count() const;
            /* Mesh.ARRAY_MAX arrays of the group's vertices, normals and UVs, for add_surface_from_arrays() */
            Array get_group_mesh_arrays(int64_t p_group) const;
            /* The group's box widened to be centred on its centre, for its visibility range */
            AABB get_group_visibility_aabb(int64_t p_group) const;
            /* The chunk as pieces of at most MAX_VERTICES, cut between groups, and a group larger than
             * that between whole triangles - itself when it is no larger */
            TypedArray<MaszynaTrianglesChunkGeometry> split();
    };
} // namespace godot
