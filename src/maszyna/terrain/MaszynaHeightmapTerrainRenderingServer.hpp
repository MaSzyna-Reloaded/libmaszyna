#pragma once

#include "maszyna/terrain/MaszynaHeightmapTerrainLook.hpp"
#include "maszyna/terrain/MaszynaHeightmapTerrainRegion.hpp"
#include "utils/RIDHash.hpp"

#include <godot_cpp/classes/image.hpp>
#include <godot_cpp/classes/object.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/templates/hash_map.hpp>
#include <godot_cpp/templates/local_vector.hpp>
#include <godot_cpp/templates/mutex.hpp>
#include <godot_cpp/variant/rid.hpp>
#include <godot_cpp/variant/typed_array.hpp>

namespace godot {
    /// The original's heightmap terrain (`heightmap_terrain`, scene/heightmapterrain.h) drawn as a
    /// clipmap: rings of a fixed grid around the camera, each twice as coarse as the one inside it,
    /// whose points the terrain's shader raises to the heights of the chunks under them and paints
    /// with their layers. Nothing is built per chunk: the heights and the paint of the chunks are
    /// textures. The server is the consumer of the regions streaming supplies
    /// (SceneryStreamingProvider CONTENT_HEIGHTMAP, MaszynaHeightmapTerrainRegion) as far as the
    /// clipmap's outer ring reaches (terrain_get_range()) and reads no file. A region is drawn:
    /// - from afar as soon as it is adopted: its heights COARSE_TEXELS a side and what its chunks
    ///   are - their layers and the average weight of each - or that there is no chunk, a hole;
    /// - in detail within FINE_RANGE_M, a piece it streams by that range: its heights at the
    ///   terrain's own spacing and the paint of its painted chunks, PAINT_SLOTS of them at most.
    class MaszynaHeightmapTerrainRenderingServer : public Object {
            GDCLASS(MaszynaHeightmapTerrainRenderingServer, Object)

        public:
            /// The heights of a region afar: a point every 1000 / 64 m
            static constexpr int COARSE_TEXELS = 65;
            /// Regions nearer get their heights at the terrain's spacing and their paint
            static constexpr double FINE_RANGE_M = 1500.0;
            /// The painted chunks near the camera that get their paint
            static constexpr int PAINT_SLOTS = 128;
            /// The cells of a ring of the clipmap along its side
            static constexpr int CLIPMAP_CELLS = 128;

        private:
            static MaszynaHeightmapTerrainRenderingServer *singleton;

            struct Terrain {
                    Ref<MaszynaHeightmapTerrainLook> look;
                    RID scenario;
                    /// How far from the camera its regions are drawn: the outer ring's corner
                    double range = 0.0;
                    /// The regions it can hold: the first one's key and how many along each side
                    Vector2i region_origin;
                    Vector2i region_count;
                    /// The points of a region along its side at the terrain's spacing
                    int fine_texels = 0;
                    /// The slot of each region in the textures, 0 none, as the shader reads them
                    Ref<Image> region_map;
                    RID region_map_texture;
                    RID coarse_heights;
                    RID chunk_info;
                    RID fine_heights;
                    RID paint_info;
                    RID paint_weights0;
                    RID paint_weights1;
                    RID palette;
                    LocalVector<int> coarse_free;
                    LocalVector<int> fine_free;
                    LocalVector<int> paint_free;
                    /// The rings of the clipmap
                    LocalVector<RID> meshes;
                    LocalVector<RID> materials;
                    LocalVector<RID> instances;
                    /// The regions adopted
                    LocalVector<RID> regions;
            };
            /// A region adopted: its slot in the textures from afar, and its detail piece
            struct Region {
                    RID terrain;
                    Vector2i key;
                    int coarse_slot = -1;
                    RID fine;
            };
            /// A region in detail, streamed: its textures' slots while it is built
            struct Piece {
                    RID terrain;
                    Ref<MaszynaHeightmapTerrainRegion> region;
                    RID stream;
                    int slot = -1;
                    LocalVector<int> paint_slots;
            };

            mutable Mutex mutex;
            RIDMap<Terrain> terrains;
            RIDMap<Region> regions;
            RIDMap<Piece> pieces;
            int stream_owner = -1;

            /// The streaming worker: the region's chunks made its detail textures' layers
            Variant _stream_preload(const RID &p_piece);
            void _stream_build(const RID &p_piece, const Variant &p_preloaded);
            void _stream_clear(const RID &p_piece);
            /// The region's slots written into the map the shader finds them by
            void _set_region_slot(Terrain &p_terrain, const Vector2i &p_region, bool p_fine, int p_slot);
            /// p_rings rings of the clipmap, each its mesh and its material, from p_spacing a cell
            void _make_clipmap(Terrain &p_terrain, double p_spacing, int p_rings);

        protected:
            static void _bind_methods();

        public:
            static MaszynaHeightmapTerrainRenderingServer *get_instance();

            MaszynaHeightmapTerrainRenderingServer();
            ~MaszynaHeightmapTerrainRenderingServer() override;

            /// A terrain of the regions p_regions - the streaming cells its provider lists - drawn into
            /// p_scenario with p_look; p_spacing is its manifest's point spacing, the finest the
            /// clipmap draws
            RID terrain_create(
                    const TypedArray<Vector2i> &p_regions, double p_spacing,
                    const Ref<MaszynaHeightmapTerrainLook> &p_look, const RID &p_scenario);
            void terrain_free(const RID &p_terrain);
            /// The world the terrain is drawn in, an empty RID for none (its scenery out of the tree)
            void terrain_set_scenario(const RID &p_terrain, const RID &p_scenario);
            /// The rings of its clipmap
            int terrain_get_ring_count(const RID &p_terrain) const;
            /// How far from the camera its regions are drawn [m]: the corner of the outer ring, past
            /// the draw distance - the range of its provider
            double terrain_get_range(const RID &p_terrain) const;
            /// The regions adopted and not freed yet
            int terrain_get_region_count(const RID &p_terrain) const;

            /// A region streaming supplies (the consumer of CONTENT_HEIGHTMAP), drawn from afar at once
            /// in the terrain it names, and in detail as the camera comes within FINE_RANGE_M
            RID region_adopt(const Ref<MaszynaHeightmapTerrainRegion> &p_region, const RID &p_scenario);
            void region_free(const RID &p_region);
    };
} // namespace godot
