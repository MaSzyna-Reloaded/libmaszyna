#pragma once

#include "maszyna/scenery/MaszynaTrianglesChunkGeometry.hpp"
#include "scenery/SceneryStreamingServer.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/array.hpp>
#include <godot_cpp/variant/callable.hpp>
#include <godot_cpp/variant/packed_vector2_array.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/string.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>
#include <godot_cpp/variant/vector3.hpp>

#include <vector>

namespace godot {
    /// Where a scenery's "triangles" nodes go as they are parsed: merged into chunks right away, so
    /// the raw triangles of every include are never kept. A large scenery places one include tens of
    /// thousands of times (grass.inc), each one in world space, and keeping them all until the end of
    /// the parse filled the memory.
    ///
    /// A node is merged as the original merges a shape (basic_section::insert(), scene.cpp:846-863):
    /// - a large opaque one - no minimum range, a maximum past CELL_RANGE_MAX_M or none - is cut along
    ///   the 1 km grid and merged by texture alone, its range dropped: the original draws these
    ///   whenever their section is in view (Render(shape, true), opengl33renderer.cpp:2495);
    /// - a translucent one, one with a range of at most CELL_RANGE_MAX_M or a minimum range is merged
    ///   with a chunk of the same texture and range whose centre is closer than MERGE_DISTANCE_M, in
    ///   its CELL_SIZE_M cell (basic_cell::insert(), scene.cpp:282-291); the chunk's centre is the
    ///   vertex-weighted mean of its shapes (shape_node::merge(), scenenode.cpp:436) and its range is
    ///   measured from there.
    ///
    /// A sink with a directory keeps at most BUDGET_BYTES of geometry in memory: past that, its
    /// largest chunks are written to the directory as parts, and finish() puts each section's chunks
    /// together in one MaszynaTrianglesSector file there - a file a section, as the original's
    /// basic_section. A sink without one keeps everything, for a
    /// subscene whose chunks are cached with it.
    ///
    /// Thread-safe: the parse adds from every worker. The order of the triangles inside a chunk is
    /// the order they arrive in.
    class MaszynaTrianglesSink : public RefCounted {
            GDCLASS(MaszynaTrianglesSink, RefCounted)

        public:
            /// The grid the chunks are cut along, the streaming's
            static constexpr double CHUNK_SIZE_M = SceneryStreamingServer::CHUNK_SIZE_M;
            /// Geometry kept in memory by a sink with a directory before the largest chunks go to disk
            /// (create() takes another for a test)
            static constexpr int64_t BUDGET_BYTES = 256LL * 1024 * 1024;
            /// The original's cell a small shape is kept in (scene.h:34, EU07_CELLSIZE)
            static constexpr double CELL_SIZE_M = 250.0;
            /// A shape with a range of at most this goes to a cell (scene.cpp:850, rangesquared_max <= 90000)
            static constexpr double CELL_RANGE_MAX_M = 300.0;
            /// Shapes in a cell merge only when their centres are closer than this (scene.cpp:287)
            static constexpr double MERGE_DISTANCE_M = 25.0;

        private:
            /// A shape the original merged within MERGE_DISTANCE_M, or a section's whole chunk
            struct Group {
                    /// What the range is measured from, in the world: the vertex-weighted mean of its
                    /// shapes' centres
                    Vector3 centre;
                    int64_t vertex_count = 0;    // kept past a spill, for the weight of the next shape
                    std::vector<float> vertices; // relative to the chunk's origin
                    std::vector<float> normals;
                    std::vector<float> uvs;
            };

            /// A texture and a range in a CELL_SIZE_M cell, or a texture in a CHUNK_SIZE_M section
            struct Chunk {
                    String texture;
                    float range_min = 0.0;
                    float range_max = 0.0;
                    Vector3 origin; // the cell's or the section's centre
                    std::vector<Group> groups;
                    int parts = 0; // written to the directory so far
            };

            mutable Mutex mutex;
            String directory;
            int64_t budget_bytes = BUDGET_BYTES;
            /// texture -> translucent (MaszynaMaterialManager.is_material_translucent()), called on the
            /// parse's workers
            Callable translucency;
            HashMap<String, bool> translucent_textures;
            std::vector<Chunk> chunks; // in the order they first appeared
            HashMap<String, int> chunk_indices;
            int64_t buffered_bytes = 0;

            bool _is_translucent(const String &p_texture);
            Chunk &_get_chunk(
                    const String &p_key, const String &p_texture, float p_range_min, float p_range_max,
                    const Vector3 &p_origin);
            Chunk &_get_section_chunk(const String &p_texture, const Vector2i &p_section);
            Chunk &
            _get_cell_chunk(const String &p_texture, float p_range_min, float p_range_max, const Vector3 &p_centre);
            static Group &_get_group(Chunk &p_chunk, const Vector3 &p_centre, int64_t p_vertex_count);
            static int64_t _get_bytes(const Group &p_group);
            static Ref<MaszynaTrianglesChunkGeometry> _to_geometry(const Chunk &p_chunk);
            static void _append_part(Chunk &p_chunk, const Ref<MaszynaTrianglesChunkGeometry> &p_part);
            String _get_part_path(int p_chunk, int p_part) const;
            void _spill();

        protected:
            static void _bind_methods();

        public:
            /* Empty: everything kept in memory. A directory is emptied first - it is the sink's own.
             * p_translucency(texture: String) -> bool */
            static Ref<MaszynaTrianglesSink>
            create(const String &p_directory, const Callable &p_translucency, int64_t p_budget_bytes = BUDGET_BYTES);
            /* The centre of a section of the 1 km grid, which its chunk's positions are relative to */
            static Vector3 cell_get_origin(const Vector2i &p_cell);
            /* A shape the original keeps in a cell and draws by its range, not in its section
             * (basic_section::insert(), scene.cpp:849-851) */
            static bool shape_is_ranged(bool p_translucent, float p_range_min, float p_range_max);

            void add_triangles(
                    const String &p_texture, const PackedVector3Array &p_vertices, const PackedVector3Array &p_normals,
                    const PackedVector2Array &p_uvs, float p_range_min, float p_range_max);
            /* Merges a chunk's geometry group by group, each as one shape, by the rules of add_triangles() */
            void add_geometry(const Ref<MaszynaTrianglesChunkGeometry> &p_geometry);
            /* add_geometry() of every chunk of a sector file another sink finished - read, added and
             * let go of */
            void add_geometry_file(const String &p_path);
            /* The chunks kept in memory, in the order they first appeared */
            TypedArray<MaszynaTrianglesChunkGeometry> get_geometries() const;
            /* Writes every section's chunks to one file, <directory>/<x>_<z>.res (MaszynaTrianglesSector),
             * and empties the sink; returns a Dictionary per chunk: path (its sector's), index (in the
             * sector), position (its origin), texture, range_min, range_max */
            Array finish();
    };
} // namespace godot
