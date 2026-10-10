extends MaszynaGutTest


class TestScenery extends MaszynaIncludeNode:
    var imports:int = 0

    func _load_content() -> void:
        imports += 1
        add_child(Node3D.new())
        _track_rids.append(TrackServer.track_create())


func test_initial_load_runs_once_and_editability_does_not_reload() -> void:
    var scenery:TestScenery = TestScenery.new()
    scenery.filename = "first.scn"
    watch_signals(scenery)
    add_child(scenery)
    await wait_idle_frames(3)
    assert_eq(scenery.imports, 1)
    var child:Node = scenery.get_child(0)
    scenery.editable_in_editor = true
    await get_tree().create_timer(0.6).timeout
    assert_eq(scenery.imports, 1)
    assert_same(scenery.get_child(0), child)
    assert_signal_emit_count(scenery, "loaded", 1)
    scenery.free()


func test_filename_change_needs_explicit_load_and_releases_previous_content() -> void:
    var baseline_tracks:int = TrackServer.track_get_rids().size()
    var scenery:TestScenery = TestScenery.new()
    scenery.filename = "first.scn"
    add_child(scenery)
    await wait_idle_frames(3)
    for cycle:int in range(3):
        var previous_child:Node = scenery.get_child(0)
        var previous_track:RID = scenery._track_rids[0]
        scenery.filename = "next_%s.scn" % cycle
        await wait_idle_frames(3)
        assert_eq(scenery.imports, cycle + 1, "filename change alone does not reload")
        scenery.load()
        assert_eq(scenery.imports, cycle + 2)
        assert_false(is_instance_valid(previous_child))
        assert_false(TrackServer.track_exists(previous_track))
        assert_eq(TrackServer.track_get_rids().size(), baseline_tracks + 1)
    var final_child:Node = scenery.get_child(0)
    scenery.filename = ""
    scenery.load()
    await wait_idle_frames(1)
    assert_eq(scenery.get_child_count(), 0)
    assert_false(is_instance_valid(final_child))
    assert_eq(TrackServer.track_get_rids().size(), baseline_tracks)
    scenery.free()


func test_explicit_load_consumes_pending_filename_change() -> void:
    var scenery:TestScenery = TestScenery.new()
    scenery.autoload = false
    scenery.filename = "first.scn"
    add_child(scenery)
    await wait_idle_frames(3)
    assert_eq(scenery.imports, 0)
    scenery.autoload = true
    await wait_idle_frames(3)
    assert_eq(scenery.imports, 0, "autoload alone does not request another import")
    scenery.filename = "second.scn"
    scenery.load()
    assert_eq(scenery.imports, 1)
    var track:RID = scenery._track_rids[0]
    scenery.free()
    assert_false(TrackServer.track_exists(track))


func test_initial_e3d_load_consumes_model_changes() -> void:
    var instance:E3DModelInstance = E3DModelInstance.new()
    instance.model = E3DModel.new()
    watch_signals(instance)
    add_child(instance)
    await wait_idle_frames(3)
    assert_false(instance._dirty)
    assert_signal_emit_count(instance, "e3d_loaded", 1)
    instance.free()
