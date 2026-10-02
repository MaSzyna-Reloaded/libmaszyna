extends Node

## The scenery's sounds (a `.scn` `sound` node, sound_source in the original, sound.cpp) - one
## SfxPlayer3D and one SfxBank for all of them, every sound an event pair placed by its own
## SfxSpatialConfig.position. The player stands at the origin, so a voice's place is its world
## place, and the hard cut is off: the distance of the player says nothing of its voices.
##
## A sound with a range is streamed (SceneryStreamingServer) as far as it is heard: a loop plays
## only while the camera is in reach of it, and resumes when it comes back. A sound heard
## everywhere (range -1) is always in reach.

enum Playback {
    ## played once (sound event mode 1)
    ONCE,
    ## looped until stopped (sound event mode -1)
    LOOP,
}

class SoundState:
    var play_event:SfxEvent
    var loop_event:SfxEvent
    var position:Vector3
    var stream_rid:RID = RID()
    ## In reach of the camera: its events may play
    var streamed:bool = true
    ## Asked to loop; a loop out of reach starts when the sound comes into reach
    var looping:bool = false

var _sounds:Dictionary[RID, SoundState] = {}
var _next_sound_id:int = 0
var _stream_owner:int = -1
var _bank:SfxBank = SfxBank.new()
var _player:SfxPlayer3D = SfxPlayer3D.new()


func _ready() -> void:
    _player.name = "ScenerySounds"
    _player.bank = _bank
    _player.hard_cut_enabled = false
    add_child(_player)
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_SOUNDS, adopt_sound, sound_free)


## An event of a sound: its stream, heard as spatial_config says
static func event_build(stream:AudioStream, spatial_config:SfxSpatialConfig) -> SfxEvent:
    var clip:SfxClip = SfxClip.new()
    clip.stream = stream
    var event:SfxEvent = SfxEvent.new()
    event.spatial_config = spatial_config
    var clips:Array[SfxClip] = [clip]
    event.clips = clips
    return event


## A sound a SceneryStreamingProvider supplies: looped at its place within its reach for as long as
## its cell is supplied (ScenerySoundPlacement)
func adopt_sound(placement:ScenerySoundPlacement, _scenario:RID) -> RID:
    var spatial_config:SfxSpatialConfig = SfxSpatialConfig.new()
    spatial_config.position = placement.position
    spatial_config.max_distance = placement.reach
    var play_event:SfxEvent = event_build(placement.stream, spatial_config)
    var loop_event:SfxEvent = event_build(placement.stream, spatial_config)
    play_event.master_track.volume_db = placement.volume_db
    loop_event.master_track.volume_db = placement.volume_db
    var sound_rid:RID = sound_create(play_event, loop_event, placement.position, placement.reach)
    sound_play(sound_rid, Playback.LOOP)
    return sound_rid


## A sound at `position`, with its once and looped events (their names are the server's; their
## SfxSpatialConfig.position is the sound's place), heard as far as `reach` [m]; 0 or less for one
## heard everywhere
func sound_create(play_event:SfxEvent, loop_event:SfxEvent, position:Vector3, reach:float) -> RID:
    _next_sound_id += 1
    var sound_rid:RID = rid_from_int64(_next_sound_id)
    var state:SoundState = SoundState.new()
    state.play_event = play_event
    state.loop_event = loop_event
    state.position = position
    play_event.name = StringName("%d/play" % _next_sound_id)
    loop_event.name = StringName("%d/loop" % _next_sound_id)
    _sounds[sound_rid] = state
    var events:Array[SfxEvent] = _bank.events
    events.append_array([play_event, loop_event])
    _bank.events = events
    # every sound plays at most one of its two events at a time (exclusive, sound.cpp:403-421)
    _player.max_tracks = _sounds.size()
    if reach > 0.0:
        if _stream_owner < 0:
            _stream_owner = SceneryStreamingServer.owner_create("sounds", Callable(), _stream_build, _stream_clear)
        state.streamed = false
        state.stream_rid = SceneryStreamingServer.stream_register(_stream_owner, sound_rid, position, reach)
    return sound_rid


func sound_free(sound_rid:RID) -> void:
    var state:SoundState = _sounds.get(sound_rid)
    if not state:
        return
    if state.stream_rid.is_valid():
        SceneryStreamingServer.stream_free(state.stream_rid)
    _player.stop(state.play_event.name)
    _player.stop(state.loop_event.name)
    _sounds.erase(sound_rid)
    var events:Array[SfxEvent] = _bank.events
    events.erase(state.play_event)
    events.erase(state.loop_event)
    _bank.events = events


## Plays the sound once or loops it; false when it is playing already, which it goes on doing
## (exclusive, sound_source::play_basic(), sound.cpp:403-421) - but asked to play once, a loop is
## not started again when the camera comes back, as the original's play() replaces the flags
## before it looks (sound.cpp:350-356). Out of reach, a sound played once is not heard, and a loop
## waits for the camera.
func sound_play(sound_rid:RID, playback:Playback) -> bool:
    var state:SoundState = _sounds.get(sound_rid)
    if not state:
        return false
    state.looping = playback == Playback.LOOP
    if _player.is_playing(state.play_event.name) or _player.is_playing(state.loop_event.name):
        return false
    if state.streamed:
        _player.play(state.loop_event.name if playback == Playback.LOOP else state.play_event.name)
    return true


func sound_stop(sound_rid:RID) -> void:
    var state:SoundState = _sounds.get(sound_rid)
    if not state:
        return
    state.looping = false
    _player.stop(state.play_event.name)
    _player.stop(state.loop_event.name)


func sound_is_playing(sound_rid:RID) -> bool:
    var state:SoundState = _sounds.get(sound_rid)
    return state and (_player.is_playing(state.play_event.name) or _player.is_playing(state.loop_event.name))


## The event a radio message plays (CabinSystem.send_radio_message())
func sound_get_play_event(sound_rid:RID) -> SfxEvent:
    var state:SoundState = _sounds.get(sound_rid)
    return state.play_event if state else null


func sound_get_position(sound_rid:RID) -> Vector3:
    var state:SoundState = _sounds.get(sound_rid)
    return state.position if state else Vector3.ZERO


func _stream_build(sound_rid:RID, _preloaded:Variant) -> void:
    var state:SoundState = _sounds.get(sound_rid)
    if not state:
        return
    state.streamed = true
    if state.looping:
        _player.play(state.loop_event.name)


func _stream_clear(sound_rid:RID) -> void:
    var state:SoundState = _sounds.get(sound_rid)
    if not state:
        return
    state.streamed = false
    _player.stop(state.play_event.name)
    _player.stop(state.loop_event.name)
    # out of reach the sound is not played, so its files need not be in memory
    for clip:SfxClip in state.play_event.clips + state.loop_event.clips:
        var stream:MaszynaAudioStream = clip.stream as MaszynaAudioStream
        if stream:
            stream.unload()
