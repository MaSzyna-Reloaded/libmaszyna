#include "maszyna/terrain/MaszynaHeightmapTerrainRegion.hpp"

#include "scenery/SceneryStreamingServer.hpp"

namespace godot {
    static_assert(
            MaszynaHeightmapTerrainRegion::CHUNKS_SIDE * MaszynaHeightmapTerrainChunk::CHUNK_SIZE_M ==
                    SceneryStreamingServer::CHUNK_SIZE_M,
            "a region is a streaming cell of chunks");

    void MaszynaHeightmapTerrainRegion::_bind_methods() {
        ClassDB::bind_static_method(
                "MaszynaHeightmapTerrainRegion", D_METHOD("chunk_get_region", "key"),
                &MaszynaHeightmapTerrainRegion::chunk_get_region);
        ClassDB::bind_method(
                D_METHOD("set_data", "terrain", "key", "chunks"), &MaszynaHeightmapTerrainRegion::set_data);
        ClassDB::bind_method(D_METHOD("get_terrain"), &MaszynaHeightmapTerrainRegion::get_terrain);
        ClassDB::bind_method(D_METHOD("get_key"), &MaszynaHeightmapTerrainRegion::get_key);
        ClassDB::bind_method(D_METHOD("get_chunks"), &MaszynaHeightmapTerrainRegion::get_chunks);
        BIND_CONSTANT(CHUNKS_SIDE);
    }

    Vector2i MaszynaHeightmapTerrainRegion::chunk_get_region(const Vector2i &p_key) {
        const auto floor_div = [](const int p_value) {
            return p_value >= 0 ? p_value / CHUNKS_SIDE : -((-p_value + CHUNKS_SIDE - 1) / CHUNKS_SIDE);
        };
        return {floor_div(p_key.x), floor_div(p_key.y)};
    }

    void MaszynaHeightmapTerrainRegion::set_data(
            const RID &p_terrain, const Vector2i &p_key, const TypedArray<MaszynaHeightmapTerrainChunk> &p_chunks) {
        terrain = p_terrain;
        key = p_key;
        chunks = p_chunks;
    }

    RID MaszynaHeightmapTerrainRegion::get_terrain() const {
        return terrain;
    }

    Vector2i MaszynaHeightmapTerrainRegion::get_key() const {
        return key;
    }

    TypedArray<MaszynaHeightmapTerrainChunk> MaszynaHeightmapTerrainRegion::get_chunks() const {
        return chunks;
    }
} // namespace godot
