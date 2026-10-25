extends MaszynaGutTest

## demo_3d.tscn has no scenery to wait for - its vehicles stand in the scene - so its Player starts
## itself (auto_start) and has its camera at once, streamed around (libmaszyna#325: nothing started
## the player, which had no camera). The scene is read, not instanced: its vehicles' data is the
## game directory's.

const DEMO_3D_SCENE:String = "res://demo_3d.tscn"
## The synthetic vehicle with a cab (demo/tests/fixtures), in place of the game directory's
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const START_VEHICLE_ID:String = "test_start_vehicle"
const PLAYER_SCENE:PackedScene = preload("res://addons/libreloaded/player/player.tscn")
const STREAMER_SCRIPT:Script = preload("res://addons/libreloaded/scenery/scenery_streamer.gd")

var _player:Player
var _streamer:SceneryStreamer
var _scene:Node3D
var _previous_game_dir:String


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    PlayerServer.player_stop()
    if is_instance_valid(_player):
        _player.free()
    if is_instance_valid(_streamer):
        _streamer.free()
    # the cab interior is freed at the end of the frame
    await wait_idle_frames(1)
    if is_instance_valid(_scene):
        _scene.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_the_player_of_demo_3d_starts_itself() -> void:
    var state:SceneState = (load(DEMO_3D_SCENE) as PackedScene).get_state()
    var auto_start:Variant = null
    for node:int in state.get_node_count():
        if not state.get_node_name(node) == &"Player":
            continue
        for property:int in state.get_node_property_count(node):
            if state.get_node_property_name(node, property) == &"auto_start":
                auto_start = state.get_node_property_value(node, property)
    assert_eq(auto_start, true, "demo_3d's Player starts as it becomes ready")


## As demo_3d has it: the streamer takes the player's camera (Player.camera_changed) and streams
## once the player has started
func test_a_player_starting_itself_has_its_camera_streamed_around() -> void:
    _streamer = STREAMER_SCRIPT.new()
    add_child(_streamer)
    _player = PLAYER_SCENE.instantiate()
    _player.auto_start = true
    _player.camera_changed.connect(_streamer.set_camera)

    add_child(_player)

    assert_true(PlayerServer.player_is_started())
    assert_not_null(_player.camera, "started, the player has its camera")
    assert_eq(get_viewport().get_camera_3d(), _player.camera, "and it is current")
    assert_eq(_player.camera.get_parent(), _player.free_view, "on foot, from the free view")
    assert_true(SceneryStreamingServer.streaming_has_camera(), "the world is streamed around it")


## As a scene whose vehicles stand in it (demo_3d.tscn with a start_vehicle_id): the vehicle and the
## Player enter the tree in one pass, and the player starts in the vehicle before it is built - and
## sits in its cab once it is
func test_a_player_starting_itself_starts_in_its_start_vehicle() -> void:
    _scene = Node3D.new()
    var vehicle:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    vehicle.data_path = "dynamic/test/synthetic_v1"
    vehicle.file_name = "synthetic"
    vehicle.vehicle_id = START_VEHICLE_ID
    _scene.add_child(vehicle)
    var player:Player = PLAYER_SCENE.instantiate()
    player.start_vehicle_id = START_VEHICLE_ID
    player.auto_start = true
    _scene.add_child(player)

    add_child(_scene)

    var start:RID = PlayerServer.player_get_start_vehicle()
    assert_true(start.is_valid(), "started in its start_vehicle_id, before the vehicle is built")
    if not await wait_for_signal(vehicle.vehicle_built, BUILD_TIMEOUT):
        fail_test("the start vehicle was not built within %.0f s" % BUILD_TIMEOUT)
        return
    await wait_idle_frames(1)
    assert_eq(start, vehicle.get_rid(), "the vehicle it started in is the scene's")
    assert_eq(PlayerServer.player_get_vehicle(), start, "built, the vehicle is taken over")
    assert_not_null(player.camera, "and the player has its camera")
    assert_eq(player.camera.get_parent(), player.cabin_view, "in its cab")
