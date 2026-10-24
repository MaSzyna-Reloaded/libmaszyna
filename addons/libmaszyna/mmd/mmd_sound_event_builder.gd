extends RefCounted
class_name MmdSoundEventBuilder

## Converts one MmdSoundSourceDefinition into an in-memory SfxEvent (plain .new() + property
## assignment - gnd-sfx resources need no .tres serialization to work at runtime, confirmed by
## reading every gnd-sfx resource class). A definition can combine BOTH shapes at once (e.g.
## "engine": begin/end from the vehicle's separate `ignition:`/`shutdown:` MMD labels, chunks from
## `engine:`'s own soundN:/pitchN: table - MmdSoundBankInstancer merges those three MMD-level
## sound sources into one definition before calling build()):
## - non-empty chunks -> 1 SfxAutomation, one clip per chunk, each on its OWN SfxTrack with a
##   fade_in_curve/fade_out_curve/pitch_curve so adjacent chunks actually crossfade instead of
##   cutting - ported from audio/sound.cpp's own sound_source::update_crossfade()/deserialize()
##   math (the gain/pitch interpolation the original engine performs at playback time), not
##   guessed. Matches engine.tres's own per-clip-track shape.
## - sound_begin and/or sound_end (+ optional sound_main) -> up to 3 plain clips, end clip
##   trigger_mode = TRIGGER_SUSTAIN (matches horn1.tres/horn2.tres's begin/trwa/stop shape). When
##   chunks are ALSO present, these become the automation's start/stop bookends on the same event
##   (matches engine.tres's own combined clips+automations shape) instead of a separate event.
## - sound_main only, no begin/end/chunks -> 1 looping clip (matches oil_pump.tres's shape).

const _CROSSFADE_CURVE_SEGMENTS:int = 8
const _CROSSFADE_LOG_FACTOR:float = -0.57
## An emitter's own pitch factor when the MMD gives no pitchvariation: (sound.cpp:374-377)
const DEFAULT_PITCH_VARIATION_MIN:float = 0.975
const DEFAULT_PITCH_VARIATION_MAX:float = 1.025


static func build(
        definition:MmdSoundSourceDefinition, event_name:StringName,
        sound_parameter:StringName = &"", parameterized:bool = false,
        soundproofed:bool = false, loop:bool = true) -> SfxEvent:
    var event := SfxEvent.new()
    event.name = event_name

    var has_bookends:bool = definition.sound_begin or definition.sound_end
    var has_chunks:bool = not not definition.chunks

    if has_chunks:
        var built:Dictionary = _build_automation(definition, sound_parameter, loop)
        event.automations = [built["automation"]]
        event.tracks = built["tracks"]

    if has_bookends:
        event.clips = _build_begin_main_end_clips(definition)
    elif not has_chunks and definition.sound_main:
        var clip := SfxClip.new()
        clip.stream = build_stream(definition.sound_main, loop, definition.source_file.get_base_dir())
        event.clips = [clip]

    if parameterized:
        event.parameter_modulations = [
            _build_modulation(&"gain", SfxParameterModulation.Target.GAIN, 0.0, 100.0, 1.0),
            _build_modulation(&"pitch", SfxParameterModulation.Target.PITCH, 0.1, 10.0, 1.0),
            _build_modulation(&"unit_size", SfxParameterModulation.Target.UNIT_SIZE, 0.1, 8.0, 1.0),
        ]

    if event_name == &"engine":
        event.parameter_modulations.append(
                _build_modulation(&"engine_gain", SfxParameterModulation.Target.GAIN, 0.0, 2.0, 1.0))

    if parameterized or soundproofed:
        add_soundproofing_modulations(event)
        event.spatial_config = _build_spatial_config(definition)

    return event


static func add_soundproofing_modulations(event:SfxEvent) -> void:
    var has_gain:bool = false
    var has_unit_size:bool = false
    for modulation:SfxParameterModulation in event.parameter_modulations:
        if not modulation.parameter_name == &"soundproofing":
            continue
        has_gain = has_gain or modulation.target == SfxParameterModulation.Target.GAIN
        has_unit_size = has_unit_size or modulation.target == SfxParameterModulation.Target.UNIT_SIZE
    if not has_gain:
        event.parameter_modulations.append(
                _build_modulation(&"soundproofing", SfxParameterModulation.Target.GAIN, 0.0, 1.0, 1.0))
    if not has_unit_size:
        event.parameter_modulations.append(
                _build_modulation(&"soundproofing", SfxParameterModulation.Target.UNIT_SIZE, 0.0, 1.0, 1.0))


static func _build_modulation(
        parameter_name:StringName, target:int,
        minimum:float, maximum:float, default_value:float) -> SfxParameterModulation:
    var modulation := SfxParameterModulation.new()
    modulation.parameter_name = parameter_name
    modulation.target = target
    modulation.min_domain = minimum
    modulation.max_domain = maximum
    modulation.default_value = default_value
    var curve := Curve.new()
    curve.min_domain = minimum
    curve.max_domain = maximum
    curve.min_value = minimum
    curve.max_value = maximum
    curve.add_point(Vector2(minimum, minimum))
    curve.add_point(Vector2(maximum, maximum))
    for point_index:int in range(curve.point_count):
        curve.set_point_left_mode(point_index, Curve.TANGENT_LINEAR)
        curve.set_point_right_mode(point_index, Curve.TANGENT_LINEAR)
    modulation.curve = curve
    return modulation


static func _build_spatial_config(definition:MmdSoundSourceDefinition) -> SfxSpatialConfig:
    var config := SfxSpatialConfig.new()
    config.position = definition.offset
    if definition.range < 0.0:
        config.attenuation_model = AudioStreamPlayer3D.ATTENUATION_DISABLED
        config.max_distance = 0.0
        config.panning_strength = 0.0
    else:
        config.unit_size = maxf(definition.range / 16.0, 0.01)
        config.max_distance = minf(definition.range * 7.5, 2750.0)
    return config


static func _build_begin_main_end_clips(definition:MmdSoundSourceDefinition) -> Array[SfxClip]:
    var clips:Array[SfxClip] = []
    # gnd-sfx's SfxEvent is a fixed timeline, unlike the original engine's real-time "play begin
    # once, then switch to looping main" state machine - the main clip's start is approximated
    # from the begin clip's own real duration (queried from the actual audio asset, not guessed)
    # so it starts right as the begin clip finishes.
    var main_offset:float = AudioStreamManager.get_stream_length(definition.sound_begin, definition.source_file.get_base_dir())

    if definition.sound_begin:
        var begin_clip := SfxClip.new()
        begin_clip.stream = build_stream(definition.sound_begin, false, definition.source_file.get_base_dir())
        # the span is known before the stream is loaded, so an event started past it
        # (TrainSoundSystem, a running sound heard again) does not play it
        begin_clip.length = main_offset
        begin_clip.bookend = true
        clips.append(begin_clip)

    if definition.sound_main:
        var main_clip := SfxClip.new()
        main_clip.stream = build_stream(definition.sound_main, true, definition.source_file.get_base_dir())
        main_clip.offset = main_offset
        clips.append(main_clip)

    if definition.sound_end:
        var end_clip := SfxClip.new()
        end_clip.stream = build_stream(definition.sound_end, false, definition.source_file.get_base_dir())
        end_clip.trigger_mode = SfxClip.TriggerMode.TRIGGER_SUSTAIN
        end_clip.bookend = true
        clips.append(end_clip)

    return clips


## Ports audio/sound.cpp's sound_source::deserialize()'s chunk fadein/fadeout placement (the
## ACTIVE code path - a commented-out alternate crossfade-interpolation formula exists in the same
## function but is dead code, never executed) AND update_crossfade()'s per-chunk gain/pitch
## interpolation (real-time playback math, not the deserialize-time one) as gnd-sfx fade_in_curve/
## fade_out_curve/pitch_curve pairs, so gnd-sfx's own crossfade blending (SfxPlaybackRuntime
## multiplies clip.pitch_curve * automation.pitch_curve, and blends simultaneously-active clips'
## fade curve outputs) reproduces the same overlap/pitch-bend behavior instead of hard-cutting
## between chunks.
static func _build_automation(
        definition:MmdSoundSourceDefinition, sound_parameter:StringName, loop:bool = true) -> Dictionary:
    var chunks:Array[Dictionary] = definition.chunks.duplicate()
    chunks.sort_custom(func(a:Dictionary, b:Dictionary) -> bool: return int(a["threshold"]) < int(b["threshold"]))

    # fadeins[i]/fadeouts[i] mirror sound_source::deserialize()'s own cached chunk range points
    # (audio/sound.cpp:60-85); the last chunk ends at its Chunkrange or its threshold (sound.cpp:85)
    var fadeins:Array[float] = []
    var fadeouts:Array[float] = []
    for idx in range(chunks.size()):
        var threshold:float = float(chunks[idx]["threshold"])
        if idx == 0:
            fadeins.append(maxf(0.0, threshold))
        else:
            var previous_threshold:float = float(chunks[idx - 1]["threshold"])
            fadeins.append(threshold - 0.01 * definition.crossfade_percent * (threshold - previous_threshold))
        fadeouts.append(float(chunks[idx + 1]["threshold"]) if idx < chunks.size() - 1
                else maxf(float(definition.chunk_range), threshold))

    var automation := SfxAutomation.new()
    automation.parameter_name = sound_parameter
    # gnd-sfx's equal-power mode takes the square root of each fade curve. Storing the squared
    # original gain preserves sound_source::update_crossfade()'s logarithmic approximation and
    # avoids the linear mode's unity-sum normalization reducing the overlap.
    automation.crossfade_mode = SfxAutomation.CrossfadeMode.EQUAL_POWER
    var clips:Array[SfxClip] = []
    var tracks:Array[SfxTrack] = []

    for idx in range(chunks.size()):
        var threshold:float = float(chunks[idx]["threshold"])
        var is_last:bool = idx == chunks.size() - 1
        var fadein:float = fadeins[idx]

        var clip := SfxClip.new()
        clip.stream = build_stream(chunks[idx]["filename"], loop, definition.source_file.get_base_dir())
        clip.offset = fadein
        clip.length = maxf(0.0, fadeouts[idx] - fadein)

        var fade_in_width:float = threshold - fadein
        clip.fade_in_curve = _build_ramp_curve(true, fade_in_width)
        if not is_last:
            var fade_out_width:float = maxf(0.0, fadeouts[idx] - fadeins[idx + 1])
            clip.fade_out_curve = _build_ramp_curve(false, fade_out_width)

        var own_pitch:float = float(chunks[idx].get("pitch", 1.0))
        if own_pitch <= 0.0:
            own_pitch = 1.0
        var pitch_span:float = maxf(maxf(clip.length, fade_in_width), 0.001)
        clip.pitch_curve = _build_pitch_curve(chunks, idx, own_pitch, fadein, pitch_span)

        var track := SfxTrack.new()
        track.track_name = "chunk_%d" % int(threshold)
        clip.track = track
        tracks.append(track)

        clips.append(clip)

    automation.clips = clips
    automation.max_domain = fadeouts.back() if fadeouts else 0.0
    return {"automation": automation, "tracks": tracks}


## 0-width (or null-curve) case is deliberately left unset - SfxPlaybackRuntime treats a null
## fade_in_curve/fade_out_curve as flat gain 1.0 (see _sample_automation_curve()), correct for the
## first chunk's fade-in and the last chunk's fade-out, which have no neighbour to blend with.
static func _build_ramp_curve(ascending:bool, width:float) -> Curve:
    if width <= 0.0:
        return null
    var curve := Curve.new()
    curve.min_domain = 0.0
    curve.max_domain = width
    for point_index:int in range(_CROSSFADE_CURVE_SEGMENTS + 1):
        var progress:float = float(point_index) / float(_CROSSFADE_CURVE_SEGMENTS)
        var linear_gain:float = progress if ascending else 1.0 - progress
        var gain:float = linear_gain / (
                1.0 + (1.0 - linear_gain) * _CROSSFADE_LOG_FACTOR)
        curve.add_point(Vector2(progress * width, gain * gain))
        curve.set_point_left_mode(point_index, Curve.TANGENT_LINEAR)
        curve.set_point_right_mode(point_index, Curve.TANGENT_LINEAR)
    return curve


## Recreates update_crossfade()'s per-chunk pitch ratio in the clip's local parameter domain. Each
## sample plays at natural pitch at its own threshold and bends toward the neighbouring sample's
## declared pitch across the complete inter-threshold gap. The clip starts partway through the
## previous gap when crossfade is below 100%, so its first point is sampled at that exact position.
static func _build_pitch_curve(
        chunks:Array[Dictionary], idx:int, own_pitch:float, fadein:float, span:float) -> Curve:
    var ratio_from_prev:float = 1.0
    if idx > 0:
        var previous_pitch:float = float(chunks[idx - 1].get("pitch", 1.0))
        if previous_pitch <= 0.0:
            previous_pitch = 1.0
        var previous_threshold:float = float(chunks[idx - 1]["threshold"])
        var own_threshold:float = float(chunks[idx]["threshold"])
        var gap_progress:float = clampf(
                (fadein - previous_threshold) / (own_threshold - previous_threshold), 0.0, 1.0)
        ratio_from_prev = lerpf(previous_pitch / own_pitch, 1.0, gap_progress)
    var ratio_to_next:float = 1.0
    if idx < chunks.size() - 1:
        var next_pitch:float = float(chunks[idx + 1].get("pitch", 1.0))
        if next_pitch <= 0.0:
            next_pitch = 1.0
        ratio_to_next = next_pitch / own_pitch

    if is_equal_approx(ratio_from_prev, 1.0) and is_equal_approx(ratio_to_next, 1.0):
        return null

    var curve := Curve.new()
    curve.min_domain = 0.0
    curve.max_domain = span
    curve.min_value = minf(1.0, minf(ratio_from_prev, ratio_to_next))
    curve.max_value = maxf(1.0, maxf(ratio_from_prev, ratio_to_next))
    curve.add_point(Vector2(0.0, ratio_from_prev))
    var home_x:float = clampf(float(chunks[idx]["threshold"]) - fadein, 0.0, span)
    if home_x > 0.0:
        curve.add_point(Vector2(home_x, 1.0))
    if home_x < span:
        curve.add_point(Vector2(span, ratio_to_next))
    for point_index:int in range(curve.point_count):
        curve.set_point_left_mode(point_index, Curve.TANGENT_LINEAR)
        curve.set_point_right_mode(point_index, Curve.TANGENT_LINEAR)
    return curve


## Makes `events` one emitter, the original's sound_source: they start at `start_fraction` of their
## samples and share one pitch factor - the MMD's pitchvariation: as a share around 1, none for 0,
## or the original's default range when there is no definition or it gives none
## (sound.cpp:207-222, 374-377). Every event built from MaSzyna's data goes through here.
static func shape_emitter(
        events:Array[SfxEvent], definition:MmdSoundSourceDefinition, start_fraction:float) -> void:
    var pitch_variation:float = randf_range(DEFAULT_PITCH_VARIATION_MIN, DEFAULT_PITCH_VARIATION_MAX)
    if definition and not definition.pitch_variation == MmdSoundSourceDefinition.NO_PITCH_VARIATION:
        var half_range:float = definition.pitch_variation / 2.0
        pitch_variation = randf_range(1.0 - half_range, 1.0 + half_range)
    for event:SfxEvent in events:
        event.start_fraction = start_fraction
        event.pitch_variation = pitch_variation


## A sound file of the game, with its length known before its first playback - a voice's start in
## it (a start fraction, a seek) is placed before the file is read. Every MaSzyna sound stream is
## made here. `vehicle_dir` is the directory of the vehicle it belongs to, looked in first
## (audio.cpp:115), "" for a sound of no vehicle.
static func build_stream(filename:String, loop:bool, vehicle_dir:String) -> MaszynaAudioStream:
    var stream := MaszynaAudioStream.new()
    stream.file_path = filename
    stream.vehicle_dir = vehicle_dir
    stream.loop = loop
    stream.length = AudioStreamManager.get_stream_length(filename, vehicle_dir)
    return stream
