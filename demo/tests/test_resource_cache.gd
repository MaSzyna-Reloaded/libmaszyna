extends MaszynaGutTest

## What a cache entry is worth: written once, read back from the disk by the next session - not
## from Godot's resource cache, which holds what was saved while it is referenced.
## The tests' hook turns the cache off; this script turns it on for each test and back after. Its
## own entries are under a cache directory of their own or removed after each test - the run
## shares the user directory of the project, the player's caches with it, and never clears them
## (no GameDataServer.cache_clear() here).

const CACHE_DIR:String = "test_resource_cache"
const ENTRY:String = "entry.res"
const HASH:String = "first"
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const E3D_DATA_PATH:String = "models/sem/karzelki"
const E3D_MODEL:String = "ktmnb"
const FIZ_PATH:String = "res://tests/fixtures/test_vehicle.fiz"

var _cache:ResourceCache
var _previous_game_dir:String = ""


func before_each() -> void:
    ProjectSettings.set_setting(CACHE_ENABLED_SETTING, true)
    _cache = ResourceCache.create(CACHE_DIR)
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    _cache.clear()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)
    ProjectSettings.set_setting(CACHE_ENABLED_SETTING, false)


## A resource only this test makes, so nothing but the entry on the disk can hand it back
func _gradient() -> Gradient:
    var gradient:Gradient = Gradient.new()
    gradient.offsets = PackedFloat32Array([0.0, 0.25, 1.0])
    gradient.colors = PackedColorArray([Color.RED, Color.GREEN, Color.BLUE])
    return gradient


func test_an_entry_is_read_back_from_the_disk_by_another_cache() -> void:
    _cache.set(ENTRY, _gradient(), HASH)
    # the saved resource is gone: what comes back is what the disk holds
    var other:ResourceCache = ResourceCache.create(CACHE_DIR)
    assert_true(FileAccess.file_exists(other.get_file_path(ENTRY)), "the entry is a file")
    assert_true(other.has(ENTRY, HASH))
    var read:Gradient = other.get(ENTRY, HASH) as Gradient
    assert_not_null(read, "the entry is read back")
    if read:
        assert_eq(read.offsets, PackedFloat32Array([0.0, 0.25, 1.0]))
        assert_eq(read.colors[2], Color.BLUE)


func test_an_entry_of_another_hash_is_not_read() -> void:
    _cache.set(ENTRY, _gradient(), HASH)
    assert_false(_cache.has(ENTRY, "second"))
    assert_null(_cache.get(ENTRY, "second"))
    # asked without a hash, the entry is there
    assert_true(_cache.has(ENTRY))


func test_an_entry_saved_again_without_a_hash_drops_the_old_one() -> void:
    _cache.set(ENTRY, _gradient(), HASH)
    _cache.set(ENTRY, _gradient())
    assert_false(_cache.has(ENTRY, HASH), "the old hash does not vouch for the new entry")
    assert_not_null(_cache.get(ENTRY))


func test_an_entry_saved_again_is_read_as_saved_last() -> void:
    _cache.set(ENTRY, _gradient(), HASH)
    var changed:Gradient = _gradient()
    changed.offsets = PackedFloat32Array([0.0, 0.5, 1.0])
    _cache.set(ENTRY, changed, "second")
    changed = null
    var read:Gradient = ResourceCache.create(CACHE_DIR).get(ENTRY, "second") as Gradient
    assert_not_null(read)
    if read:
        assert_eq(read.offsets[1], 0.5)


func test_remove_and_clear_take_the_files_away() -> void:
    _cache.set(ENTRY, _gradient(), HASH)
    _cache.set("other.res", _gradient(), HASH)
    _cache.remove(ENTRY)
    assert_false(FileAccess.file_exists(_cache.get_file_path(ENTRY)))
    assert_false(FileAccess.file_exists(_cache.get_file_path(ENTRY) + ".hash"))
    assert_true(_cache.has("other.res", HASH))
    _cache.clear()
    assert_false(DirAccess.dir_exists_absolute("user://cache".path_join(CACHE_DIR)))


func test_switched_off_it_reads_and_writes_nothing() -> void:
    _cache.set(ENTRY, _gradient(), HASH)
    ProjectSettings.set_setting(CACHE_ENABLED_SETTING, false)
    assert_false(_cache.has(ENTRY, HASH))
    assert_null(_cache.get(ENTRY, HASH))
    _cache.set("other.res", _gradient(), HASH)
    assert_false(FileAccess.file_exists(_cache.get_file_path("other.res")))


## E3DModelManager - the scenery's and the vehicles' models, the largest cache of a game directory
func test_a_model_is_read_back_from_its_cache_entry() -> void:
    var cache:ResourceCache = E3DModelManager._cache
    # the path the manager reads and hashes: the game directory as the settings hand it out
    var source:String = UserSettings.get_maszyna_game_dir().path_join(E3D_DATA_PATH).path_join(E3D_MODEL + ".e3d")
    var entry:String = E3DModelManager._make_cache_path(E3D_DATA_PATH, E3D_MODEL, source)
    cache.remove(entry)
    var parsed:E3DModel = E3DModelManager.load_model(E3D_DATA_PATH, E3D_MODEL)
    assert_not_null(parsed)
    assert_true(cache.has(entry, E3DModelManager._make_cache_hash(source)), "the model was cached")
    var submodels:int = parsed.submodels.size() if parsed else -1
    parsed = null

    var read:E3DModel = cache.get(entry, E3DModelManager._make_cache_hash(source)) as E3DModel
    assert_not_null(read, "the entry is read back")
    if read:
        assert_eq(read.submodels.size(), submodels)
        assert_eq(read.resource_path, cache.get_file_path(entry))
    cache.remove(entry)


## FizVehicleBuilder - a vehicle's parsed .fiz, its controller and components
func test_a_vehicle_description_is_read_back_from_its_cache_entry() -> void:
    var cache:ResourceCache = FizVehicleBuilder._cache
    var entry:String = FizVehicleBuilder._make_cache_path(FIZ_PATH)
    cache.remove(entry)
    var parsed:VehicleController = FizVehicleBuilder.build_description_at(FIZ_PATH)
    assert_not_null(parsed)
    assert_true(cache.has(entry, FizVehicleBuilder._make_cache_hash(FIZ_PATH)), "the description was cached")
    var components:int = parsed.get_components().size() if parsed else -1
    parsed = null

    var read:VehicleController = cache.get(entry, FizVehicleBuilder._make_cache_hash(FIZ_PATH)) as VehicleController
    assert_not_null(read, "the entry is read back")
    if read:
        assert_eq(read.get_components().size(), components)
    cache.remove(entry)
