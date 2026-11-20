extends MaszynaGutTest

## The original's heightmap terrain (`heightmap_terrain`, scene/heightmapterrain.h): its pack files
## (.tch) read chunk by chunk - their slices stored or zstd-compressed, checked by their CRC-32, the
## heights delta coded in steps or kept as floats, the paint planar per layer - its terrain.yaml, the
## directive that names it, the provider supplying it region by region and the clipmap drawing the
## regions. The fixture is laid out as heightmap::write_pack() writes a pack.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const TERRAIN_DIR:String = "res://tests/fixtures/terrain/fixture_terrain"
## The fixture's chunks: two cells a side, 125 m apart
const PAINTED:Vector2i = Vector2i(0, 0)
const PLAIN:Vector2i = Vector2i(1, 0)
const DAMAGED:Vector2i = Vector2i(2, 0)
const CELLS:int = 2
## heightmap::chunk_size [m]
const CHUNK_SIZE:float = 250.0
## The painted chunk's heights: 1 m and 0.5 m more at each next point; the plain one's all 7.25 m
const FIRST_HEIGHT:float = 1.0
const HEIGHT_STEP:float = 0.5
const PLAIN_HEIGHT:float = 7.25
## The painted chunk's first layer covers its left column of 3 x 3 paint samples, the second the rest
const PAINT_SAMPLES:int = 2
const FIRST_LAYER_SHARE:float = 1.0 / 3.0
## The manifest's spacing and its palette
const SPACING:float = 5.0
const PALETTE:Array[String] = ["grass", "@fixture_quoted", "materials/fixture_path"]

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _directory() -> String:
    return ProjectSettings.globalize_path(TERRAIN_DIR)


func _pack() -> MaszynaHeightmapTerrainPack:
    var pack := MaszynaHeightmapTerrainPack.new()
    assert_true(pack.open(_directory().path_join("0_0.tch")))
    return pack


func test_a_pack_lists_its_chunks() -> void:
    var pack:MaszynaHeightmapTerrainPack = _pack()
    assert_eq(pack.get_key(), Vector2i.ZERO)
    assert_eq(pack.get_chunk_keys(), [PAINTED, PLAIN, DAMAGED] as Array[Vector2i])


func test_a_file_of_another_kind_is_no_pack() -> void:
    var pack := MaszynaHeightmapTerrainPack.new()
    assert_false(pack.open(_directory().path_join("terrain.yaml")))
    assert_push_error("no terrain chunks file")


## A compressed chunk: its heights in steps of 5 mm delta coded, its two layers painted
func test_a_compressed_chunk_is_read_with_its_paint() -> void:
    var chunk:MaszynaHeightmapTerrainChunk = _pack().read_chunk(PAINTED)
    assert_not_null(chunk)
    assert_eq(chunk.get_cells(), CELLS)
    assert_eq(chunk.get_spacing(), CHUNK_SIZE / CELLS)
    for index:int in (CELLS + 1) * (CELLS + 1):
        assert_almost_eq(chunk.get_heights()[index], FIRST_HEIGHT + HEIGHT_STEP * index, 0.001)
    assert_eq(chunk.get_paint(), PAINT_SAMPLES)
    assert_eq(chunk.get_layers(), PackedInt32Array([0, 1]))
    var averages:PackedFloat32Array = chunk.get_average_weights()
    assert_almost_eq(averages[0], FIRST_LAYER_SHARE, 0.001)
    assert_almost_eq(averages[1], 1.0 - FIRST_LAYER_SHARE, 0.001)
    var images:Array = chunk.build_weight_images(0)
    assert_eq((images[0] as Image).get_size(), Vector2i(PAINT_SAMPLES + 1, PAINT_SAMPLES + 1))
    assert_eq((images[0] as Image).get_pixel(0, 1), Color(1.0, 0.0, 0.0, 0.0), "the left column the first layer's")
    assert_eq((images[0] as Image).get_pixel(2, 1), Color(0.0, 1.0, 0.0, 0.0), "the rest the second's")


## A stored chunk of float heights and one layer: covered with it alone
func test_a_stored_chunk_without_paint_is_its_first_layer() -> void:
    var chunk:MaszynaHeightmapTerrainChunk = _pack().read_chunk(PLAIN)
    assert_not_null(chunk)
    assert_eq(chunk.get_height(1, 1), PLAIN_HEIGHT)
    assert_eq(chunk.get_layers(), PackedInt32Array([2]))
    assert_eq(chunk.get_average_weights()[0], 1.0)
    assert_eq((chunk.build_weight_images(0)[0] as Image).get_size(), Vector2i.ONE)


func test_a_slice_of_a_wrong_checksum_is_damaged() -> void:
    assert_null(_pack().read_chunk(DAMAGED))
    assert_engine_error("heights.size() != count")
    assert_null(_pack().read_chunk(Vector2i(5, 5)), "no chunk there")


func test_the_manifest_gives_the_spacing_and_the_palette() -> void:
    var manifest:MaszynaHeightmapTerrainManifest = MaszynaHeightmapTerrainManifest.read(_directory())
    assert_eq(manifest.spacing, SPACING)
    assert_eq(manifest.layers, PALETTE)
    assert_null(MaszynaHeightmapTerrainManifest.read(_directory().path_join("missing")))


func test_the_directive_names_the_terrain_folder() -> void:
    var context := MaszynaImporterContext.new()
    MaszynaSceneryProvider.parse_file("heightmap_terrain.scn", {}, context)
    var expected:Array[String] = [MaszynaGameDirectory.get_game_dir().path_join("terrain/fixture_terrain")]
    assert_eq(context.heightmap_terrains, expected)


func _provider() -> MaszynaHeightmapTerrainProvider:
    var provider := MaszynaHeightmapTerrainProvider.new()
    assert_true(provider.open(_directory()))
    return provider


## The fixture's three chunks lie in one region, the streaming cell of 4 x 4 chunks
func test_the_provider_lists_the_regions_of_its_chunks() -> void:
    var provider:MaszynaHeightmapTerrainProvider = _provider()
    assert_eq(provider.get_chunk_count(), 3)
    assert_eq(provider.get_chunk_cells(), [Vector2i.ZERO] as Array[Vector2i])
    assert_eq(provider.get_content_kinds(), PackedInt32Array([SceneryStreamingProvider.CONTENT_HEIGHTMAP]))


func test_a_folder_without_packs_supplies_nothing() -> void:
    var provider := MaszynaHeightmapTerrainProvider.new()
    assert_false(provider.open(_directory().path_join("missing")))
    assert_engine_error("No heightmap terrain chunks")


## A region holds its chunks in rows, a hole where there is none or it is damaged, and names the
## terrain it is drawn in
func test_a_region_is_supplied_with_its_chunks_and_holes() -> void:
    var provider:MaszynaHeightmapTerrainProvider = _provider()
    var terrain:RID = rid_from_int64(rid_allocate_id())
    provider.set_terrain(terrain)
    var items:Array = provider.chunk_load(Vector2i.ZERO, SceneryStreamingProvider.CONTENT_HEIGHTMAP)
    assert_engine_error("heights.size() != count")
    assert_eq(items.size(), 1)
    var region:MaszynaHeightmapTerrainRegion = items[0]
    assert_eq(region.get_terrain(), terrain)
    assert_eq(region.get_key(), Vector2i.ZERO)
    var chunks:Array = region.get_chunks()
    var side:int = MaszynaHeightmapTerrainRegion.CHUNKS_SIDE
    assert_eq(chunks.size(), side * side)
    assert_eq((chunks[PAINTED.y * side + PAINTED.x] as MaszynaHeightmapTerrainChunk).get_key(), PAINTED)
    assert_eq((chunks[PLAIN.y * side + PLAIN.x] as MaszynaHeightmapTerrainChunk).get_key(), PLAIN)
    assert_null(chunks[DAMAGED.y * side + DAMAGED.x], "a damaged chunk is a hole")
    assert_null(chunks[side * side - 1], "no chunk there")
    assert_eq(provider.chunk_load(Vector2i.ZERO, SceneryStreamingProvider.CONTENT_TERRAIN), [])


## The clipmap's rings go from the terrain's spacing, each twice as coarse, until the outer one
## reaches the draw distance; the terrain's regions are wanted as far as the outer ring's corner, past
## the draw distance, and the regions streaming supplies are drawn in the terrain they name until they
## are let go of
func test_the_terrain_draws_the_regions_supplied_with_rings_to_the_draw_distance() -> void:
    var manifest:MaszynaHeightmapTerrainManifest = MaszynaHeightmapTerrainManifest.read(_directory())
    var look:MaszynaHeightmapTerrainLook = MaszynaHeightmapTerrainLooks.make(manifest)
    assert_eq(look.palette.size(), PALETTE.size() * MaszynaHeightmapTerrainLook.PALETTE_FLOATS)
    var provider:MaszynaHeightmapTerrainProvider = _provider()
    var terrain:RID = MaszynaHeightmapTerrainRenderingServer.terrain_create(
        provider.get_chunk_cells(), manifest.spacing, look, get_tree().root.get_world_3d().scenario)
    assert_true(terrain.is_valid())
    var rings:int = 1
    var half:float = MaszynaHeightmapTerrainRenderingServer.CLIPMAP_CELLS / 2.0 * SPACING
    while half < SceneryStreamingServer.streaming_get_draw_distance():
        half *= 2.0
        rings += 1
    assert_eq(MaszynaHeightmapTerrainRenderingServer.terrain_get_ring_count(terrain), rings)
    var terrain_range:float = MaszynaHeightmapTerrainRenderingServer.terrain_get_range(terrain)
    assert_almost_eq(terrain_range, half * sqrt(2.0), 0.01, "the outer ring's corner")
    assert_gt(terrain_range, SceneryStreamingServer.streaming_get_draw_distance())
    provider.set_range(terrain_range)
    assert_almost_eq(provider.get_range(), terrain_range, 0.01, "the provider's regions wanted as far")

    provider.set_terrain(terrain)
    var region:MaszynaHeightmapTerrainRegion = provider.chunk_load(
        Vector2i.ZERO, SceneryStreamingProvider.CONTENT_HEIGHTMAP)[0]
    assert_engine_error("heights.size() != count")
    var adopted:RID = MaszynaHeightmapTerrainRenderingServer.region_adopt(region, RID())
    assert_true(adopted.is_valid())
    assert_eq(MaszynaHeightmapTerrainRenderingServer.terrain_get_region_count(terrain), 1)
    MaszynaHeightmapTerrainRenderingServer.region_free(adopted)
    assert_eq(MaszynaHeightmapTerrainRenderingServer.terrain_get_region_count(terrain), 0)
    MaszynaHeightmapTerrainRenderingServer.terrain_free(terrain)
