extends MaszynaGutTest


var _source_path:String
var _nested_path:String
var _cache_path:String


func before_each() -> void:
    var suffix:String = str(Time.get_ticks_usec())
    _source_path = OS.get_temp_dir().path_join("libmaszyna_scenery_%s.scn" % suffix)
    _nested_path = OS.get_temp_dir().path_join("libmaszyna_scenery_%s.inc" % suffix)
    _write_file(_source_path, "root")
    _write_file(_nested_path, "include")
    _cache_path = SceneryInstancer._get_cache_path(
        _source_path,
        SceneryInstancer._get_parameters_hash({}),
    )


func after_each() -> void:
    SceneryInstancer._cache.remove(_cache_path)
    DirAccess.remove_absolute(_source_path)
    DirAccess.remove_absolute(_nested_path)


func test_binary_cache_restores_typed_track_resources_and_nodes() -> void:
    var context:MaszynaImporterContext = _make_context()
    var cached_vehicle := Node3D.new()
    cached_vehicle.name = "CachedVehicle"
    var objects:Array = [cached_vehicle]
    var parameters_hash:String = SceneryInstancer._get_parameters_hash({})
    var compiled:MaszynaCompiledScenery = SceneryInstancer._compile_scenery(
        _source_path,
        parameters_hash,
        context,
        objects,
    )
    SceneryInstancer._cache.set(_cache_path, compiled)

    var cache_file:String = "user://cache".path_join(
        SceneryInstancer._cache.get_cache_dir()
    ).path_join(_cache_path)
    var disk_resource:Resource = ResourceLoader.load(
        cache_file,
        "",
        ResourceLoader.CACHE_MODE_IGNORE,
    )
    var loaded:MaszynaCompiledScenery = disk_resource as MaszynaCompiledScenery
    assert_not_null(loaded)
    assert_eq(loaded.tracks.size(), 1)
    assert_true(loaded.tracks[0] is MaszynaTrackData)
    assert_eq((loaded.tracks[0] as MaszynaTrackData).curve.p2, Vector3(10.0, 0.0, 0.0))

    var cached_objects:Array = SceneryInstancer._instantiate_cached_nodes(loaded.nodes)
    assert_eq(cached_objects.size(), 1)
    assert_eq((cached_objects[0] as Node).name, "CachedVehicle")
    for object:Node in cached_objects:
        object.free()
    for object:Node in objects:
        object.free()


func test_nested_dependency_change_invalidates_cache() -> void:
    var context:MaszynaImporterContext = _make_context()
    var node := Node3D.new()
    var objects:Array = [node]
    var parameters_hash:String = SceneryInstancer._get_parameters_hash({})
    var compiled:MaszynaCompiledScenery = SceneryInstancer._compile_scenery(
        _source_path,
        parameters_hash,
        context,
        objects,
    )
    assert_true(SceneryInstancer._is_cache_valid(compiled, _source_path, parameters_hash))

    _write_file(_nested_path, "include changed")
    assert_false(SceneryInstancer._is_cache_valid(compiled, _source_path, parameters_hash))
    node.free()


func test_parameter_hash_is_deterministic_and_part_of_cache_key() -> void:
    var first_hash:String = SceneryInstancer._get_parameters_hash({"p2": "b", "p1": "a"})
    var reordered_hash:String = SceneryInstancer._get_parameters_hash({"p1": "a", "p2": "b"})
    var changed_hash:String = SceneryInstancer._get_parameters_hash({"p1": "x", "p2": "b"})
    assert_eq(first_hash, reordered_hash)
    assert_false(first_hash == changed_hash)
    assert_false(
        SceneryInstancer._get_cache_path(_source_path, first_hash) ==
        SceneryInstancer._get_cache_path(_source_path, changed_hash)
    )


func test_recursive_file_entry_disables_cache() -> void:
    var context := MaszynaImporterContext.new()
    assert_true(context.begin_file(_source_path))
    assert_false(context.begin_file(_source_path))
    assert_false(context.cacheable)
    context.end_file(_source_path)
    assert_true(context.begin_file(_source_path))


func _make_context() -> MaszynaImporterContext:
    var context := MaszynaImporterContext.new()
    context.register_dependency(_source_path)
    context.register_dependency(_nested_path)
    var track := MaszynaTrackData.new()
    track.track_name = "test"
    track.curve = TrackCurve.new()
    track.curve.p2 = Vector3(10.0, 0.0, 0.0)
    context.tracks.append(track)
    return context


func _write_file(path:String, content:String) -> void:
    var file:FileAccess = FileAccess.open(path, FileAccess.WRITE)
    file.store_string(content)
