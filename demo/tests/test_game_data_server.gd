extends MaszynaGutTest

## The cache directory is the test's own, so nothing the game caches is touched
const CACHE_DIRECTORY:String = "test_game_data_server"

var _cache:ResourceCache
## The requests of a reload, in the order they came
var _requests:Array[StringName] = []


func before_each():
    _cache = ResourceCache.create(CACHE_DIRECTORY)
    _requests.clear()

func after_each():
    _cache.clear()
    _cache = null

func test_clear_cache_emits_the_request():
    watch_signals(GameDataServer)
    GameDataServer.cache_clear()
    assert_signal_emitted(GameDataServer, "cache_clear_requested")

func test_a_created_cache_follows_the_request():
    var resource:Resource = Resource.new()
    _cache.set("entry.res", resource)
    assert_true(_cache.has("entry.res"), "the entry should be in the cache before it is cleared")

    GameDataServer.cache_clear()
    assert_false(_cache.has("entry.res"), "cache_clear() should drop the entry of every cache")

## An entry saved again is what the cache hands out, while the one read before is still held
func test_an_entry_saved_again_replaces_the_one_read_before():
    var first:Resource = Resource.new()
    first.resource_name = "first"
    _cache.set("entry.res", first)
    var read:Resource = _cache.get("entry.res")
    assert_eq(read.resource_name, "first")

    var second:Resource = Resource.new()
    second.resource_name = "second"
    _cache.set("entry.res", second)
    assert_eq(_cache.get("entry.res").resource_name, "second")

func test_a_reload_unloads_first_then_reloads():
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)
    GameDataServer.data_reload()
    GameDataServer.data_unload_requested.disconnect(_on_data_unload_requested)
    GameDataServer.data_reload_requested.disconnect(_on_data_reload_requested)
    var expected:Array[StringName] = [&"unload", &"reload"]
    assert_eq(_requests, expected)

func test_a_reload_leaves_the_caches_on_disk():
    _cache.set("entry.res", Resource.new())
    GameDataServer.data_reload()
    assert_true(_cache.has("entry.res"), "a reload should not clear the caches - that is cache_clear()")

func test_build_number_is_the_stamp_of_the_build():
    var stamp:String = GameDataServer.build_get_number()
    assert_eq(stamp, FileAccess.get_file_as_string("res://build_number.txt").strip_edges())


func _on_data_unload_requested() -> void:
    _requests.append(&"unload")

func _on_data_reload_requested() -> void:
    _requests.append(&"reload")
