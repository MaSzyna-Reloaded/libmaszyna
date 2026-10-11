extends MaszynaGutTest

## Large parameterless includes are parsed as subscenes cached per inherited origin/rotate; a cached
## subscene gives the same result as parsing it in place.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const ROOT:String = "subscene/root.scn"

var _previous_game_dir:String = ""
var _cache_dir:String = "user://cache".path_join(SceneryInstancer.CACHE_DIRECTORY)
var _existing_files:PackedStringArray = []
var _existing_directories:PackedStringArray = []
var _previous_lazy_loading:bool = false


func before_each() -> void:
    # the terrain is streamed from its chunk files, loaded when wanted
    _previous_lazy_loading = ResourceLazyLoader.lazy_loading
    ResourceLazyLoader.lazy_loading = true
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    _existing_files = _list_cache_files()
    _existing_directories = DirAccess.get_directories_at(_cache_dir)


func after_each() -> void:
    for file:String in _new_cache_files():
        DirAccess.remove_absolute(_cache_dir.path_join(file))
    # the terrain chunks of a parsed scenery, a directory beside its cache entry
    for directory:String in DirAccess.get_directories_at(_cache_dir):
        if _existing_directories.has(directory):
            continue
        for file:String in DirAccess.get_files_at(_cache_dir.path_join(directory)):
            DirAccess.remove_absolute(_cache_dir.path_join(directory).path_join(file))
        DirAccess.remove_absolute(_cache_dir.path_join(directory))
    UserSettings.save_maszyna_game_dir(_previous_game_dir)
    ResourceLazyLoader.lazy_loading = _previous_lazy_loading


func test_subscenes_are_cached_per_origin_and_match_in_place_parsing() -> void:
    var in_place := MaszynaImporterContext.new()
    SceneryInstancer.parse_file(ROOT, {}, in_place)

    var parsed:MaszynaImporterContext = _parse_threaded()
    # big.scm twice without parameters (two origins); with parameters and small.inc are not cached
    assert_eq(_new_cache_files().size(), 2)
    var cached:MaszynaImporterContext = _parse_threaded()
    assert_eq(_new_cache_files().size(), 2)

    var expected:Array = _describe(in_place)
    assert_eq(expected[0].size(), 7)
    # big.scm's triangle straddles both grid lines through the origin: four cells, which its three
    # placements share
    assert_eq(expected[1].size(), 4)
    assert_eq(_describe(parsed), expected)
    assert_eq(_describe(cached), expected)
    assert_eq(cached.dependencies.keys().size(), 3)
    assert_true(cached.cacheable)


## The parse writes the terrain out chunk by chunk, beside the cache entries of the scenery and of its
## subscenes, and the scenery streams it
## from those files (ResourceLazyLoader), letting go of every one as it is freed
func test_loaded_scenery_writes_its_terrain_chunks_as_files() -> void:
    var registered:int = ResourceLazyLoader.resource_get_statistics()["registered"]
    var scenery := MaszynaIncludeNode.new()
    scenery.use_cache = false
    scenery.filename = ROOT
    add_child(scenery)
    if not await wait_loaded(scenery.loaded, scenery.filename):
        return
    var chunk_files:int = 0
    for directory:String in DirAccess.get_directories_at(_cache_dir):
        if not _existing_directories.has(directory):
            chunk_files += DirAccess.get_files_at(_cache_dir.path_join(directory)).size()
    # one file per texture, cell and range: the scenery's four, and each cached subscene's own - big.scm
    # at the origin straddles four cells but has area in three (the fourth it touches at a corner),
    # at x = 100 two
    assert_eq(chunk_files, 4 + 3 + 2, "one file per texture, cell and range")
    scenery.free()
    assert_eq(ResourceLazyLoader.resource_get_statistics()["registered"], registered, "chunks left registered")


func _parse_threaded() -> MaszynaImporterContext:
    var queue := WorkerTaskQueue.new()
    return SceneryInstancer.parse_file_task(ROOT, {}, MaszynaImporterContext.new().get_state(), queue)


## [models as [filename, position], terrain chunks as [texture, cell, range, vertex floats], sorted -
## the order the chunks appear in is the order the workers finished in]
func _describe(context:MaszynaImporterContext) -> Array:
    var models:Array = context.models.map(
        func(model:MaszynaModelData) -> Array: return [model.model_filename, model.position]
    )
    var chunks:Array = context.triangles_sink.get_geometries().map(
        func(geometry:MaszynaTrianglesChunkGeometry) -> Array:
            return [geometry.texture, geometry.cell, geometry.range_max, geometry.vertices.size()]
    )
    chunks.sort()
    return [models, chunks]


func _new_cache_files() -> PackedStringArray:
    var files:PackedStringArray = []
    for file:String in _list_cache_files():
        if not _existing_files.has(file):
            files.append(file)
    return files


func _list_cache_files() -> PackedStringArray:
    return DirAccess.get_files_at(_cache_dir)
