extends MaszynaGutTest

## The cab radio (CabinRadio3D) as TTrain's (Train.cpp:10227-10262, 11010-11029): every message in
## reach plays, muted until the radio is on, powered and on its channel; switching on or tuning in
## mid-message makes it heard, switching off mutes it; messages overlap; the Radio-Stop alarm loops
## while the radio is on and its Radio-Stop set, and the lamps of the message and of the Radio-Stop
## follow.

const LOOP_PATH:String = "res://tests/fixtures/dynamic/test/animated_v1/test_loop.ogg"
const CHANNEL:int = 3
## Presses of the volume knob down from any volume to silence (RailVehicleRadio.VOLUME_STEP)
const PRESSES_TO_SILENCE:int = 8
## Below it a message is not heard: a gain of 0 plays at -80 dB (gnd-sfx's floor of 0.0001)
const SILENT_DB:float = -60.0
const VOLUME_TOLERANCE_DB:float = 0.1
const TRANSCRIPT_LINE:String = "Pociag do Krakowa"
## How long the line shows [s]
const TRANSCRIPT_SECONDS:float = 5.0

const LOCOMOTIVE_PATH:String = "res://tests/fixtures/dynamic/pkp/ep09_v1/104e-039.fiz"
const TRACK:String = "TestCabinRadioTrack"
const TRACK_LENGTH:float = 200.0
const OFFSET:float = 50.0

var vehicle_rid:RID
var vehicle:RailVehicle3D
var track:RID
var radio:CabinRadio3D


func _event(event_name:StringName) -> SfxEvent:
    var stream:AudioStreamOggVorbis = AudioStreamOggVorbis.load_from_file(ProjectSettings.globalize_path(LOOP_PATH))
    stream.loop = true
    var clip := SfxClip.new()
    clip.stream = stream
    var event := SfxEvent.new()
    event.name = event_name
    event.clips = [clip]
    return event


## The locomotive stands on a track: a Radio-Stop it sends it hears itself, as every vehicle in
## reach (basic_region::RadioStop, scene.cpp:1269)
func before_each() -> void:
    track = build_track(TRACK, TRACK_LENGTH)
    var physics_node:VehiclePhysicsNode = build_vehicle_node("TestCabinRadio",
            MaszynaFizManager.load_description_at(LOCOMOTIVE_PATH), 0.0, MaszynaVehicleData.DriverType.DRIVER_HEAD)
    vehicle = RailVehicle3D.new()
    vehicle.start_track_name = TRACK
    vehicle.start_track_offset = OFFSET
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    vehicle_rid = physics_node.get_vehicle_rid()
    await step(3)
    VehicleServer.vehicle_send_command(vehicle_rid, "battery", true)
    radio = CabinRadio3D.new()
    radio.radio_stop_alarm = _event(CabinRadio3D.RADIO_STOP)
    add_child_autofree(radio)
    radio.set_vehicle_rid(vehicle_rid)
    await step(2)


func after_each() -> void:
    free_rail_vehicle(vehicle)
    TrackServer.track_free(track)
    TrackServer.topology_rebuild()


func _message_played() -> bool:
    return CabinSystem.vehicle_state_value(vehicle_rid, CabinRadio3D.MESSAGE_PLAYED_KEY, false)


func _send(channel:int) -> void:
    SceneryVirtualRadio.new().transmit(Vector3.ZERO, channel, 0.0, RadioServer.KIND_MESSAGE,
            {"sound": _event(&"message"), "transcript": null})
    await step(2)


func _tune(channel:int) -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio_channel_set", channel)
    await step(2)


func _players() -> int:
    return radio.get_children().filter(func(child:Node) -> bool: return child is SfxPlayer3D).size()


func test_a_message_off_the_channel_is_heard_once_tuned_in() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await _tune(1)
    await _send(CHANNEL)
    assert_false(_message_played(), "playing muted on another channel")
    await _tune(CHANNEL)
    assert_true(_message_played(), "heard once tuned in mid-message")


func test_a_message_is_heard_once_the_radio_is_switched_on_mid_message() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", false)
    await _tune(CHANNEL)
    await _send(CHANNEL)
    assert_false(_message_played(), "playing muted with the radio off")
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await step(2)
    assert_true(_message_played(), "heard once switched on mid-message")


func test_switching_the_radio_off_mutes_a_message() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await _tune(CHANNEL)
    await _send(CHANNEL)
    assert_true(_message_played())
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", false)
    await step(2)
    assert_false(_message_played(), "muted with the radio off")


## A message is played as loud as the radio is set on its channel, and silent off it or with the
## radio off (TTrain::update_sounds_radio(), Train.cpp:10227-10262)
func test_a_message_follows_the_radio_volume_channel_and_power() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await _tune(CHANNEL)
    await _send(CHANNEL)
    var volume:float = (VehicleServer.vehicle_component_get(
            vehicle_rid, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio).get_volume()
    assert_almost_eq(_message_volume_db(), linear_to_db(volume), VOLUME_TOLERANCE_DB, "as loud as the radio is set")

    await _tune(1)
    assert_lt(_message_volume_db(), SILENT_DB, "silent off its channel")

    await _tune(CHANNEL)
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", false)
    await step(2)
    assert_lt(_message_volume_db(), SILENT_DB, "silent with the radio off")


## How loud the last message plays [dB]: the volume its player's voice is set to
func _message_volume_db() -> float:
    var players:Array[Node] = radio.get_children().filter(func(child:Node) -> bool: return child is SfxPlayer3D)
    var loudest:float = -INF
    for voice:Node in players.back().get_children(true):
        if voice is AudioStreamPlayer3D and (voice as AudioStreamPlayer3D).playing:
            loudest = maxf(loudest, (voice as AudioStreamPlayer3D).volume_db)
    return loudest

func test_messages_overlap() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await step(2)
    var before:int = _players()
    await _send(CHANNEL)
    await _send(CHANNEL)
    assert_eq(_players() - before, 2, "each message on a player of its own")


func test_the_radio_stop_alarm_and_its_lamp() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await step(2)
    assert_false(CabinSystem.vehicle_state_value(vehicle_rid, CabinRadio3D.RADIO_STOP_LAMP_KEY, true))
    VehicleServer.vehicle_send_command(vehicle_rid, "radio_stop", true)
    await step(2)
    assert_true(CabinSystem.vehicle_state_value(vehicle_rid, CabinRadio3D.RADIO_STOP_LAMP_KEY, false), "the lamp")
    assert_true(_message_played(), "the alarm sounds on the radio")


## A message the radio receives as it starts shows its transcript however low the volume is: it
## starts at full gain (TTrain::radio_message(), Train.cpp:11047-11048)
func test_a_received_message_shows_its_transcript_at_any_volume() -> void:
    VehicleServer.vehicle_send_command(vehicle_rid, "radio", true)
    await _tune(CHANNEL)
    VehicleServer.vehicle_send_command(vehicle_rid, "radio_volume_decrease", PRESSES_TO_SILENCE)
    var transcript := Transcript.new()
    transcript.add_line(TRANSCRIPT_LINE, 0.0, TRANSCRIPT_SECONDS)
    SceneryVirtualRadio.new().transmit(Vector3.ZERO, CHANNEL, 0.0, RadioServer.KIND_MESSAGE,
            {"sound": _event(&"message"), "transcript": transcript})
    await step(2)
    assert_has(TranscriptSystem.get_lines(), TRANSCRIPT_LINE)
