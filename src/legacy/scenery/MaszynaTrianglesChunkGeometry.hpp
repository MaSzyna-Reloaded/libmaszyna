#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/aabb.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector3.hpp>

namespace godot {
    /// The triangles of one terrain chunk - scenery "triangles" nodes merged as the original merges
    /// its shapes (SceneryTrianglesSink): a texture in a section of the 1 km grid, or a texture
    /// and a visibility range around one centre - a file of its own, read only while the camera is
    /// within the chunk's range (MaszynaSceneryChunkRenderingServer, ResourceLazyLoader).
    /// Positions are relative to the origin: the section's centre, or the merged shapes' centre the
    /// visibility range is measured from (scene.cpp:282-291). Floats, not vectors: the
    /// project is built in double precision, which would double every vertex on disk and in memory;
    /// the mesh takes them as vectors only as it is made (to_mesh_arrays()).
    class MaszynaTrianglesChunkGeometry : public Resource {
            GDCLASS(MaszynaTrianglesChunkGeometry, Resource)

        private:
            String texture;
            Vector3 origin;
            float range_min = 0.0;
            float range_max = 0.0;
            PackedFloat32Array vertices; // x, y, z per vertex
            PackedFloat32Array normals;  // x, y, z per vertex
            PackedFloat32Array uvs;      // u, v per vertex

        protected:
            static void _bind_methods();

        public:
            /// Floats per vertex in vertices and normals, and per vertex in uvs
            static constexpr int VECTOR3_FLOATS = 3;
            static constexpr int VECTOR2_FLOATS = 2;
            /// The most vertices a chunk is built of: it is uploaded on the main thread in one go,
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

            /* Mesh.ARRAY_MAX arrays with the vertices, normals and UVs, for add_surface_from_arrays() */
            Array to_mesh_arrays() const;
            /* The vertices' box widened to be centred on the origin, for a visibility range */
            AABB get_visibility_aabb() const;
            /* The chunk as pieces of at most MAX_VERTICES, cut between whole triangles - itself when
             * it is no larger */
            TypedArray<MaszynaTrianglesChunkGeometry> split();
    };
} // namespace godot
