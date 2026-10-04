#pragma once

#include <godot_cpp/classes/resource.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_float32_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// The triangles of one terrain chunk - scenery "triangles" nodes sharing a texture, a cell of
    /// the 1 km grid and a visibility range (SceneryTrianglesSink) - a file of its own, read only
    /// while the camera is within the chunk's range (MaszynaSceneryChunkRenderingServer,
    /// ResourceLazyLoader). Positions are relative to the cell's centre. Floats, not vectors: the
    /// project is built in double precision, which would double every vertex on disk and in memory;
    /// the mesh takes them as vectors only as it is made (to_mesh_arrays()).
    class MaszynaTrianglesChunkGeometry : public Resource {
            GDCLASS(MaszynaTrianglesChunkGeometry, Resource)

        private:
            String texture;
            Vector2i cell;
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
            void set_cell(const Vector2i &p_cell);
            Vector2i get_cell() const;
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
            /* The chunk as pieces of at most MAX_VERTICES, cut between whole triangles - itself when
             * it is no larger */
            TypedArray<MaszynaTrianglesChunkGeometry> split();
    };
} // namespace godot
