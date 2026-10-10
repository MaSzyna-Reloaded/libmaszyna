extends MaszynaGutTest


func test_engine_gain_uses_rpm_and_load_without_synthetic_sound_state() -> void:
    var source:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    source.amplitude_offset = 0.5
    source.amplitude_factor = 1.5

    # level = 0.75 * 0.8 + 0.25 * 0.5 = 0.725
    assert_almost_eq(TrainSoundSystem._engine_gain(source, 0.8, 500.0, 1000.0), 1.5875, 0.001)



func test_engine_gain_clamps_to_event_modulation_domain() -> void:
    var source:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    source.amplitude_offset = 0.5
    source.amplitude_factor = 2.0

    assert_almost_eq(TrainSoundSystem._engine_gain(source, 2.0, 4000.0, 1000.0), 2.0, 0.001)
    assert_almost_eq(TrainSoundSystem._engine_gain(source, -1.0, -100.0, 1000.0), 0.5, 0.001)



func test_internal_sound_is_silent_without_an_occupied_listener_vehicle() -> void:
    var runtime:TrainSoundSystem.BankRuntime = TrainSoundSystem.BankRuntime.new()
    var source:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    source.placement = &"internal"

    assert_almost_eq(TrainSoundSystem._soundproofing(runtime, source), 0.0, 0.001)


const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const PLAYER_SCENE:PackedScene = preload("res://addons/libmaszyna/player/player.tscn")
## Where the camera stands to hear the vehicle at the origin, and where it cannot
const NEAR:Vector3 = Vector3(0.0, 0.0, 10.0)
const FAR:Vector3 = Vector3(0.0, 0.0, 10000.0)
const BOOKEND_LENGTH:float = 1.0
const MIX_RATE:int = 8000
## The sound system runs on the simulation's time. Simulated seconds a change takes to be heard or
## silenced: a sweep to see where the listener is (SWEEP_INTERVAL), a bank's
## update interval (never above FAR_UPDATE_INTERVAL) and a trigger tick (TRIGGER_INTERVAL)
const HEARD_SECONDS:float = (TrainSoundSystem.SWEEP_INTERVAL + TrainSoundSystem.FAR_UPDATE_INTERVAL
        + TrainSoundSystem.TRIGGER_INTERVAL)
## Simulated seconds within which a change is heard, or proved unheard
const SETTLE:float = HEARD_SECONDS + TICK
const EVENT_TIME_TOLERANCE:float = 0.25

var _vehicle_rid:RID
var _physics_node:VehiclePhysicsNode
var _vehicle:RailVehicle3D
var _camera:Camera3D
var _sound:SfxPlayer3D


## The vehicle leaves before its physics node is freed - it disconnects from its controller then
func after_each() -> void:
    if is_instance_valid(_vehicle):
        remove_child(_vehicle)
        _vehicle.free()
    _vehicle = null


## A vehicle at the origin, not given its controller yet, and the player whose camera is the listener
func _build_vehicle() -> void:
    _physics_node = build_vehicle_node("SoundCullingTest", SM42)
    var controller:VehicleController = VehicleServer.vehicle_get_controller(_physics_node.get_vehicle_rid())
    _vehicle_rid = controller.get_rid()
    _vehicle = RailVehicle3D.new()
    add_child(_vehicle)

    var player:MaszynaPlayer = PLAYER_SCENE.instantiate()
    add_child_autoqfree(player)
    # on foot the player looks through the free camera
    _camera = player.free_camera


func _attach_controller() -> void:
    _vehicle.controller_path = _vehicle.get_path_to(_physics_node)


## One bank on the vehicle: an "engine" event shaped like the one MmdSoundEventBuilder builds
## (opening bookend at 0, the loop after it) on the battery
func _register_bank(trigger_mode:int) -> void:
    var begin_clip:SfxClip = SfxClip.new()
    begin_clip.stream = _silent_stream()
    var main_clip:SfxClip = SfxClip.new()
    main_clip.stream = _silent_stream()
    main_clip.offset = BOOKEND_LENGTH
    var clips:Array[SfxClip] = [begin_clip, main_clip]
    var event:SfxEvent = SfxEvent.new()
    event.name = &"engine"
    event.clips = clips
    var events:Array[SfxEvent] = [event]
    _sound = SfxPlayer3D.new()
    _sound.bank = SfxBank.new()
    _sound.bank.events = events
    _vehicle.add_child(_sound)
    var soundproofing:Array[PackedFloat32Array] = []
    TrainSoundSystem.register_bank(_sound, {
        "vehicle": _vehicle_rid,
        "soundproofing": soundproofing,
        "triggers": [{
            "state_property": "battery_enabled",
            "trigger_mode": trigger_mode,
            "sound_event": &"engine",
        }],
    })


## The bank registered before the vehicle is a rail one and drawn (RailVehicle3D takes it then)
func _build(trigger_mode:int) -> void:
    _build_vehicle()
    _register_bank(trigger_mode)
    _attach_controller()


func _silent_stream() -> AudioStreamWAV:
    var stream:AudioStreamWAV = AudioStreamWAV.new()
    stream.format = AudioStreamWAV.FORMAT_8_BITS
    stream.mix_rate = MIX_RATE
    var data:PackedByteArray = PackedByteArray()
    data.resize(int(BOOKEND_LENGTH * MIX_RATE))
    stream.data = data
    return stream


func _playing() -> bool:
    return _sound.is_playing(&"engine")


func test_running_sound_plays_again_when_the_camera_comes_back() -> void:
    _build(TrainSoundSystem.TRIGGER_MODE_TOGGLE)
    _camera.global_position = NEAR
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    if not await wait_simulated_until(_playing, SETTLE, "the running sound"):
        return
    assert_true(_playing(), "heard while the camera is near")

    _camera.global_position = FAR
    if not await wait_simulated_until(func() -> bool: return not _playing(), SETTLE, "the running sound silenced"):
        return
    assert_false(_playing(), "silent while the camera is away")

    _camera.global_position = NEAR
    if not await wait_simulated_until(_playing, SETTLE, "the running sound heard again"):
        return
    assert_true(_playing(), "heard again once the camera is back")


func test_sound_switched_on_while_heard_plays_its_opening_bookend() -> void:
    _build(TrainSoundSystem.TRIGGER_MODE_TOGGLE)
    # the sweep sees the listener away, then near - nothing to observe but its time
    _camera.global_position = FAR
    await step(ticks(SETTLE))
    _camera.global_position = NEAR
    await step(ticks(SETTLE))
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    if not await wait_simulated_until(_playing, SETTLE, "the sound switched on"):
        return

    assert_almost_eq(
            float(_sound.get_event_visualization_state(&"engine")["event_time"]), 0.0,
            EVENT_TIME_TOLERANCE, "starts at its opening bookend")


func test_one_shot_due_while_the_camera_is_away_is_dropped() -> void:
    _build(TrainSoundSystem.TRIGGER_MODE_CHANGE)
    # the sweep sees the listener near, then away; the change is due unheard, and the listener comes
    # back - nothing to observe but the time each takes
    _camera.global_position = NEAR
    await step(ticks(SETTLE))
    _camera.global_position = FAR
    await step(ticks(SETTLE))
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    await step(ticks(SETTLE))
    _camera.global_position = NEAR
    await step(ticks(SETTLE))

    assert_false(_playing(), "the change happened unheard")


func test_bank_registered_after_its_vehicle_has_a_controller_is_heard() -> void:
    _build_vehicle()
    _attach_controller()
    _register_bank(TrainSoundSystem.TRIGGER_MODE_TOGGLE)
    _camera.global_position = NEAR
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    if not await wait_simulated_until(_playing, SETTLE, "the late bank heard"):
        return

    assert_true(_playing())


func test_bank_is_built_once_its_vehicle_is_within_earshot() -> void:
    _build_vehicle()
    var built:Array[int] = [0]
    _camera.global_position = FAR
    TrainSoundSystem.vehicle_set_bank_builder(_vehicle_rid, func() -> void: built[0] += 1)
    # proved unbuilt by the time a sweep takes
    await step(ticks(SETTLE))
    assert_eq(built[0], 0, "not built while the camera is away")

    _camera.global_position = NEAR
    if not await wait_simulated_until(func() -> bool: return built[0] > 0, SETTLE, "the bank built after the camera came near"):
        return
    # proved built only once by the time the next sweep takes
    await step(ticks(SETTLE))
    assert_eq(built[0], 1, "built once, when the camera came near")


func test_bank_builder_taken_back_is_never_called() -> void:
    _build_vehicle()
    var built:Array[int] = [0]
    _camera.global_position = FAR
    TrainSoundSystem.vehicle_set_bank_builder(_vehicle_rid, func() -> void: built[0] += 1)
    TrainSoundSystem.vehicle_set_bank_builder(_vehicle_rid, Callable())
    _camera.global_position = NEAR
    # proved never called by the time a sweep takes
    await step(ticks(SETTLE))

    assert_eq(built[0], 0)
