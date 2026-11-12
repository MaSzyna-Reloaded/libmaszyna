@tool
extends RefCounted
class_name MaszynaScenerySounds

## The scenery's `sound` nodes. Loaded with the scenery as data, they are made audible
## (ScenerySoundServer) only while its scenario runs (MaszynaScenario), so a scenery loaded
## without one - in the editor - is silent. The sound events (MaszynaSoundAction) find them
## here by the names they target.

## A scenery sound's range of -1 is heard everywhere, on the listener (audiorenderer.cpp:146-163)
const UNLIMITED_RANGE:float = -1.0
## Where a scenery sound's file and its transcript are (AudioStreamManager.get_stream())
const SOUNDS_DIRECTORY:String = "sounds"
## Under -1 a range is an ambient sound's: on the listener, 0.4 as loud (sound.cpp:1021-1024)
const AMBIENT_GAIN:float = 0.4
## A sound placed by its range is as loud as OpenAL's inverse distance, clamped, makes it: full up
## to its range over REFERENCE_DIVISOR, falling by ROLLOFF past it (alDistanceModel(),
## AL_REFERENCE_DISTANCE, AL_ROLLOFF_FACTOR, audiorenderer.cpp:178-183, 300-311, 412)
const REFERENCE_DIVISOR:float = 16.0
const ROLLOFF:float = 1.75
## Past its range a sound fades out to nothing at its range and FADE_SHARE of it more, squared
## (audiorenderer.cpp:184-199)
const FADE_SHARE:float = 0.75
## A sound played once is heard within START_REACH_SHARE of its range, START_REACH_MAX [m] at most
## (sound.cpp:364-371); a playing one stops past STOP_REACH_SHARE of it, STOP_REACH_MAX at most
## (audiorenderer.cpp:25, 146-155)
const START_REACH_SHARE:float = 5.0
const START_REACH_MAX:float = 2750.0
const STOP_REACH_SHARE:float = 7.5
const STOP_REACH_MAX:float = 3000.0
## The points of a sound's loudness over the distance (its SfxParameterModulation's curve): from
## the reference distance to the range, a step each the same share further, and along the fade
const FALLOFF_POINTS:int = 48
const FADE_POINTS:int = 16
## The bus the outside is heard on - behind the walls of a cab the listener sits in, as the original
## proofs the environment's sounds by the listener's vehicle (sound.cpp:1012-1018; mmd_sound_bank_instancer.gd)
const EXTERIOR_BUS:StringName = &"Exterior"

var _data_by_name:Dictionary[String, SoundData] = {}
var _sounds_by_name:Dictionary[String, RID] = {}
var _transcripts_by_name:Dictionary[String, Transcript] = {}


func _init(sounds:Array[SoundData]) -> void:
    for sound:SoundData in sounds:
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
    ScenerySoundServer.set_bus(EXTERIOR_BUS)
    var built_count:int = 0
    for sound_name:String in _data_by_name:
        var sound:SoundData = _data_by_name[sound_name]
        await SceneryServer.frame_budget_wait()
        # on the listener, unless placed by a range of its own (audiorenderer.cpp:158-163): the voices
        # of the one player at the origin stand at the sound's own place
        var placed:bool = sound.range_max >= 0.0
        var spatial_config:SfxSpatialConfig = SfxSpatialConfig.new()
        spatial_config.position = sound.position
        spatial_config.attenuation_model = AudioStreamPlayer3D.ATTENUATION_DISABLED
        spatial_config.max_distance = 0.0
        spatial_config.panning_strength = 1.0 if placed else 0.0
        var bank_events:Array[SfxEvent] = [
            ScenerySoundServer.event_build(MmdSoundEventBuilder.build_stream(sound.file, false, ""), spatial_config),
            ScenerySoundServer.event_build(MmdSoundEventBuilder.build_stream(sound.file, true, ""), spatial_config),
        ]
        var reach:float = absf(sound.range_max)
        var heard_everywhere:bool = sound.range_max == UNLIMITED_RANGE
        if not heard_everywhere:
            var loudness:SfxParameterModulation = _loudness(sound.range_max)
            for sound_event:SfxEvent in bank_events:
                var modulations:Array[SfxParameterModulation] = [loudness]
                sound_event.parameter_modulations = modulations
                if sound.range_max < UNLIMITED_RANGE:
                    sound_event.master_track.volume_db = linear_to_db(AMBIENT_GAIN)
        # the once and the looped event are one sound source of the original
        MmdSoundEventBuilder.shape_emitter(bank_events, null, 0.0)
        # streamed as far as a playing sound goes on; heard everywhere (-1), it is never out of reach
        _sounds_by_name[sound_name] = ScenerySoundServer.sound_create(
                bank_events[0], bank_events[1], sound.position,
                0.0 if heard_everywhere else minf(reach * STOP_REACH_SHARE, STOP_REACH_MAX),
                0.0 if heard_everywhere else minf(reach * START_REACH_SHARE, START_REACH_MAX))
        _transcripts_by_name[sound_name] = MaszynaSoundCaption.from_sound_file(
                MaszynaGameDirectory.get_game_dir().path_join(SOUNDS_DIRECTORY).path_join(sound.file))
        built_count += 1
        if progressed.is_valid():
            progressed.call(float(built_count) / _data_by_name.size())


## How loud a sound of the range is with the camera that far from its place: by the inverse
## distance when placed by its range, faded out past the range either way. The curve's points stand
## where the loudness bends: at the reference distance, closer together where it falls fast, and
## along the fade.
static func _loudness(range_max:float) -> SfxParameterModulation:
    var reach:float = absf(range_max)
    var reference:float = reach / REFERENCE_DIVISOR
    var curve:Curve = Curve.new()
    curve.max_domain = maxf(reach * (1.0 + FADE_SHARE), 1.0)
    var distances:PackedFloat64Array = [0.0]
    for index:int in FALLOFF_POINTS:
        distances.append(reference * pow(REFERENCE_DIVISOR, float(index) / (FALLOFF_POINTS - 1)))
    for index:int in range(1, FADE_POINTS):
        distances.append(reach + reach * FADE_SHARE * float(index) / (FADE_POINTS - 1))
    for distance:float in distances:
        var gain:float = 1.0
        if range_max >= 0.0:
            gain = reference / (reference + ROLLOFF * (maxf(distance, reference) - reference)) if reference > 0.0 else 0.0
        if distance > reach:
            var fade:float = (distance * distance - reach * reach) / pow(reach * FADE_SHARE, 2.0) if reach > 0.0 else 1.0
            gain *= lerpf(1.0, 0.0, clampf(fade, 0.0, 1.0))
        # one point a distance: a sound of no range has all of them at 0
        if distance <= curve.max_domain and (curve.point_count == 0
                or distance > curve.get_point_position(curve.point_count - 1).x):
            curve.add_point(Vector2(distance, gain), 0.0, 0.0, Curve.TANGENT_LINEAR, Curve.TANGENT_LINEAR)
    var loudness:SfxParameterModulation = SfxParameterModulation.new()
    loudness.parameter_name = ScenerySoundServer.DISTANCE_PARAMETER
    loudness.target = SfxParameterModulation.Target.GAIN
    loudness.min_domain = 0.0
    loudness.max_domain = curve.max_domain
    loudness.curve = curve
    return loudness


## The sounds freed; the data stays for the next build()
func clear() -> void:
    for sound:RID in _sounds_by_name.values():
        ScenerySoundServer.sound_free(sound)
    _sounds_by_name.clear()
    _transcripts_by_name.clear()
