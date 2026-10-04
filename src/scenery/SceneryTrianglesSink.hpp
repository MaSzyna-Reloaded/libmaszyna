#pragma once

#include "legacy/scenery/MaszynaTrianglesChunkGeometry.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace godot {
    /// Where a scenery's "triangles" nodes go as they are parsed: cut along the 1 km grid and merged
    /// into chunks - a texture, a cell and a visibility range each - right away, so the raw triangles
    /// of every include are never kept. A large scenery places one include tens of thousands of
    /// times (grass.inc), each one in world space, and keeping them all until the end of the parse
    /// filled the memory.
    ///
    /// A sink with a directory keeps at most BUDGET_BYTES of geometry in memory: past that, its
    /// largest chunks are written to the directory as parts, and finish() puts each chunk together
    /// in one MaszynaTrianglesChunkGeometry file there. A sink without one keeps everything, for a
    /// subscene whose chunks are cached with it.
    ///
    /// Thread-safe: the parse adds from every worker. The order of the triangles inside a chunk is
    /// the order they arrive in.
    class SceneryTrianglesSink : public RefCounted {
            GDCLASS(SceneryTrianglesSink, RefCounted)

        public:
            /// The grid the chunks are cut along, the streaming's (SceneryStreamingServer::CHUNK_SIZE_M)
            static constexpr double CHUNK_SIZE_M = 1000.0;
            /// Geometry kept in memory by a sink with a directory before the largest chunks go to disk
            /// (create() takes another for a test)
            static constexpr int64_t BUDGET_BYTES = 256LL * 1024 * 1024;

        private:
            struct Chunk {
                    String texture;
                    Vector2i cell;
                    float range_min = 0.0;
                    float range_max = 0.0;
                    std::vector<float> vertices;
                    std::vector<float> normals;
                    std::vector<float> uvs;
                    int parts = 0; // written to the directory so far
            };

            mutable Mutex mutex;
            String directory;
            int64_t budget_bytes = BUDGET_BYTES;
            std::vector<Chunk> chunks; // in the order they first appeared
            HashMap<String, int> chunk_indices;
            int64_t buffered_bytes = 0;

            static String
            _get_chunk_key(const String &p_texture, const Vector2i &p_cell, float p_range_min, float p_range_max);
            Chunk &_get_chunk(const String &p_texture, const Vector2i &p_cell, float p_range_min, float p_range_max);
            static int64_t _get_bytes(const Chunk &p_chunk);
            static Ref<MaszynaTrianglesChunkGeometry> _to_geometry(const Chunk &p_chunk);
            static void _append(Chunk &p_chunk, const Ref<MaszynaTrianglesChunkGeometry> &p_geometry);
            String _get_part_path(int p_chunk, int p_part) const;
            void _spill();

        protected:
            static void _bind_methods();

        public:
            /* Empty: everything kept in memory. A directory is emptied first - it is the sink's own */
            static Ref<SceneryTrianglesSink> create(const String &p_directory, int64_t p_budget_bytes = BUDGET_BYTES);
            /* The centre of a cell of the grid, which a chunk's positions are relative to */
            static Vector3 cell_get_origin(const Vector2i &p_cell);

            void add_triangles(
                    const String &p_texture, const PackedVector3Array &p_vertices, const PackedVector3Array &p_normals,
                    const PackedVector2Array &p_uvs, float p_range_min, float p_range_max);
            /* Appends a whole chunk's geometry to the chunk of its texture, cell and range */
            void add_geometry(const Ref<MaszynaTrianglesChunkGeometry> &p_geometry);
            /* add_geometry() of a chunk file another sink finished - read, added and let go of */
            void add_geometry_file(const String &p_path);
            /* The chunks kept in memory, in the order they first appeared */
            TypedArray<MaszynaTrianglesChunkGeometry> get_geometries() const;
            /* Writes every chunk to <directory>/<index>.res and empties the sink; returns a Dictionary per
             * chunk: path, position, texture, range_min, range_max */
            Array finish();
    };
} // namespace godot
