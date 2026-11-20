#pragma once

#include "maszyna/terrain/MaszynaHeightmapTerrainPack.hpp"
#include "scenery/SceneryStreamingProvider.hpp"

#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/string.hpp>

namespace godot {
    /// The original's heightmap terrain (`heightmap_terrain`, scene/heightmapterrain.h) supplied from
    /// its pack files (.tch) region by region as the camera comes near - a region is the streaming's
    /// cell of 1 km, MaszynaHeightmapTerrainRegion's CHUNKS_SIDE chunks a side - as the regions of a
    /// heightmap terrain (CONTENT_HEIGHTMAP), the chunks of each read on the streaming worker. What
    /// draws them (MaszynaHeightmapTerrainRenderingServer) takes them by the terrain they name.
    class MaszynaHeightmapTerrainProvider : public SceneryStreamingProvider {
            GDCLASS(MaszynaHeightmapTerrainProvider, SceneryStreamingProvider)

        private:
            LocalVector<Ref<MaszynaHeightmapTerrainPack>> packs;
            /// The pack of each chunk, its index in packs
            HashMap<Vector2i, int> chunk_packs;
            /// The chunks of each region
            HashMap<Vector2i, int> regions;
            RID terrain;
            float range = 0.0F;

        protected:
            static void _bind_methods();

        public:
            /// The packs of p_directory - terrain/<name>/ of the game - opened; false when there is
            /// none
            bool open(const String &p_directory);
            /// The terrain its regions are drawn in, given before the provider is registered
            void set_terrain(const RID &p_terrain);
            RID get_terrain() const;
            /// How far from the camera its regions are wanted - as far as the terrain draws them
            /// (MaszynaHeightmapTerrainRenderingServer terrain_get_range()) - given before the
            /// provider is registered
            void set_range(float p_range);
            /// The chunks its packs hold
            int get_chunk_count() const;

            TypedArray<Vector2i> get_chunk_cells() const override;
            float get_range() const override;
            PackedInt32Array get_content_kinds() const override;
            Array chunk_load(const Vector2i &p_cell, ContentKind p_kind) override;
    };
} // namespace godot
