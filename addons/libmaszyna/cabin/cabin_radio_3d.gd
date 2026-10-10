extends Node3D
class_name CabinRadio3D

## The cab's radio loudspeaker, where the cab puts it (TTrain::m_radiosound, Train.cpp:10732-10739).
## Every radio message within reach plays on it, on any channel and with the radio off - muted
## until the radio is on, powered and tuned to the message's channel, as the original starts them
## all in case the radio is tuned mid-message (TTrain::radio_message(), Train.cpp:11010-11029).
## While messages play their gain follows the radio, and the Radio-Stop alarm loops while the
## radio is on and its Radio-Stop is set (TTrain::update_sounds_radio(), Train.cpp:10227-10262).
## The cab's lamps of the message and of the Radio-Stop read their state from here
## (btLampkaRadioMessage, btLampkaRadioStop, Train.cpp:9129-9130).

## Original engine: m_radiosound's range, 2 * EU07_SOUND_CABCONTROLSCUTOFFRANGE (Train.h:845, sound.h:17)
const RANGE:float = 15.0
const RADIO_MESSAGE:StringName = &"radio_message"
const RADIO_STOP:StringName = &"radio_stop"
## The parameter a message's gain follows the radio by
const GAIN_PARAMETER:StringName = &"radio_gain"
## The cab's state the lamp of a message heard is lit by (CabinSystem.vehicle_state_value())
const MESSAGE_PLAYED_KEY:String = "radio_message_played"
## The cab's state the lamp of the Radio-Stop is lit by
const RADIO_STOP_LAMP_KEY:String = "radio_stop_lamp"
const BUS:StringName = &"Cabin"

## The Radio-Stop alarm, the MMD's `radiostop:` (Train.cpp:10340); null when the cab has none
@export var radio_stop_alarm:SfxEvent = null

var _vehicle_rid:RID
var _radio:RailVehicleRadio
## [channel:int, player:SfxPlayer3D] of each message playing
var _messages:Array[Array] = []
var _alarm_player:SfxPlayer3D
var _message_played:bool = false


func _ready() -> void:
    CabinSystem.radio_message_sent.connect(_on_radio_message_sent)
    if radio_stop_alarm:
        _alarm_player = _new_player()
        var bank := SfxBank.new()
        var events:Array[SfxEvent] = [radio_stop_alarm]
        bank.events = events
        _alarm_player.bank = bank
        add_child(_alarm_player)
    set_process(false)


func _exit_tree() -> void:
    CabinSystem.radio_message_sent.disconnect(_on_radio_message_sent)
    if _vehicle_rid.is_valid():
        CabinSystem.state_computed_value_unregister(_vehicle_rid, MESSAGE_PLAYED_KEY)
        CabinSystem.state_computed_value_unregister(_vehicle_rid, RADIO_STOP_LAMP_KEY)


func set_vehicle_rid(vehicle_rid:RID) -> void:
    _vehicle_rid = vehicle_rid
    _radio = CabinSystem.vehicle_component(vehicle_rid, VehicleComponentType.COMPONENT_RADIO) as RailVehicleRadio
    CabinSystem.state_computed_value_register(vehicle_rid, MESSAGE_PLAYED_KEY, func() -> bool: return _message_played)
    CabinSystem.state_computed_value_register(vehicle_rid, RADIO_STOP_LAMP_KEY,
            func() -> bool: return _radio != null and _radio.get_enabled() and _radio.get_radio_stop_active())
    # the alarm is watched for as long as the cab has one and a radio to sound it
    set_process(_radio != null and _alarm_player != null)


func _new_player() -> SfxPlayer3D:
    var player := SfxPlayer3D.new()
    player.bus = BUS
    player.max_distance = int(RANGE)
    return player


## The gain of a message on `channel`: the radio's volume while it is on, powered and tuned to it
func _gain(channel:int) -> float:
    if _radio and _radio.get_powered() and _radio.get_channel() == channel:
        return _radio.get_volume()
    return 0.0


func _on_radio_message_sent(
    message:SfxEvent, transcript:Transcript, channel:int, position:Vector3, reach:float
) -> void:
    # a vehicle without a radio hears nothing it could ever tune to
    if not _radio or message == null:
        return
    if reach > 0.0 and RailVehicleServer.vehicle_get_transform(_vehicle_rid).origin.distance_to(position) > reach:
        return
    var played:SfxEvent = message.duplicate(true)
    played.name = RADIO_MESSAGE
    played.spatial_config = null
    var gain := SfxParameterModulation.new()
    gain.parameter_name = GAIN_PARAMETER
    gain.target = SfxParameterModulation.Target.GAIN
    gain.default_value = 0.0
    played.parameter_modulations.append(gain)
    var player:SfxPlayer3D = _new_player()
    var bank := SfxBank.new()
    var events:Array[SfxEvent] = [played]
    bank.events = events
    player.bank = bank
    add_child(player)
    var start_gain:float = _gain(channel)
    player.play(RADIO_MESSAGE, {GAIN_PARAMETER: start_gain})
    _messages.append([channel, player])
    set_process(true)
    # its transcript is shown when it is heard at all (sound_source::update_counter(), sound.cpp:955)
    if transcript and start_gain > 0.0:
        TranscriptSystem.add(transcript)


## update_sounds_radio() (Train.cpp:10227-10262): the finished messages go, the rest follow the
## radio - a handful at most
func _process(_delta:float) -> void:
    var finished:Array[Array] = _messages.filter(
            func(message:Array) -> bool: return not (message[1] as SfxPlayer3D).is_playing(RADIO_MESSAGE))
    for message:Array in finished:
        _messages.erase(message)
        (message[1] as SfxPlayer3D).queue_free()
    var played:bool = false
    for message:Array in _messages:
        var gain:float = _gain(message[0])
        (message[1] as SfxPlayer3D).modulate(RADIO_MESSAGE, {GAIN_PARAMETER: gain})
        played = played or (_radio.get_powered() and _radio.get_channel() == message[0])
    if _alarm_player:
        if _radio.get_powered() and _radio.get_radio_stop_active():
            if not _alarm_player.is_playing(RADIO_STOP):
                _alarm_player.play(RADIO_STOP)
            played = true
        elif _alarm_player.is_playing(RADIO_STOP):
            _alarm_player.stop(RADIO_STOP)
    _message_played = played
    set_process(not _messages.is_empty() or _alarm_player != null)
