#pragma once

#include "maszyna/terrain/MaszynaHeightmapTerrainChunk.hpp"

#include <godot_cpp/classes/ref_counted.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>
#include <godot_cpp/variant/vector2i.hpp>

namespace godot {
    /// A region of a heightmap terrain as streaming supplies it (SceneryStreamingProvider
    /// CONTENT_HEIGHTMAP, MaszynaHeightmapTerrainProvider): a streaming cell - CHUNKS_SIDE chunks of
    /// MaszynaHeightmapTerrainChunk along its side - its chunks read, and the terrain it is drawn in
    /// (MaszynaHeightmapTerrainRenderingServer).
    class MaszynaHeightmapTerrainRegion : public RefCounted {
            GDCLASS(MaszynaHeightmapTerrainRegion, RefCounted)

        public:
            /// The chunks of a region along its side: a streaming cell of chunks
            /// (SceneryStreamingServer CHUNK_SIZE_M over MaszynaHeightmapTerrainChunk CHUNK_SIZE_M)
            static constexpr int CHUNKS_SIDE = 4;

        private:
            RID terrain;
            Vector2i key;
            TypedArray<MaszynaHeightmapTerrainChunk> chunks;

        protected:
            static void _bind_methods();

        public:
            /// The region holding the chunk of p_key
            static Vector2i chunk_get_region(const Vector2i &p_key);

            /// Takes the terrain it is drawn in, its key - its streaming cell - and its chunks,
            /// CHUNKS_SIDE^2 in rows of growing z, null where there is none
            void set_data(
                    const RID &p_terrain, const Vector2i &p_key,
                    const TypedArray<MaszynaHeightmapTerrainChunk> &p_chunks);

            RID get_terrain() const;
            Vector2i get_key() const;
            TypedArray<MaszynaHeightmapTerrainChunk> get_chunks() const;
    };
} // namespace godot
