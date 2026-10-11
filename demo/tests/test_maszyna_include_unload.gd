extends MaszynaGutTest

## A scenery that unloads - reloaded in place (another file set on the same node) or freed -
## stops what runs on its content before freeing it: the streaming's preloads in flight are
## joined, and its scenario is told to stop (MaszynaIncludeNode.unloading).

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY_FILE:String = "no_time.scn"
## evening_time.scn: time 18:45
const OTHER_SCENERY_FILE:String = "evening_time.scn"
const OTHER_SCENERY_START_TIME:float = 18.75
## A scenery with vehicles and trainsets (demo/tests/fixtures/scenery)
const TRAINSETS_SCENERY_FILE:String = "trainsets.scn"
## How long a preload in flight takes [ms] - longer than a reload of an empty scenery
const PRELOAD_MSEC:int = 500
## Frames given to the streaming to start the preload
const STREAMING_FRAMES:int = 60

var _previous_game_dir:String = ""
var _camera:Camera3D
var _stream_rids:Array[RID] = []
var _preload_mutex:Mutex = Mutex.new()
var _preloads_started:int = 0
var _preloads_finished:int = 0
var _unloads:int = 0


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    _camera = Camera3D.new()
    add_child_autoqfree(_camera)
    _unloads = 0


func after_each() -> void:
    for stream_rid:RID in _stream_rids:
        SceneryStreamingServer.stream_free(stream_rid)
    _stream_rids.clear()
    SceneryStreamingServer.streaming_set_camera(0)
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


## A reload frees the content its preloads in flight are loading: they are joined first, as on
## leaving the tree
func test_a_reload_in_place_joins_the_preloads_in_flight() -> void:
    var owner:int = SceneryStreamingServer.owner_create("unload_test", _slow_preload, _ignore_build, _ignore_clear)
    _stream_rids.append(SceneryStreamingServer.stream_register(owner, rid_from_int64(20000), Vector3.ZERO, 0.0))
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())
    for frame:int in STREAMING_FRAMES:
        if _get_preloads_started() > 0:
            break
        await wait_idle_frames(1)
    assert_gt(_get_preloads_started(), 0, "the streaming started no preload")

    var include:MaszynaIncludeNode = MaszynaIncludeNode.new()
    include.autoload = false
    add_child_autofree(include)
    await include.load()

    _preload_mutex.lock()
    var in_flight:int = _preloads_started - _preloads_finished
    _preload_mutex.unlock()
    assert_eq(in_flight, 0, "a preload still ran after the content was cleared")


## Out of the tree the content stays (the editor takes a scene out of the tree when another tab is
## shown); it goes when the node is freed
func test_unloading_is_announced_on_a_reload_and_when_the_node_is_freed() -> void:
    var include:MaszynaIncludeNode = MaszynaIncludeNode.new()
    include.autoload = false
    add_child(include)
    include.unloading.connect(_count_unload)

    await include.load()
    assert_eq(_unloads, 1, "a reload did not announce the unload")

    remove_child(include)
    assert_eq(_unloads, 1, "leaving the tree is not an unload")
    include.free()
    assert_eq(_unloads, 2, "the node freed did not announce the unload")


## "Load" pressed again while a scenery loads - the editor, after the file was changed - gives the
## load under way up, and what the scenery is set to now is loaded instead
func test_a_load_asked_for_during_a_load_gives_that_one_up() -> void:
    var scenery:MaszynaSceneryNode = MaszynaSceneryNode.new()
    scenery.autoload = false
    scenery.filename = SCENERY_FILE
    add_child_autofree(scenery)
    watch_signals(scenery)

    scenery.load()
    scenery.filename = OTHER_SCENERY_FILE
    scenery.load()
    if not await wait_loaded(scenery.scenery_loaded, scenery.filename):
        return

    assert_signal_emit_count(scenery, "scenery_loaded", 1, "the load given up announced a scenery")
    assert_eq(scenery.start_time, OTHER_SCENERY_START_TIME, "the scenery loaded is not the one asked for last")


## "Stop": the load under way ends without a scenery, and what it had built is freed
func test_a_stopped_load_leaves_nothing_loaded() -> void:
    var scenery:MaszynaSceneryNode = MaszynaSceneryNode.new()
    scenery.autoload = false
    scenery.filename = SCENERY_FILE
    add_child_autofree(scenery)
    watch_signals(scenery)

    scenery.load()
    scenery.stop_loading()
    if not await wait_loaded(scenery.load_ended, scenery.filename):
        return

    assert_signal_not_emitted(scenery, "scenery_loaded")
    assert_signal_not_emitted(scenery, "loaded")
    assert_eq(scenery.get_vehicles().size() + scenery.get_trainsets().size(), 0)


## "Clear": what is loaded is freed, and the scenery stays set to its file
func test_clearing_frees_what_is_loaded_and_keeps_the_file() -> void:
    var scenery:MaszynaSceneryNode = MaszynaSceneryNode.new()
    scenery.autoload = false
    scenery.filename = TRAINSETS_SCENERY_FILE
    add_child_autofree(scenery)
    await scenery.load()
    assert_gt(scenery.get_vehicles().size(), 0, "the scenery has vehicles to free")
    watch_signals(scenery)

    await scenery.clear()

    assert_signal_emitted(scenery, "unloading", "what runs on the content is told to stop")
    assert_eq(scenery.get_vehicles().size() + scenery.get_trainsets().size(), 0)
    assert_eq(scenery.filename, TRAINSETS_SCENERY_FILE)


## The scenario runs only once started, and stops when its scenery unloads - as the game's world
## wires it (world.gd)
func test_a_scenario_runs_from_its_start_until_its_scenery_unloads() -> void:
    var scenery:MaszynaSceneryNode = MaszynaSceneryNode.new()
    scenery.filename = SCENERY_FILE
    add_child_autofree(scenery)
    if not await wait_loaded(scenery.scenery_loaded, scenery.filename):
        return

    var scenario:MaszynaLegacyScenario = MaszynaLegacyScenario.new()
    assert_false(scenario.get_script_context().is_valid(), "a scenario ran before it was started")
    await scenario.start(scenery)
    assert_true(scenario.get_script_context().is_valid(), "a started scenario has no script context")

    scenery.unloading.connect(scenario.stop)
    scenery.filename = ""
    await scenery.load()
    assert_false(scenario.get_script_context().is_valid(), "the scenario ran on after its scenery unloaded")


func _count_unload() -> void:
    _unloads += 1


func _get_preloads_started() -> int:
    _preload_mutex.lock()
    var started:int = _preloads_started
    _preload_mutex.unlock()
    return started


## Streaming worker thread
func _slow_preload(_user_rid:RID) -> Variant:
    _preload_mutex.lock()
    _preloads_started += 1
    _preload_mutex.unlock()
    OS.delay_msec(PRELOAD_MSEC)
    _preload_mutex.lock()
    _preloads_finished += 1
    _preload_mutex.unlock()
    return null


func _ignore_build(_user_rid:RID, _preloaded:Variant) -> void:
    pass


func _ignore_clear(_user_rid:RID) -> void:
    pass
