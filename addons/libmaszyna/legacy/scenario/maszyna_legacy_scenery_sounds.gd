@tool
extends RefCounted
class_name MaszynaLegacyScenerySounds

## The scenery's `sound` nodes. Loaded with the scenery as data, they are made audible
## (ScenerySoundServer) only while its scenario runs (MaszynaLegacyScenario), so a scenery loaded
## without one - in the editor - is silent. The sound events (MaszynaLegacySoundAction) find them
## here by the names they target.

## A scenery sound's range of -1 is heard everywhere; under it, an ambient sound is 0.4 as loud,
## fades out past its range to AMBIENT_FADE_END of it, and is not heard further than
## AMBIENT_CUTOFF_RANGE [m] from its place (sound.cpp:364-371, 1021-1024,
## audiorenderer.cpp:184-199: the fade reaches 0 at range + 0.75 range, squared - 1.25 range)
const UNLIMITED_RANGE:float = -1.0
## Where a scenery sound's file and its transcript are (AudioStreamManager.get_stream())
const SOUNDS_DIRECTORY:String = "sounds"
const AMBIENT_GAIN:float = 0.4
const AMBIENT_FADE_END:float = 1.25
const AMBIENT_CUTOFF_RANGE:float = 2750.0

var _data_by_name:Dictionary[String, MaszynaSoundData] = {}
var _sounds_by_name:Dictionary[String, RID] = {}
var _transcripts_by_name:Dictionary[String, Transcript] = {}


func _init(sounds:Array[MaszynaSoundData]) -> void:
    for sound:MaszynaSoundData in sounds:
        _data_by_name[sound.name.to_lower()] = sound


func has_sound(sound_name:String) -> bool:
    return _data_by_name.has(sound_name)


## The sound's RID in ScenerySoundServer, invalid while the scenario does not run
func get_sound(sound_name:String) -> RID:
    return _sounds_by_name.get(sound_name, RID())


## The sound's range [m] (the scenery node's rmax): a radio message reaches only as far
func get_reach(sound_name:String) -> float:
    return _data_by_name[sound_name].range_max


## The sound's transcript, null for a sound with none
func get_transcript(sound_name:String) -> Transcript:
    return _transcripts_by_name.get(sound_name)


## Every sound made an event pair of ScenerySoundServer's one bank, played once or looped;
## `progressed` is told the share of them made (0..1) after each
func build(progressed:Callable = Callable()) -> void:
    var built_count:int = 0
    for sound_name:String in _data_by_name:
        var sound:MaszynaSoundData = _data_by_name[sound_name]
        await SceneryInstancer.frame_budget_wait()
        # heard as far as a vehicle's sound of the same range (sound_source::range(), sound.cpp:364-389)
        var source:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
        source.range = sound.range_max
        var spatial_config:SfxSpatialConfig = MmdSoundEventBuilder._build_spatial_config(source)
        # a range under -1 is an ambient sound: on the listener, as a negative range always is, but
        # heard only within reach of its place and 0.4 as loud (sound.cpp:364-371, 1021-1024,
        # audiorenderer.cpp:158-199); -1 is heard everywhere
        var ambient:bool = sound.range_max < UNLIMITED_RANGE
        if ambient:
            spatial_config.max_distance = minf(absf(sound.range_max) * AMBIENT_FADE_END, AMBIENT_CUTOFF_RANGE)
        # the voices of the one player at the origin stand at the sound's own place
        spatial_config.position = sound.position
        var bank_events:Array[SfxEvent] = [
            ScenerySoundServer.event_build(MmdSoundEventBuilder.build_stream(sound.file, false, ""), spatial_config),
            ScenerySoundServer.event_build(MmdSoundEventBuilder.build_stream(sound.file, true, ""), spatial_config),
        ]
        if ambient:
            for sound_event:SfxEvent in bank_events:
                sound_event.master_track.volume_db = linear_to_db(AMBIENT_GAIN)
        # the once and the looped event are one sound source of the original
        MmdSoundEventBuilder.shape_emitter(bank_events, null, 0.0)
        # streamed as far as it is heard; heard everywhere (-1), it is never out of reach
        _sounds_by_name[sound_name] = ScenerySoundServer.sound_create(
                bank_events[0], bank_events[1], sound.position, spatial_config.max_distance)
        _transcripts_by_name[sound_name] = MaszynaLegacySoundCaption.from_sound_file(
                UserSettings.get_maszyna_game_dir().path_join(SOUNDS_DIRECTORY).path_join(sound.file))
        built_count += 1
        if progressed.is_valid():
            progressed.call(float(built_count) / _data_by_name.size())


## The sounds freed; the data stays for the next build()
func clear() -> void:
    for sound:RID in _sounds_by_name.values():
        ScenerySoundServer.sound_free(sound)
    _sounds_by_name.clear()
    _transcripts_by_name.clear()
