extends MaszynaGutTest

## ResourceLazyLoader: a resource is shared by its key. With lazy loading it is loaded when it is
## wanted, held while it is held and let go once nobody holds it; without it, it is loaded when it is
## registered and kept until its last registration is freed.

var _loads:int = 0
var _rids:Array[RID] = []
var _previous_lazy_loading:bool = false


func before_each() -> void:
    _loads = 0
    _previous_lazy_loading = ResourceLazyLoader.lazy_loading
    ResourceLazyLoader.lazy_loading = true


func after_each() -> void:
    for rid:RID in _rids:
        ResourceLazyLoader.resource_free(rid)
    _rids.clear()
    ResourceLazyLoader.lazy_loading = _previous_lazy_loading


func test_one_key_is_one_resource() -> void:
    var first:RID = _register("test/shared")
    var second:RID = _register("test/shared")
    assert_eq(first, second, "one key gave two resources")
    var held:Resource = _hold(first)
    assert_same(ResourceLazyLoader.resource_load(second), held, "the held resource was not handed out")
    assert_eq(_loads, 1, "a held resource was loaded again")
    ResourceLazyLoader.resource_release(first)


func test_resource_is_held_until_the_last_release() -> void:
    var rid:RID = _register("test/held")
    assert_false(ResourceLazyLoader.resource_is_resident(rid), "resident before anybody held it")
    _hold(rid)
    _hold(rid)
    ResourceLazyLoader.resource_release(rid)
    assert_true(ResourceLazyLoader.resource_is_resident(rid), "let go while still held")
    ResourceLazyLoader.resource_release(rid)
    assert_false(ResourceLazyLoader.resource_is_resident(rid), "held after the last release")


func test_copies_loaded_before_the_hold_give_one_held_resource() -> void:
    var rid:RID = _register("test/copies")
    var first:Resource = ResourceLazyLoader.resource_load(rid)
    var second:Resource = ResourceLazyLoader.resource_load(rid)
    assert_false(ResourceLazyLoader.resource_is_resident(rid), "a load alone holds the resource")
    assert_same(ResourceLazyLoader.resource_hold(rid, first), first, "the first copy was not held")
    assert_same(ResourceLazyLoader.resource_hold(rid, second), first, "a second copy was held")
    ResourceLazyLoader.resource_release(rid)
    ResourceLazyLoader.resource_release(rid)


func test_released_resource_is_loaded_again() -> void:
    var rid:RID = _register("test/reloaded")
    _hold(rid)
    ResourceLazyLoader.resource_release(rid)
    _hold(rid)
    assert_eq(_loads, 2, "a released resource was kept")
    ResourceLazyLoader.resource_release(rid)


func test_data_reload_lets_go_of_held_resources() -> void:
    var rid:RID = _register("test/unloaded")
    var before:Resource = _hold(rid)
    GameDataServer.data_reload()
    assert_false(ResourceLazyLoader.resource_is_resident(rid), "held over the data reload")
    assert_not_same(ResourceLazyLoader.resource_load(rid), before, "the old data was handed out again")
    ResourceLazyLoader.resource_release(rid)


func test_without_lazy_loading_a_resource_is_loaded_when_registered() -> void:
    ResourceLazyLoader.lazy_loading = false
    var rid:RID = _register("test/eager")
    _register("test/eager")
    assert_true(ResourceLazyLoader.resource_is_resident(rid), "not loaded by its registration")
    assert_eq(_loads, 1, "loaded again by a second registration")


func test_without_lazy_loading_a_released_resource_is_kept() -> void:
    ResourceLazyLoader.lazy_loading = false
    var rid:RID = _register("test/eager_kept")
    var loaded:Resource = ResourceLazyLoader.resource_load(rid)
    assert_same(_hold(rid), loaded, "the resource loaded at registration was not the one held")
    ResourceLazyLoader.resource_release(rid)
    assert_true(ResourceLazyLoader.resource_is_resident(rid), "let go after the last release")
    assert_eq(_loads, 1, "loaded again after the release")


func test_without_lazy_loading_the_last_registration_lets_go() -> void:
    ResourceLazyLoader.lazy_loading = false
    var rid:RID = ResourceLazyLoader.resource_register("test/eager_freed", _load)
    ResourceLazyLoader.resource_free(rid)
    var again:RID = _register("test/eager_freed")
    assert_eq(_loads, 2, "kept after its last registration was freed")
    assert_true(ResourceLazyLoader.resource_is_resident(again), "not loaded by its new registration")


func _register(key:String) -> RID:
    var rid:RID = ResourceLazyLoader.resource_register(key, _load)
    _rids.append(rid)
    return rid


func _hold(rid:RID) -> Resource:
    return ResourceLazyLoader.resource_hold(rid, ResourceLazyLoader.resource_load(rid))


func _load() -> Resource:
    _loads += 1
    return Resource.new()
