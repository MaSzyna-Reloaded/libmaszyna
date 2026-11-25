#include "maszyna/terrain/MaszynaHeightmapTerrainRenderingServer.hpp"

#include "scenery/SceneryStreamingServer.hpp"

#include <godot_cpp/classes/rendering_server.hpp>
#include <godot_cpp/variant/packed_vector3_array.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <array>

namespace godot {
    namespace {
        constexpr double REGION_SIZE_M = SceneryStreamingServer::CHUNK_SIZE_M;
        constexpr int REGION_CHUNKS = MaszynaHeightmapTerrainRegion::CHUNKS_SIDE;
        /// The texels of a chunk's line in a region's chunk information: the palette indices of its
        /// first four layers and of the next four, the average weights of the first four and of the
        /// next four
        constexpr int CHUNK_INFO_TEXELS = 4;
        /// A palette index of no layer, the first one's of no chunk (heightmap::no_layer)
        constexpr uint8_t NO_LAYER = 255;
        /// The paint of a chunk, heightmap::default_paint_samples + 1 a side
        constexpr int PAINT_TEXELS = 251;
        /// No paint slot for a chunk
        constexpr float NO_PAINT = -1.0F;
        /// The rows of the palette texture: what a layer is, and its extras
        constexpr int PALETTE_ROWS = 2;
        /// A ring of the clipmap leaves out what the ring inside it covers: CLIPMAP_CELLS / 4 of its
        /// cells from its centre, less these - the inner ring may stand a cell off it
        constexpr int RING_HOLE_MARGIN = 2;
        /// The terrain's box reaches this low and this high [m]
        constexpr double BOX_LOW_M = -1000.0;
        constexpr double BOX_HEIGHT_M = 11000.0;
        /// What a region's detail preload holds, by index
        enum Preloaded {
            PRELOADED_HEIGHTS,
            PRELOADED_PAINT,
            PRELOADED_MAX,
        };
        /// The half side of the outer ring of the clipmap reaches its corner this many times farther
        constexpr double CORNER_FACTOR = 1.41421356237309504880;
    } // namespace

    /// The height of the chunk's surface at (p_x, p_z) metres from its corner, its grid's points
    /// interpolated
    static float chunk_height_at(const Ref<MaszynaHeightmapTerrainChunk> &p_chunk, const double p_x, const double p_z) {
        const double spacing = p_chunk->get_spacing();
        const double gx = p_x / spacing;
        const double gz = p_z / spacing;
        const int ix = static_cast<int>(Math::floor(gx));
        const int iz = static_cast<int>(Math::floor(gz));
        const auto fx = static_cast<float>(gx - ix);
        const auto fz = static_cast<float>(gz - iz);
        const float h00 = p_chunk->get_height(ix, iz);
        const float h10 = p_chunk->get_height(ix + 1, iz);
        const float h01 = p_chunk->get_height(ix, iz + 1);
        const float h11 = p_chunk->get_height(ix + 1, iz + 1);
        return Math::lerp(Math::lerp(h00, h10, fx), Math::lerp(h01, h11, fx), fz);
    }

    /// p_count blank layers of p_width x p_height
    static TypedArray<Image>
    blank_layers(const int p_count, const int p_width, const int p_height, const Image::Format p_format) {
        TypedArray<Image> layers;
        const Ref<Image> blank = Image::create_empty(p_width, p_height, false, p_format);
        for (int index = 0; index < p_count; index++) {
            layers.push_back(blank);
        }
        return layers;
    }

    /// The heights of a region of p_chunks, p_texels a side, each point from the chunk under it - the
    /// far edge's from the last chunk, which holds it as its own edge
    static Ref<Image> region_heights(const TypedArray<MaszynaHeightmapTerrainChunk> &p_chunks, const int p_texels) {
        const double step = REGION_SIZE_M / (p_texels - 1);
        const double chunk_size = MaszynaHeightmapTerrainChunk::CHUNK_SIZE_M;
        const Ref<Image> heights = Image::create_empty(p_texels, p_texels, false, Image::FORMAT_RF);
        for (int tz = 0; tz < p_texels; tz++) {
            const double z = tz * step;
            const int cz = MIN(static_cast<int>(z / chunk_size), REGION_CHUNKS - 1);
            for (int tx = 0; tx < p_texels; tx++) {
                const double x = tx * step;
                const int cx = MIN(static_cast<int>(x / chunk_size), REGION_CHUNKS - 1);
                const Ref<MaszynaHeightmapTerrainChunk> chunk = p_chunks[(cz * REGION_CHUNKS) + cx];
                if (chunk.is_valid()) {
                    heights->set_pixel(
                            tx, tz,
                            Color(chunk_height_at(chunk, x - (cx * chunk_size), z - (cz * chunk_size)), 0.0, 0.0));
                }
            }
        }
        return heights;
    }

    /// What each chunk of a region is: its layers and their average weights, a hole where there is
    /// none
    static Ref<Image> region_chunk_info(const TypedArray<MaszynaHeightmapTerrainChunk> &p_chunks) {
        const Ref<Image> info =
                Image::create_empty(REGION_CHUNKS * CHUNK_INFO_TEXELS, REGION_CHUNKS, false, Image::FORMAT_RGBA8);
        for (int index = 0; index < static_cast<int>(p_chunks.size()); index++) {
            const Ref<MaszynaHeightmapTerrainChunk> chunk = p_chunks[index];
            std::array<uint8_t, MaszynaHeightmapTerrainChunk::MAX_LAYERS> layers{};
            layers.fill(NO_LAYER);
            std::array<float, MaszynaHeightmapTerrainChunk::MAX_LAYERS> averages{};
            if (chunk.is_valid()) {
                const PackedInt32Array chunk_layers = chunk->get_layers();
                const PackedFloat32Array chunk_averages = chunk->get_average_weights();
                const int used = MIN(static_cast<int>(chunk_layers.size()), MaszynaHeightmapTerrainChunk::MAX_LAYERS);
                for (int layer = 0; layer < used; layer++) {
                    layers[layer] = static_cast<uint8_t>(MIN(chunk_layers[layer], NO_LAYER - 1));
                    averages[layer] = chunk_averages[layer];
                }
            }
            const auto byte = [](const uint8_t p_value) { return static_cast<float>(p_value) / NO_LAYER; };
            const int x = (index % REGION_CHUNKS) * CHUNK_INFO_TEXELS;
            const int z = index / REGION_CHUNKS;
            info->set_pixel(x, z, Color(byte(layers[0]), byte(layers[1]), byte(layers[2]), byte(layers[3])));
            info->set_pixel(x + 1, z, Color(byte(layers[4]), byte(layers[5]), byte(layers[6]), byte(layers[7])));
            info->set_pixel(x + 2, z, Color(averages[0], averages[1], averages[2], averages[3]));
            info->set_pixel(x + 3, z, Color(averages[4], averages[5], averages[6], averages[7]));
        }
        return info;
    }

    /// The rings of the clipmap from p_spacing a cell, each twice as coarse, until the outer one
    /// reaches p_draw_distance
    static int ring_count(const double p_spacing, const double p_draw_distance) {
        constexpr int HALF = MaszynaHeightmapTerrainRenderingServer::CLIPMAP_CELLS / 2;
        int count = 1;
        while (HALF * p_spacing * (1 << count) / 2.0 < p_draw_distance) {
            count++;
        }
        return count;
    }

    MaszynaHeightmapTerrainRenderingServer *MaszynaHeightmapTerrainRenderingServer::singleton = nullptr;

    void MaszynaHeightmapTerrainRenderingServer::_bind_methods() {
        ClassDB::bind_method(
                D_METHOD("terrain_create", "regions", "spacing", "look", "scenario"),
                &MaszynaHeightmapTerrainRenderingServer::terrain_create);
        ClassDB::bind_method(
                D_METHOD("terrain_free", "terrain"), &MaszynaHeightmapTerrainRenderingServer::terrain_free);
        ClassDB::bind_method(
                D_METHOD("terrain_set_scenario", "terrain", "scenario"),
                &MaszynaHeightmapTerrainRenderingServer::terrain_set_scenario);
        ClassDB::bind_method(
                D_METHOD("terrain_get_range", "terrain"), &MaszynaHeightmapTerrainRenderingServer::terrain_get_range);
        ClassDB::bind_method(
                D_METHOD("terrain_get_region_count", "terrain"),
                &MaszynaHeightmapTerrainRenderingServer::terrain_get_region_count);
        ClassDB::bind_method(
                D_METHOD("region_adopt", "region", "scenario"), &MaszynaHeightmapTerrainRenderingServer::region_adopt);
        ClassDB::bind_method(D_METHOD("region_free", "region"), &MaszynaHeightmapTerrainRenderingServer::region_free);
        ClassDB::bind_method(
                D_METHOD("terrain_get_ring_count", "terrain"),
                &MaszynaHeightmapTerrainRenderingServer::terrain_get_ring_count);
        BIND_CONSTANT(COARSE_TEXELS);
        BIND_CONSTANT(PAINT_SLOTS);
        BIND_CONSTANT(CLIPMAP_CELLS);
    }

    MaszynaHeightmapTerrainRenderingServer *MaszynaHeightmapTerrainRenderingServer::get_instance() {
        return singleton;
    }

    /// The consumer of the regions streaming supplies, each streamed again at its levels of detail
    MaszynaHeightmapTerrainRenderingServer::MaszynaHeightmapTerrainRenderingServer() {
        singleton = this;
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        ERR_FAIL_NULL(streaming);
        streaming->content_set_consumer(
                SceneryStreamingProvider::CONTENT_HEIGHTMAP,
                callable_mp(this, &MaszynaHeightmapTerrainRenderingServer::region_adopt),
                callable_mp(this, &MaszynaHeightmapTerrainRenderingServer::region_free));
        stream_owner = streaming->owner_create(
                "heightmap terrain", callable_mp(this, &MaszynaHeightmapTerrainRenderingServer::_stream_preload),
                callable_mp(this, &MaszynaHeightmapTerrainRenderingServer::_stream_build),
                callable_mp(this, &MaszynaHeightmapTerrainRenderingServer::_stream_clear));
    }

    MaszynaHeightmapTerrainRenderingServer::~MaszynaHeightmapTerrainRenderingServer() {
        if (singleton == this) {
            singleton = nullptr;
        }
    }

    RID MaszynaHeightmapTerrainRenderingServer::terrain_create(
            const TypedArray<Vector2i> &p_regions, const double p_spacing,
            const Ref<MaszynaHeightmapTerrainLook> &p_look, const RID &p_scenario) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        RenderingServer *rendering = RenderingServer::get_singleton();
        ERR_FAIL_NULL_V(streaming, RID());
        ERR_FAIL_COND_V(p_look.is_null() || p_look->get_shader().is_null() || p_spacing <= 0.0, RID());
        ERR_FAIL_COND_V_MSG(p_regions.is_empty(), RID(), "A heightmap terrain of no regions");
        Terrain terrain;
        terrain.look = p_look;
        terrain.scenario = p_scenario;

        // the regions it can hold
        Vector2i low(INT32_MAX, INT32_MAX);
        Vector2i high(INT32_MIN, INT32_MIN);
        for (const Variant &region: p_regions) {
            low = low.min(region);
            high = high.max(region);
        }
        terrain.region_origin = low;
        terrain.region_count = high - low + Vector2i(1, 1);
        terrain.fine_texels = static_cast<int>(Math::round(REGION_SIZE_M / p_spacing)) + 1;
        terrain.region_map =
                Image::create_empty(terrain.region_count.x, terrain.region_count.y, false, Image::FORMAT_RGF);
        terrain.region_map_texture = rendering->texture_2d_create(terrain.region_map);

        // the rings reach past the draw distance, and the regions as far as the outer ring's corners
        const double draw_distance = streaming->streaming_get_draw_distance();
        const int rings = ring_count(p_spacing, draw_distance);
        terrain.range = CLIPMAP_CELLS / 2.0 * p_spacing * (1 << (rings - 1)) * CORNER_FACTOR;
        // as many regions in the textures as the streaming keeps at once: the range's and
        // FINE_RANGE_M's squares of regions, kept a while after they are left (HYSTERESIS_M)
        const auto square = [](const double p_range) {
            const int side =
                    (2 *
                     static_cast<int>(Math::ceil((p_range + SceneryStreamingServer::HYSTERESIS_M) / REGION_SIZE_M))) +
                    1;
            return side * side;
        };
        const int coarse_capacity = MIN(square(terrain.range), static_cast<int>(p_regions.size()));
        const int fine_capacity = MIN(square(FINE_RANGE_M), static_cast<int>(p_regions.size()));
        terrain.coarse_heights = rendering->texture_2d_layered_create(
                blank_layers(coarse_capacity, COARSE_TEXELS, COARSE_TEXELS, Image::FORMAT_RF),
                RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
        terrain.chunk_info = rendering->texture_2d_layered_create(
                blank_layers(coarse_capacity, REGION_CHUNKS * CHUNK_INFO_TEXELS, REGION_CHUNKS, Image::FORMAT_RGBA8),
                RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
        terrain.fine_heights = rendering->texture_2d_layered_create(
                blank_layers(fine_capacity, terrain.fine_texels, terrain.fine_texels, Image::FORMAT_RF),
                RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
        terrain.paint_info = rendering->texture_2d_layered_create(
                blank_layers(fine_capacity, REGION_CHUNKS, REGION_CHUNKS, Image::FORMAT_RF),
                RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
        terrain.paint_weights0 = rendering->texture_2d_layered_create(
                blank_layers(PAINT_SLOTS, PAINT_TEXELS, PAINT_TEXELS, Image::FORMAT_RGBA8),
                RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
        terrain.paint_weights1 = rendering->texture_2d_layered_create(
                blank_layers(PAINT_SLOTS, PAINT_TEXELS, PAINT_TEXELS, Image::FORMAT_RGBA8),
                RenderingServer::TEXTURE_LAYERED_2D_ARRAY);
        for (int slot = coarse_capacity - 1; slot >= 0; slot--) {
            terrain.coarse_free.push_back(slot);
        }
        for (int slot = fine_capacity - 1; slot >= 0; slot--) {
            terrain.fine_free.push_back(slot);
        }
        for (int slot = PAINT_SLOTS - 1; slot >= 0; slot--) {
            terrain.paint_free.push_back(slot);
        }

        // what each layer of the palette is, a column of the palette texture
        const PackedFloat32Array palette = p_look->get_palette();
        const int palette_size = MAX(1, static_cast<int>(palette.size() / MaszynaHeightmapTerrainLook::PALETTE_FLOATS));
        const Ref<Image> palette_image = Image::create_empty(palette_size, PALETTE_ROWS, false, Image::FORMAT_RGBAF);
        for (int layer = 0; layer < palette_size; layer++) {
            const int64_t at = static_cast<int64_t>(layer) * MaszynaHeightmapTerrainLook::PALETTE_FLOATS;
            if (at + MaszynaHeightmapTerrainLook::PALETTE_FLOATS > palette.size()) {
                break;
            }
            palette_image->set_pixel(layer, 0, Color(palette[at], palette[at + 1], palette[at + 2], palette[at + 3]));
            palette_image->set_pixel(
                    layer, 1, Color(palette[at + 4], palette[at + 5], palette[at + 6], palette[at + 7]));
        }
        terrain.palette = rendering->texture_2d_create(palette_image);
        _make_clipmap(terrain, p_spacing, rings);

        const RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        MutexLock lock(mutex);
        terrains.insert(rid, terrain);
        return rid;
    }

    void MaszynaHeightmapTerrainRenderingServer::_make_clipmap(
            Terrain &p_terrain, const double p_spacing, const int p_rings) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        const Ref<MaszynaHeightmapTerrainLook> &look = p_terrain.look;
        const double coarse_spacing = REGION_SIZE_M / (COARSE_TEXELS - 1);
        const AABB box(
                Vector3(static_cast<real_t>(p_terrain.region_origin.x * REGION_SIZE_M), static_cast<real_t>(BOX_LOW_M),
                        static_cast<real_t>(p_terrain.region_origin.y * REGION_SIZE_M)),
                Vector3(static_cast<real_t>(p_terrain.region_count.x * REGION_SIZE_M),
                        static_cast<real_t>(BOX_HEIGHT_M),
                        static_cast<real_t>(p_terrain.region_count.y * REGION_SIZE_M)));
        constexpr int HALF = CLIPMAP_CELLS / 2;
        constexpr int HOLE = (CLIPMAP_CELLS / 4) - RING_HOLE_MARGIN;
        // the grid in cells around a ring's centre - the shader places it and raises its points
        PackedVector3Array vertices;
        for (int z = 0; z <= CLIPMAP_CELLS; z++) {
            for (int x = 0; x <= CLIPMAP_CELLS; x++) {
                vertices.push_back(Vector3(static_cast<real_t>(x - HALF), 0.0, static_cast<real_t>(z - HALF)));
            }
        }
        const auto at = [](const int p_x, const int p_z) { return (p_z * (CLIPMAP_CELLS + 1)) + p_x; };
        // a ring each twice as coarse as the one inside it
        for (int ring = 0; ring < p_rings; ring++) {
            const bool first = ring == 0;
            const double spacing = p_spacing * (1 << ring);
            PackedInt32Array indices;
            for (int z = 0; z < CLIPMAP_CELLS; z++) {
                for (int x = 0; x < CLIPMAP_CELLS; x++) {
                    // what the ring inside it covers
                    if (!first && x >= HALF - HOLE && x < HALF + HOLE && z >= HALF - HOLE && z < HALF + HOLE) {
                        continue;
                    }
                    // Godot's front faces turn clockwise seen from above
                    indices.append_array(PackedInt32Array({at(x, z), at(x + 1, z), at(x, z + 1)}));
                    indices.append_array(PackedInt32Array({at(x + 1, z + 1), at(x, z + 1), at(x + 1, z)}));
                }
            }
            Array arrays;
            arrays.resize(RenderingServer::ARRAY_MAX);
            arrays[RenderingServer::ARRAY_VERTEX] = vertices;
            arrays[RenderingServer::ARRAY_INDEX] = indices;
            const RID mesh = rendering->mesh_create();
            rendering->mesh_add_surface_from_arrays(mesh, RenderingServer::PRIMITIVE_TRIANGLES, arrays);
            const RID material = rendering->material_create();
            rendering->material_set_shader(material, look->get_shader()->get_rid());
            rendering->material_set_param(material, "ring_spacing", spacing);
            rendering->material_set_param(material, "ring_coarse", spacing >= coarse_spacing);
            rendering->material_set_param(material, "next_coarse", spacing * 2.0 >= coarse_spacing);
            rendering->material_set_param(material, "ring_last", ring == p_rings - 1);
            rendering->material_set_param(material, "region_map", p_terrain.region_map_texture);
            rendering->material_set_param(
                    material, "region_origin",
                    Vector2(static_cast<real_t>(p_terrain.region_origin.x * REGION_SIZE_M),
                            static_cast<real_t>(p_terrain.region_origin.y * REGION_SIZE_M)));
            rendering->material_set_param(material, "coarse_heights", p_terrain.coarse_heights);
            rendering->material_set_param(material, "chunk_info", p_terrain.chunk_info);
            rendering->material_set_param(material, "fine_heights", p_terrain.fine_heights);
            rendering->material_set_param(material, "fine_texels", p_terrain.fine_texels);
            rendering->material_set_param(material, "paint_info", p_terrain.paint_info);
            rendering->material_set_param(material, "paint_weights0", p_terrain.paint_weights0);
            rendering->material_set_param(material, "paint_weights1", p_terrain.paint_weights1);
            rendering->material_set_param(material, "palette", p_terrain.palette);
            rendering->material_set_param(material, "layer_diffuse", look->get_diffuse_layers()->get_rid());
            rendering->material_set_param(material, "layer_normal", look->get_normal_layers()->get_rid());
            rendering->material_set_param(material, "layer_specgloss", look->get_specgloss_layers()->get_rid());
            rendering->mesh_surface_set_material(mesh, 0, material);
            const RID instance = rendering->instance_create2(mesh, p_terrain.scenario);
            // drawn wherever the camera is: the shader follows it
            rendering->instance_set_custom_aabb(instance, box);
            p_terrain.meshes.push_back(mesh);
            p_terrain.materials.push_back(material);
            p_terrain.instances.push_back(instance);
        }
    }

    void MaszynaHeightmapTerrainRenderingServer::terrain_free(const RID &p_terrain) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        LocalVector<RID> adopted;
        {
            MutexLock lock(mutex);
            const Terrain *terrain = terrains.getptr(p_terrain);
            ERR_FAIL_NULL(terrain);
            adopted = terrain->regions;
        }
        // regions its provider has not let go of yet
        for (const RID &region: adopted) {
            region_free(region);
        }
        MutexLock lock(mutex);
        Terrain *terrain = terrains.getptr(p_terrain);
        for (const RID &instance: terrain->instances) {
            rendering->free_rid(instance);
        }
        for (const RID &mesh: terrain->meshes) {
            rendering->free_rid(mesh);
        }
        for (const RID &material: terrain->materials) {
            rendering->free_rid(material);
        }
        for (const RID &texture:
             {terrain->region_map_texture, terrain->coarse_heights, terrain->chunk_info, terrain->fine_heights,
              terrain->paint_info, terrain->paint_weights0, terrain->paint_weights1, terrain->palette}) {
            rendering->free_rid(texture);
        }
        terrains.erase(p_terrain);
    }

    void MaszynaHeightmapTerrainRenderingServer::terrain_set_scenario(const RID &p_terrain, const RID &p_scenario) {
        RenderingServer *rendering = RenderingServer::get_singleton();
        MutexLock lock(mutex);
        Terrain *terrain = terrains.getptr(p_terrain);
        ERR_FAIL_NULL(terrain);
        terrain->scenario = p_scenario;
        for (const RID &instance: terrain->instances) {
            rendering->instance_set_scenario(instance, p_scenario);
        }
    }

    int MaszynaHeightmapTerrainRenderingServer::terrain_get_ring_count(const RID &p_terrain) const {
        MutexLock lock(mutex);
        const Terrain *terrain = terrains.getptr(p_terrain);
        ERR_FAIL_NULL_V(terrain, 0);
        return static_cast<int>(terrain->instances.size());
    }

    double MaszynaHeightmapTerrainRenderingServer::terrain_get_range(const RID &p_terrain) const {
        MutexLock lock(mutex);
        const Terrain *terrain = terrains.getptr(p_terrain);
        ERR_FAIL_NULL_V(terrain, 0.0);
        return terrain->range;
    }

    int MaszynaHeightmapTerrainRenderingServer::terrain_get_region_count(const RID &p_terrain) const {
        MutexLock lock(mutex);
        const Terrain *terrain = terrains.getptr(p_terrain);
        ERR_FAIL_NULL_V(terrain, 0);
        return static_cast<int>(terrain->regions.size());
    }

    /// Drawn from afar at once - its heights COARSE_TEXELS a side and what its chunks are, made of
    /// the chunks streaming has read - and in detail within FINE_RANGE_M, a piece streamed by that
    /// range
    RID MaszynaHeightmapTerrainRenderingServer::region_adopt(
            const Ref<MaszynaHeightmapTerrainRegion> &p_region, const RID & /*p_scenario*/) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        RenderingServer *rendering = RenderingServer::get_singleton();
        ERR_FAIL_NULL_V(streaming, RID());
        ERR_FAIL_COND_V(p_region.is_null(), RID());
        const TypedArray<MaszynaHeightmapTerrainChunk> chunks = p_region->get_chunks();
        const Ref<Image> heights = region_heights(chunks, COARSE_TEXELS);
        const Ref<Image> info = region_chunk_info(chunks);
        RID rid = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
        Region region;
        region.terrain = p_region->get_terrain();
        region.key = p_region->get_key();
        {
            MutexLock lock(mutex);
            Terrain *terrain = terrains.getptr(region.terrain);
            ERR_FAIL_NULL_V_MSG(terrain, RID(), "A heightmap terrain region of no terrain");
            const Vector2i at = p_region->get_key() - terrain->region_origin;
            ERR_FAIL_COND_V(
                    at.x < 0 || at.y < 0 || at.x >= terrain->region_count.x || at.y >= terrain->region_count.y, RID());
            ERR_FAIL_COND_V_MSG(
                    terrain->coarse_free.is_empty(), RID(), "Heightmap terrain: no room for another region");
            region.coarse_slot = terrain->coarse_free[terrain->coarse_free.size() - 1];
            terrain->coarse_free.resize(terrain->coarse_free.size() - 1);
            rendering->texture_2d_update(terrain->coarse_heights, heights, region.coarse_slot);
            rendering->texture_2d_update(terrain->chunk_info, info, region.coarse_slot);
            _set_region_slot(*terrain, p_region->get_key(), false, region.coarse_slot);
            Piece piece;
            piece.terrain = region.terrain;
            piece.region = p_region;
            region.fine = UtilityFunctions::rid_from_int64(UtilityFunctions::rid_allocate_id());
            pieces.insert(region.fine, piece);
            terrain->regions.push_back(rid);
            regions.insert(rid, region);
        }
        // registered once everything its preload reads is in place
        const Vector2i key = p_region->get_key();
        const Vector3 centre(
                static_cast<real_t>((key.x + 0.5) * REGION_SIZE_M), 0.0,
                static_cast<real_t>((key.y + 0.5) * REGION_SIZE_M));
        const RID fine =
                streaming->stream_register(stream_owner, region.fine, centre, static_cast<float>(FINE_RANGE_M));
        MutexLock lock(mutex);
        pieces.getptr(region.fine)->stream = fine;
        return rid;
    }

    void MaszynaHeightmapTerrainRenderingServer::region_free(const RID &p_region) {
        SceneryStreamingServer *streaming = SceneryStreamingServer::get_instance();
        Region region;
        RID stream;
        {
            MutexLock lock(mutex);
            const Region *adopted = regions.getptr(p_region);
            if (adopted == nullptr) {
                return;
            }
            region = *adopted;
            stream = pieces.getptr(region.fine)->stream;
        }
        // the streaming clears the detail as it lets go of it
        if (streaming != nullptr && stream.is_valid()) {
            streaming->stream_free(stream);
        }
        MutexLock lock(mutex);
        pieces.erase(region.fine);
        regions.erase(p_region);
        Terrain *terrain = terrains.getptr(region.terrain);
        if (terrain != nullptr) {
            terrain->regions.erase(p_region);
            terrain->coarse_free.push_back(region.coarse_slot);
            _set_region_slot(*terrain, region.key, false, -1);
        }
    }

    /// The streaming worker: the region's heights at the terrain's spacing and its painted chunks'
    /// paint
    Variant MaszynaHeightmapTerrainRenderingServer::_stream_preload(const RID &p_piece) {
        int fine_texels = 0;
        TypedArray<MaszynaHeightmapTerrainChunk> chunks;
        {
            MutexLock lock(mutex);
            const Piece *piece = pieces.getptr(p_piece);
            if (piece == nullptr) {
                return {};
            }
            const Terrain *terrain = terrains.getptr(piece->terrain);
            if (terrain == nullptr) {
                return {};
            }
            fine_texels = terrain->fine_texels;
            chunks = piece->region->get_chunks();
        }
        Array preloaded;
        preloaded.resize(PRELOADED_MAX);
        preloaded[PRELOADED_HEIGHTS] = region_heights(chunks, fine_texels);
        // the paint of its painted chunks: their index in the region and their weights
        Array paint;
        for (int index = 0; index < static_cast<int>(chunks.size()); index++) {
            const Ref<MaszynaHeightmapTerrainChunk> chunk = chunks[index];
            if (chunk.is_null() || chunk->get_paint() <= 0 || chunk->get_layers().size() <= 1) {
                continue;
            }
            const Array images = chunk->build_weight_images(PAINT_TEXELS);
            for (int64_t image = 0; image < images.size(); image++) {
                const Ref<Image> weights = images[image];
                if (weights->get_width() != PAINT_TEXELS) {
                    weights->resize(PAINT_TEXELS, PAINT_TEXELS, Image::INTERPOLATE_BILINEAR);
                }
            }
            paint.push_back(Array::make(index, images[0], images[1]));
        }
        preloaded[PRELOADED_PAINT] = paint;
        return preloaded;
    }

    void MaszynaHeightmapTerrainRenderingServer::_set_region_slot(
            Terrain &p_terrain, const Vector2i &p_region, const bool p_fine, const int p_slot) {
        const Vector2i at = p_region - p_terrain.region_origin;
        Color slots = p_terrain.region_map->get_pixelv(at);
        // a slot as the shader reads it, 0 none
        (p_fine ? slots.g : slots.r) = static_cast<float>(p_slot + 1);
        p_terrain.region_map->set_pixelv(at, slots);
        RenderingServer::get_singleton()->texture_2d_update(p_terrain.region_map_texture, p_terrain.region_map, 0);
    }

    void MaszynaHeightmapTerrainRenderingServer::_stream_build(const RID &p_piece, const Variant &p_preloaded) {
        const Array preloaded = p_preloaded;
        if (preloaded.size() != PRELOADED_MAX) {
            return;
        }
        RenderingServer *rendering = RenderingServer::get_singleton();
        MutexLock lock(mutex);
        Piece *piece = pieces.getptr(p_piece);
        if (piece == nullptr || piece->slot >= 0) {
            return;
        }
        Terrain *terrain = terrains.getptr(piece->terrain);
        if (terrain == nullptr) {
            return;
        }
        ERR_FAIL_COND_MSG(terrain->fine_free.is_empty(), "Heightmap terrain: no room for another region");
        piece->slot = terrain->fine_free[terrain->fine_free.size() - 1];
        terrain->fine_free.resize(terrain->fine_free.size() - 1);
        rendering->texture_2d_update(terrain->fine_heights, preloaded[PRELOADED_HEIGHTS], piece->slot);
        const Ref<Image> paint_info = Image::create_empty(REGION_CHUNKS, REGION_CHUNKS, false, Image::FORMAT_RF);
        paint_info->fill(Color(NO_PAINT, 0.0, 0.0));
        const Array paint = preloaded[PRELOADED_PAINT];
        for (int64_t index = 0; index < paint.size(); index++) {
            if (terrain->paint_free.is_empty()) {
                // the chunk is drawn with its average weights, as from afar
                WARN_PRINT_ONCE("Heightmap terrain: more painted chunks near than PAINT_SLOTS");
                break;
            }
            const Array chunk = paint[index];
            const int slot = terrain->paint_free[terrain->paint_free.size() - 1];
            terrain->paint_free.resize(terrain->paint_free.size() - 1);
            piece->paint_slots.push_back(slot);
            rendering->texture_2d_update(terrain->paint_weights0, chunk[1], slot);
            rendering->texture_2d_update(terrain->paint_weights1, chunk[2], slot);
            const int chunk_index = chunk[0];
            paint_info->set_pixel(
                    chunk_index % REGION_CHUNKS, chunk_index / REGION_CHUNKS,
                    Color(static_cast<float>(slot), 0.0, 0.0));
        }
        rendering->texture_2d_update(terrain->paint_info, paint_info, piece->slot);
        _set_region_slot(*terrain, piece->region->get_key(), true, piece->slot);
    }

    void MaszynaHeightmapTerrainRenderingServer::_stream_clear(const RID &p_piece) {
        MutexLock lock(mutex);
        Piece *piece = pieces.getptr(p_piece);
        if (piece == nullptr || piece->slot < 0) {
            return;
        }
        Terrain *terrain = terrains.getptr(piece->terrain);
        if (terrain == nullptr) {
            return;
        }
        terrain->fine_free.push_back(piece->slot);
        for (const int slot: piece->paint_slots) {
            terrain->paint_free.push_back(slot);
        }
        piece->paint_slots.clear();
        piece->slot = -1;
        _set_region_slot(*terrain, piece->region->get_key(), true, -1);
    }
} // namespace godot
