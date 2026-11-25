#include "maszyna/scenery/MaszynaHeightmapTerrainProvider.hpp"

#include "maszyna/terrain/MaszynaHeightmapTerrainRegion.hpp"

#include <godot_cpp/classes/dir_access.hpp>

namespace godot {
    namespace {
        /// The pack files of a terrain's folder (heightmap::pack_file_name())
        constexpr const char *PACK_EXTENSION = "tch";
    } // namespace

    void MaszynaHeightmapTerrainProvider::_bind_methods() {
        ClassDB::bind_method(D_METHOD("open", "directory"), &MaszynaHeightmapTerrainProvider::open);
        ClassDB::bind_method(D_METHOD("set_terrain", "terrain"), &MaszynaHeightmapTerrainProvider::set_terrain);
        ClassDB::bind_method(D_METHOD("get_terrain"), &MaszynaHeightmapTerrainProvider::get_terrain);
        ClassDB::bind_method(D_METHOD("set_range", "range"), &MaszynaHeightmapTerrainProvider::set_range);
        ClassDB::bind_method(D_METHOD("get_chunk_count"), &MaszynaHeightmapTerrainProvider::get_chunk_count);
    }

    bool MaszynaHeightmapTerrainProvider::open(const String &p_directory) {
        const PackedStringArray files = DirAccess::dir_exists_absolute(p_directory)
                                                ? DirAccess::get_files_at(p_directory)
                                                : PackedStringArray();
        for (const String &file: files) {
            if (file.get_extension().to_lower() != PACK_EXTENSION) {
                continue;
            }
            Ref<MaszynaHeightmapTerrainPack> pack;
            pack.instantiate();
            if (!pack->open(p_directory.path_join(file))) {
                continue;
            }
            const int index = static_cast<int>(packs.size());
            packs.push_back(pack);
            for (const Variant &key: pack->get_chunk_keys()) {
                chunk_packs[key] = index;
                regions[MaszynaHeightmapTerrainRegion::chunk_get_region(key)]++;
            }
        }
        ERR_FAIL_COND_V_MSG(chunk_packs.is_empty(), false, "No heightmap terrain chunks in \"" + p_directory + "\"");
        return true;
    }

    void MaszynaHeightmapTerrainProvider::set_terrain(const RID &p_terrain) {
        terrain = p_terrain;
    }

    RID MaszynaHeightmapTerrainProvider::get_terrain() const {
        return terrain;
    }

    void MaszynaHeightmapTerrainProvider::set_range(const float p_range) {
        range = p_range;
    }

    float MaszynaHeightmapTerrainProvider::get_range() const {
        return range;
    }

    int MaszynaHeightmapTerrainProvider::get_chunk_count() const {
        return static_cast<int>(chunk_packs.size());
    }

    TypedArray<Vector2i> MaszynaHeightmapTerrainProvider::get_chunk_cells() const {
        TypedArray<Vector2i> cells;
        for (const KeyValue<Vector2i, int> &region: regions) {
            cells.push_back(region.key);
        }
        return cells;
    }

    PackedInt32Array MaszynaHeightmapTerrainProvider::get_content_kinds() const {
        PackedInt32Array kinds;
        kinds.push_back(CONTENT_HEIGHTMAP);
        return kinds;
    }

    /// The region's chunks read on the streaming worker - each pack opens the file for itself
    Array MaszynaHeightmapTerrainProvider::chunk_load(const Vector2i &p_cell, const ContentKind p_kind) {
        Array items;
        if (p_kind != CONTENT_HEIGHTMAP || !regions.has(p_cell)) {
            return items;
        }
        constexpr int SIDE = MaszynaHeightmapTerrainRegion::CHUNKS_SIDE;
        TypedArray<MaszynaHeightmapTerrainChunk> chunks;
        for (int z = 0; z < SIDE; z++) {
            for (int x = 0; x < SIDE; x++) {
                const Vector2i key = (p_cell * SIDE) + Vector2i(x, z);
                const int *pack = chunk_packs.getptr(key);
                chunks.push_back(pack != nullptr ? packs[*pack]->read_chunk(key) : Ref<MaszynaHeightmapTerrainChunk>());
            }
        }
        Ref<MaszynaHeightmapTerrainRegion> region;
        region.instantiate();
        region->set_data(terrain, p_cell, chunks);
        items.push_back(region);
        return items;
    }
} // namespace godot
