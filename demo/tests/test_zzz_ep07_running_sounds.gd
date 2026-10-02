extends MaszynaGutTest

## Running sounds of the real EP07-424 from td.scn (dynamic/pkp/303e_v1/303e-ep-tv.mmd): traction
## motors, wheel clatter, running noise, ventilator and curve squeal must be built from the MMD and
## the motors, clatter and outer noise must actually play once the trainset moves.

const EP07_MMD:String = "dynamic/pkp/303e_v1/303e-ep-tv.mmd"

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"
## The listener a vehicle's sound is built for once it is within earshot (TrainSoundSystem)
const PLAYER_SCENE:PackedScene = preload("res://addons/libmaszyna/player/player.tscn")

## How near its vehicle the node the vehicle's sound players are built under stands [m]
const MOUNT_TOLERANCE:float = 1.0
## Simulated seconds the rolling EP07 is listened to: about 110 m at 40 km/h, over 25 m rails
const LISTEN_SECONDS:float = 10.0

var _previous_game_dir:String = ""
var scenery:MaszynaSceneryNode


func before_each():
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each():
    if scenery:
        scenery.free()
        scenery = null
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_ep07_mmd_builds_positioned_running_sound_events() -> void:
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    add_child_autofree(vehicle)
    var diagnostics:Array[Dictionary] = []
    MmdSoundBankInstancer.build_into(
            vehicle, RID(), UserSettings.get_maszyna_game_dir().path_join(EP07_MMD), {}, {}, diagnostics)

    var running:SfxPlayer3D = vehicle.get_node_or_null("RunningSfxPlayer3D") as SfxPlayer3D
    assert_not_null(running, "EP07 should get its own running sound player")
    if not running:
        return
    for event_name:StringName in [
            &"traction_motor_0", &"traction_motor_1", &"ventilator", &"curve", &"outer_noise_0",
            &"outer_noise_1", &"wheel_clatter_0_0", &"wheel_clatter_1_0", &"wheel_clatter_2_0",
            &"wheel_clatter_3_5"]:
        assert_not_null(running.bank.get_event(event_name), "missing running event %s" % event_name)
    # tractionmotors: -4.26 4.26 - a negative MMD offset is ahead of the centre, -Z in the vehicle
    assert_almost_eq(running.bank.get_event(&"traction_motor_0").spatial_config.position.z, -4.26, 0.001)
    assert_almost_eq(running.bank.get_event(&"traction_motor_1").spatial_config.position.z, 4.26, 0.001)
    # wheel_clatter axles in the MMD order: -5.78 -2.75 2.74 5.78, one event per chunk
    assert_almost_eq(running.bank.get_event(&"wheel_clatter_0_0").spatial_config.position.z, -5.78, 0.001)
    assert_almost_eq(running.bank.get_event(&"wheel_clatter_3_0").spatial_config.position.z, 5.78, 0.001)
    # the clatter chunks are one-shots, the motor chunks loop
    var clatter_clip:SfxClip = running.bank.get_event(&"wheel_clatter_0_0").clips[0]
    var motor_clip:SfxClip = running.bank.get_event(&"traction_motor_0").automations[0].clips[0]
    assert_false((clatter_clip.stream as MaszynaAudioStream).loop)
    assert_true((motor_clip.stream as MaszynaAudioStream).loop)

    var cabin:SfxPlayer3D = vehicle.get_node("CabinSfxPlayer3D") as SfxPlayer3D
    assert_not_null(cabin.bank.get_event(&"running_noise"), "runningnoise: belongs to the cab bank")


## Starts a second EP07 already rolling (initial_velocity, the scenery's own trainset velocity) - the
## sounds only need the vehicle to move along the track, not the whole cab start-up sequence.
func test_ep07_plays_motor_clatter_and_outer_noise_when_rolling_on_td_scn() -> void:
    scenery = MaszynaSceneryNode.new()
    scenery.filename = SCENERY
    add_child(scenery)
    if not await wait_loaded(scenery.scenery_loaded, SCENERY):
        return
    var template:RID = VehicleServer.vehicle_get_rid_by_name("EP07-424")
    assert_true(template.is_valid(), "EP07-424 should be a vehicle of the loaded scenery")
    if not template.is_valid():
        return
    var template_dynamic:MaszynaDynamicData = MaszynaLegacyVehicleSystem.vehicle_get_dynamic(template)
    var vehicle := MaszynaRailVehicle3D.new()
    vehicle.data_path = template_dynamic.data_path
    vehicle.file_name = template_dynamic.file_name
    vehicle.skin = template_dynamic.skin
    vehicle.vehicle_id = "running_sounds_ep07"
    vehicle.initial_velocity = 40.0
    vehicle.start_track_name = "tdo_n25"
    vehicle.start_track_offset = 50.0
    scenery.add_child(vehicle)
    if not await wait_for_signal(vehicle.vehicle_built, BUILD_TIMEOUT):
        fail_test("the rolling EP07 was not built within %.0f s" % BUILD_TIMEOUT)
        return
    # resumed inside the vehicle's own emission: on from the next frame
    await wait_idle_frames(1)
    var rail_vehicle:RailVehicle3D = vehicle as RailVehicle3D
    assert_not_null(rail_vehicle.get_controller(), "the rolling EP07 has its controller once built")
    if not rail_vehicle.get_controller():
        return
    var controller:VehicleController = rail_vehicle.get_controller()
    # the sound is built once a listener is near the vehicle - on foot the player listens through
    # the free camera
    var player:MaszynaPlayer = PLAYER_SCENE.instantiate()
    add_child_autofree(player)
    player.free_camera.global_position = rail_vehicle.global_position
    # the vehicle's sound players are built under a node riding on it, of the system that built it -
    # the machine's work over a few frames once the listener is near; an array, as a lambda takes a
    # local by value
    var found_players:Array[SfxPlayer3D] = []
    var found:Callable = func() -> bool:
        for node:Node in MaszynaLegacyVehicleSystem.find_children("RunningSfxPlayer3D", "SfxPlayer3D", true, false):
            if (node.get_parent() as Node3D).global_position.distance_to(rail_vehicle.global_position) < MOUNT_TOLERANCE:
                found_players.append(node as SfxPlayer3D)
        return found_players.size() > 0
    if not await wait_until(found, BUILD_TIMEOUT):
        fail_test("the rolling EP07's sound was not built within %.0f s" % BUILD_TIMEOUT)
        return
    var running:SfxPlayer3D = found_players[0]

    var motor_heard:bool = false
    var outer_noise_heard:bool = false
    var clatter_count:int = 0
    var clatter_playing:Dictionary = {}
    for _tick:int in ticks(LISTEN_SECONDS):
        await step(1)
        motor_heard = motor_heard or running.is_playing(&"traction_motor_0")
        outer_noise_heard = outer_noise_heard or running.is_playing(&"outer_noise_0")
        for event:SfxEvent in running.bank.events:
            if not String(event.name).begins_with("wheel_clatter_"):
                continue
            var playing:bool = running.is_playing(event.name)
            if playing and not clatter_playing.get(event.name, false):
                clatter_count += 1
            clatter_playing[event.name] = playing
    print("EP07 speed %.1f km/h, motor=%s outer_noise=%s clatter hits=%d" % [
            float(controller.get_state().get("speed", 0.0)), motor_heard, outer_noise_heard, clatter_count])
    assert_gt(float(controller.get_state().get("speed", 0.0)), 5.0, "EP07 should be rolling")
    assert_true(motor_heard, "traction motors turn with the wheels, so they should play")
    assert_true(outer_noise_heard, "outer noise should play while rolling")
    # ~110 m at 40 km/h over 25 m rails - every axle clicks at each joint
    assert_gt(clatter_count, 4, "wheel clatter should play at the rail joints")
