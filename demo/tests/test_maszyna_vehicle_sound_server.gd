extends MaszynaGutTest


const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const PLAYER_SCENE:PackedScene = preload("res://addons/libreloaded/player/player.tscn")
## Where the camera stands to hear the vehicle at the origin, and where it cannot
const NEAR:Vector3 = Vector3(0.0, 0.0, 10.0)
const FAR:Vector3 = Vector3(0.0, 0.0, 10000.0)
const BOOKEND_LENGTH:float = 1.0
const MIX_RATE:int = 8000
## MaszynaVehicleSoundServer runs on the simulation's time. Simulated seconds a change takes to be
## heard or silenced, at most: a sweep to see where the listener is (0.25 s) and a vehicle's feed
## interval (never above 0.5 s)
const HEARD_SECONDS:float = 0.25 + 0.5
## The quantity the bank's trigger follows
const STATE_PARAMETER:StringName = &"battery_enabled"
## Simulated seconds within which a change is heard, or proved unheard
const SETTLE:float = HEARD_SECONDS + TICK
const EVENT_TIME_TOLERANCE:float = 0.25

var _vehicle_rid:RID
var _physics_node:VehiclePhysicsNode
var _vehicle:RailVehicle3D
## The view the player's camera looks through - moved, it takes the listener with it
var _camera:Node3D
var _sound:SfxPlayer3D


## The vehicle leaves before its physics node is freed - it disconnects from its controller then
func after_each() -> void:
    PlayerServer.player_stop()
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

    var player:Player = PLAYER_SCENE.instantiate()
    add_child_autoqfree(player)
    # started outside, as after a scenery with no start vehicle: only now has the player its camera
    PlayerServer.player_start(RID())
    # on foot the player looks through the free view
    _camera = player.free_view


func _attach_controller() -> void:
    _vehicle.controller_path = _vehicle.get_path_to(_physics_node)


## One bank on the vehicle: an "engine" event shaped like the one MmdSoundEventBuilder builds
## (opening bookend at 0, the loop after it), started by the battery's state as MmdSoundFormulas
## starts it - a gate playing while it is on (TOGGLE) or a one-shot at each change (CHANGE)
func _register_bank(trigger_mode:MmdSoundCatalog.TriggerMode) -> void:
    var begin_clip:SfxClip = SfxClip.new()
    begin_clip.stream = _silent_stream()
    var main_clip:SfxClip = SfxClip.new()
    main_clip.stream = _silent_stream()
    main_clip.offset = BOOKEND_LENGTH
    var clips:Array[SfxClip] = [begin_clip, main_clip]
    var trigger:SfxParameterTrigger = SfxParameterTrigger.new()
    trigger.parameter = STATE_PARAMETER
    if trigger_mode == MmdSoundCatalog.TriggerMode.CHANGE:
        trigger.mode = SfxParameterTrigger.STEP
    else:
        trigger.mode = SfxParameterTrigger.GATE
        trigger.on_threshold = 0.5
        trigger.off_threshold = 0.5
        trigger.resume_offset = BOOKEND_LENGTH
    var triggers:Array[SfxParameterTrigger] = [trigger]
    var event:SfxEvent = SfxEvent.new()
    event.name = &"engine"
    event.clips = clips
    event.triggers = triggers
    var events:Array[SfxEvent] = [event]
    var state:SfxParameter = SfxParameter.new()
    state.name = STATE_PARAMETER
    var parameters:Array[SfxParameter] = [state]
    var bank:SfxBank = SfxBank.new()
    bank.parameters = parameters
    bank.events = events
    _sound = SfxPlayer3D.new()
    _sound.bank = bank
    _vehicle.add_child(_sound)
    MaszynaVehicleSoundServer.bank_register(_sound.get_instance_id(), _vehicle_rid, {})


## The bank registered before the vehicle is a rail one and drawn (RailVehicle3D takes it then)
func _build(trigger_mode:MmdSoundCatalog.TriggerMode) -> void:
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
    _build(MmdSoundCatalog.TriggerMode.TOGGLE)
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
    _build(MmdSoundCatalog.TriggerMode.TOGGLE)
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


func test_sound_switched_on_while_the_camera_is_away_is_heard_past_its_opening_bookend() -> void:
    _build(MmdSoundCatalog.TriggerMode.TOGGLE)
    _camera.global_position = FAR
    await step(ticks(SETTLE))
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    await step(ticks(SETTLE))
    _camera.global_position = NEAR
    if not await wait_simulated_until(_playing, SETTLE, "the sound heard"):
        return

    # past the bookend, by however long the sound has played since
    assert_gt(float(_sound.get_event_visualization_state(&"engine")["event_time"]),
            BOOKEND_LENGTH - EVENT_TIME_TOLERANCE, "switched on unheard, it is heard past its opening bookend")


func test_sound_on_before_its_bank_is_built_is_heard_past_its_opening_bookend() -> void:
    _build_vehicle()
    _attach_controller()
    _camera.global_position = NEAR
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    await step(ticks(SETTLE))
    # the bank of a vehicle the camera has just come to, built with the sound on already
    _register_bank(MmdSoundCatalog.TriggerMode.TOGGLE)
    if not await wait_simulated_until(_playing, SETTLE, "the sound heard"):
        return

    # past the bookend, by however long the sound has played since
    assert_gt(float(_sound.get_event_visualization_state(&"engine")["event_time"]),
            BOOKEND_LENGTH - EVENT_TIME_TOLERANCE, "on before it was heard, it is heard past its opening bookend")


func test_one_shot_due_while_the_camera_is_away_is_dropped() -> void:
    _build(MmdSoundCatalog.TriggerMode.CHANGE)
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
    _register_bank(MmdSoundCatalog.TriggerMode.TOGGLE)
    _camera.global_position = NEAR
    VehicleServer.vehicle_send_command(_vehicle_rid, "battery", true)
    if not await wait_simulated_until(_playing, SETTLE, "the late bank heard"):
        return

    assert_true(_playing())


func test_bank_is_built_once_its_vehicle_is_within_earshot() -> void:
    _build_vehicle()
    var built:Array[int] = [0]
    _camera.global_position = FAR
    MaszynaVehicleSoundServer.vehicle_set_bank_builder(_vehicle_rid, func() -> void: built[0] += 1)
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
    MaszynaVehicleSoundServer.vehicle_set_bank_builder(_vehicle_rid, func() -> void: built[0] += 1)
    MaszynaVehicleSoundServer.vehicle_set_bank_builder(_vehicle_rid, Callable())
    _camera.global_position = NEAR
    # proved never called by the time a sweep takes
    await step(ticks(SETTLE))

    assert_eq(built[0], 0)


## A sound inside the cab is not heard where no listener sits in the vehicle (sound.cpp:981-1008)
func test_internal_sound_is_silent_without_an_occupied_listener_vehicle() -> void:
    _build_vehicle()
    _attach_controller()
    var proofing:SfxParameter = SfxParameter.new()
    proofing.name = &"soundproofing_cab"
    proofing.default_value = 1.0
    var parameters:Array[SfxParameter] = [proofing]
    var bank:SfxBank = SfxBank.new()
    bank.parameters = parameters
    _sound = SfxPlayer3D.new()
    _sound.bank = bank
    _vehicle.add_child(_sound)
    MaszynaVehicleSoundServer.bank_register(_sound.get_instance_id(), _vehicle_rid, {
        "proofing": [{"parameter": &"soundproofing_cab", "placement": MaszynaVehicleSoundServer.PLACEMENT_INTERNAL}],
    })

    assert_almost_eq(_sound.get_parameter(&"soundproofing_cab"), 0.0, 0.001)
