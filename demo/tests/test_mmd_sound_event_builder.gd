extends MaszynaGutTest


func test_sound_main_only_produces_one_looping_clip():
    var definition := MmdSoundSourceDefinition.new()
    definition.sound_main = "oilpump"
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"oil_pump")
    assert_eq(event.name, &"oil_pump")
    assert_eq(event.clips.size(), 1)
    var stream:MaszynaAudioStream = event.clips[0].stream
    assert_eq(stream.file_path, "oilpump")
    assert_true(stream.loop)


func test_begin_main_end_produces_three_clips_with_sustain_release():
    var definition := MmdSoundSourceDefinition.new()
    definition.sound_begin = "horn1_start"
    definition.sound_main = "horn1_trwa"
    definition.sound_end = "horn1_stop"
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"horn1")
    assert_eq(event.clips.size(), 3)
    assert_eq((event.clips[0].stream as MaszynaAudioStream).file_path, "horn1_start")
    assert_false(event.clips[0].stream.loop)
    assert_eq((event.clips[1].stream as MaszynaAudioStream).file_path, "horn1_trwa")
    assert_true(event.clips[1].stream.loop)
    assert_eq((event.clips[2].stream as MaszynaAudioStream).file_path, "horn1_stop")
    assert_eq(event.clips[2].trigger_mode, SfxClip.TriggerMode.TRIGGER_SUSTAIN)


func test_begin_only_omits_main_and_end_clips():
    var definition := MmdSoundSourceDefinition.new()
    definition.sound_begin = "click"
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"click")
    assert_eq(event.clips.size(), 1)
    assert_eq((event.clips[0].stream as MaszynaAudioStream).file_path, "click")


func test_chunks_produce_one_automation_with_clips_positioned_by_threshold():
    var definition := MmdSoundSourceDefinition.new()
    definition.crossfade_percent = 90
    definition.chunks = [
        {"threshold": 1000, "filename": "idle_high", "pitch": 1000.0},
        {"threshold": 400, "filename": "silence1", "pitch": 400.0},
        {"threshold": 600, "filename": "idle_low", "pitch": 600.0},
    ]
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"engine", &"rpm")
    assert_eq(event.automations.size(), 1)
    var automation:SfxAutomation = event.automations[0]
    assert_eq(automation.parameter_name, &"rpm")
    assert_eq(automation.crossfade_mode, SfxAutomation.CrossfadeMode.EQUAL_POWER)
    assert_eq(automation.clips.size(), 3)

    # sorted by threshold regardless of input order (400, 600, 1000)
    var low:SfxClip = automation.clips[0]
    assert_eq((low.stream as MaszynaAudioStream).file_path, "silence1")
    assert_almost_eq(low.offset, 400.0, 0.01)
    assert_almost_eq(low.length, 200.0, 0.01) # fadeout(600) - fadein(400)
    assert_null(low.fade_in_curve) # first chunk: no previous chunk to blend in from
    assert_not_null(low.fade_out_curve)

    var mid:SfxClip = automation.clips[1]
    assert_eq((mid.stream as MaszynaAudioStream).file_path, "idle_low")
    assert_almost_eq(mid.offset, 420.0, 0.01) # 600 - 0.9*(600-400)
    assert_almost_eq(mid.length, 580.0, 0.01) # fadeout(1000) - fadein(420)
    assert_not_null(mid.fade_in_curve)
    assert_not_null(mid.fade_out_curve)

    var high:SfxClip = automation.clips[2]
    assert_eq((high.stream as MaszynaAudioStream).file_path, "idle_high")
    assert_almost_eq(high.offset, 640.0, 0.01) # 1000 - 0.9*(1000-600)
    # the top chunk ends at max(Chunkrange 100, its threshold) - above it the original plays none
    # (sound.cpp:85, 427-430)
    assert_almost_eq(high.length, 360.0, 0.01) # fadeout(1000) - fadein(640)
    assert_not_null(high.fade_in_curve)
    assert_null(high.fade_out_curve) # last chunk: no next chunk to blend out into

    # every chunk gets its own dedicated track so overlapping voices actually mix/crossfade
    # instead of competing on one shared master track.
    assert_eq(event.tracks.size(), 3)
    assert_true(low.track in event.tracks)
    assert_true(mid.track in event.tracks)
    assert_true(high.track in event.tracks)
    assert_ne(low.track, mid.track)

    # each chunk declared its own pitch, distinct from its neighbours, so every clip should bend
    # pitch across the crossfade zone rather than playing at a flat, unmodulated pitch.
    assert_not_null(low.pitch_curve)
    assert_not_null(mid.pitch_curve)
    assert_not_null(high.pitch_curve)

    # gnd-sfx square-roots equal-power curves at playback. The stored midpoint is therefore the
    # square of MaSzyna's logarithmic crossfade gain (~0.699), preserving the original result.
    assert_almost_eq(mid.fade_in_curve.sample(90.0), 0.489, 0.002)
    assert_almost_eq(mid.fade_out_curve.sample(180.0), 0.489, 0.002)

    # Each sample is natural at its own threshold and bends continuously towards neighbouring
    # sample pitches over the complete threshold gap.
    assert_almost_eq(low.pitch_curve.sample(0.0), 1.0, 0.001)
    assert_almost_eq(low.pitch_curve.sample(200.0), 1.5, 0.001)
    assert_almost_eq(mid.pitch_curve.sample(0.0), 0.7, 0.001)
    assert_almost_eq(mid.pitch_curve.sample(180.0), 1.0, 0.001)
    assert_almost_eq(mid.pitch_curve.sample(580.0), 1000.0 / 600.0, 0.001)
    assert_almost_eq(high.pitch_curve.sample(0.0), 0.64, 0.001)
    assert_almost_eq(high.pitch_curve.sample(360.0), 1.0, 0.001)

    assert_almost_eq(automation.max_domain, 1000.0, 0.01)


func test_engine_event_has_separate_physical_gain_modulation() -> void:
    var definition:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    definition.sound_main = "engine"
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"engine")

    var modulation:SfxParameterModulation = _find_modulation(event, &"engine_gain")
    assert_not_null(modulation)
    assert_eq(modulation.target, SfxParameterModulation.Target.GAIN)
    assert_almost_eq(modulation.min_domain, 0.0, 0.001)
    assert_almost_eq(modulation.max_domain, 2.0, 0.001)
    assert_almost_eq(modulation.curve.sample(1.25), 1.25, 0.001)


func test_soundproofed_event_has_gain_unit_size_and_spatial_configuration() -> void:
    var definition:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    definition.sound_main = "compressor"
    definition.offset = Vector3(1.0, 2.0, 3.0)
    definition.range = 160.0
    var event:SfxEvent = MmdSoundEventBuilder.build(
            definition, &"compressor", &"", false, true)

    var proofing_modulations:Array[SfxParameterModulation] = []
    for modulation:SfxParameterModulation in event.parameter_modulations:
        if modulation.parameter_name == &"soundproofing":
            proofing_modulations.append(modulation)
    assert_eq(proofing_modulations.size(), 2)
    assert_true(_has_modulation_target(
            proofing_modulations, SfxParameterModulation.Target.GAIN))
    assert_true(_has_modulation_target(
            proofing_modulations, SfxParameterModulation.Target.UNIT_SIZE))
    assert_eq(event.spatial_config.position, definition.offset)
    assert_almost_eq(event.spatial_config.unit_size, 10.0, 0.001)
    assert_almost_eq(event.spatial_config.max_distance, 1200.0, 0.001)


func test_bookends_and_chunks_combine_into_one_event_with_both_shapes():
    # mirrors engine.tres's own combined shape: a start (ignition) clip, a stop (shutdown) clip,
    # AND an rpm-driven crossfade automation all on the SAME event - not split into 3 separate
    # bank events, since MmdSoundBankInstancer merges ignition:/engine:/shutdown: into one
    # definition before calling build().
    var definition := MmdSoundSourceDefinition.new()
    definition.sound_begin = "engine_start"
    definition.sound_end = "engine_stop"
    definition.chunks = [{"threshold": 500, "filename": "idle", "pitch": 0.0}]
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"engine", &"rpm")

    assert_eq(event.clips.size(), 2)
    assert_eq((event.clips[0].stream as MaszynaAudioStream).file_path, "engine_start")
    assert_eq((event.clips[1].stream as MaszynaAudioStream).file_path, "engine_stop")
    assert_eq(event.clips[1].trigger_mode, SfxClip.TriggerMode.TRIGGER_SUSTAIN)

    assert_eq(event.automations.size(), 1)
    assert_eq(event.automations[0].clips.size(), 1)
    assert_eq(event.tracks.size(), 1)
    assert_eq(event.automations[0].clips[0].track, event.tracks[0])


func _find_modulation(event:SfxEvent, parameter_name:StringName) -> SfxParameterModulation:
    for modulation:SfxParameterModulation in event.parameter_modulations:
        if modulation.parameter_name == parameter_name:
            return modulation
    return null


func _has_modulation_target(modulations:Array[SfxParameterModulation], target:int) -> bool:
    for modulation:SfxParameterModulation in modulations:
        if modulation.target == target:
            return true
    return false


# sound.cpp:85 - a chunk table below its Chunkrange ends at the Chunkrange (100 by default)
func test_last_chunk_ends_at_the_chunk_range() -> void:
    var definition:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    definition.chunks = [{"threshold": 10, "filename": "low"}, {"threshold": 50, "filename": "high"}]
    var event:SfxEvent = MmdSoundEventBuilder.build(definition, &"chunked", &"point")
    var automation:SfxAutomation = event.automations[0]
    assert_almost_eq(automation.max_domain, float(MmdSoundSourceDefinition.DEFAULT_CHUNK_RANGE), 0.01)
    definition.chunk_range = 160
    automation = MmdSoundEventBuilder.build(definition, &"chunked", &"point").automations[0]
    assert_almost_eq(automation.max_domain, 160.0, 0.01, "outernoise: the vehicle's Vmax (DynObj.cpp:6389)")
