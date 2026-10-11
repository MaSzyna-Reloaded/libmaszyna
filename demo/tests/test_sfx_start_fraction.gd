extends GutTest

## Every vehicle of a trainset plays the same running noise, and it is an automation crossfaded
## between speed chunks. Each vehicle's shift has to reach every chunk it swaps in, or the copies
## play in step and ring metallic (DynObj.cpp:6511, audiorenderer.cpp:99).

const SAMPLE_RATE:int = 1000
const CHUNK_SPEED:float = 50.0
const MAX_SPEED:float = 100.0
const START_FRACTION:float = 0.5
const TICK:float = 0.016
const VOICE_SLOTS:int = 4
## a speed inside the first chunk and one inside the second
const FIRST_CHUNK_SPEED:float = 10.0
const SECOND_CHUNK_SPEED:float = 70.0
## A looping sample in the fixtures game directory (demo/tests/fixtures/sounds)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const FIXTURE_LOOP:String = "TEST_LOOP"
## An emitter's own pitch factor, inside the original's 97.5-102.5 % (sound.cpp:374-377)
const PITCH_VARIATION:float = 1.02


func _chunk(offset:float) -> SfxClip:
    var stream:AudioStreamWAV = AudioStreamWAV.new()
    stream.format = AudioStreamWAV.FORMAT_16_BITS
    stream.mix_rate = SAMPLE_RATE
    var data:PackedByteArray = PackedByteArray()
    data.resize(SAMPLE_RATE * 2)
    stream.data = data
    stream.loop_mode = AudioStreamWAV.LOOP_FORWARD
    stream.loop_end = SAMPLE_RATE
    var clip:SfxClip = SfxClip.new()
    clip.stream = stream
    clip.offset = offset
    clip.length = CHUNK_SPEED
    return clip


func _play(start_fraction:float, speed:float) -> SfxPlaybackRuntime:
    var automation:SfxAutomation = SfxAutomation.new()
    automation.parameter_name = &"speed"
    automation.max_domain = MAX_SPEED
    var clips:Array[SfxClip] = [_chunk(0.0), _chunk(CHUNK_SPEED)]
    automation.clips = clips
    var event:SfxEvent = SfxEvent.new()
    event.name = &"outer_noise"
    var automations:Array[SfxAutomation] = [automation]
    event.automations = automations
    event.start_fraction = start_fraction
    var runtime:SfxPlaybackRuntime = SfxPlaybackRuntime.new()
    runtime.set_slot_capacity(VOICE_SLOTS)
    runtime.play(event, 0.0, {&"speed": speed})
    runtime.update(TICK)
    return runtime


func _start_position(runtime:SfxPlaybackRuntime) -> float:
    var slot:SfxVoiceSlot = runtime.get_slots()[0]
    return slot.start_position / slot.stream.get_length()


func test_automation_voice_starts_at_the_start_fraction() -> void:
    assert_almost_eq(_start_position(_play(START_FRACTION, FIRST_CHUNK_SPEED)), START_FRACTION, 0.001)


func test_chunk_swapped_in_keeps_the_start_fraction() -> void:
    var runtime:SfxPlaybackRuntime = _play(START_FRACTION, FIRST_CHUNK_SPEED)
    runtime.modulate(&"outer_noise", {&"speed": SECOND_CHUNK_SPEED})
    runtime.update(TICK)
    assert_almost_eq(_start_position(runtime), START_FRACTION, 0.001)


func test_automation_voice_without_fraction_starts_at_the_stream_offset() -> void:
    assert_almost_eq(_start_position(_play(0.0, FIRST_CHUNK_SPEED)), 0.0, 0.001)


## A single-sample running noise (outernoise: { soundmain: ... }) is one plain looping clip, not an
## automation - it has to take the shift as well
func test_plain_looping_clip_starts_at_the_start_fraction() -> void:
    assert_almost_eq(_start_position(_play_clip(_chunk(0.0))), START_FRACTION, 0.001)


func test_plain_looping_clip_keeps_playing_past_its_length() -> void:
    var runtime:SfxPlaybackRuntime = _play_clip(_chunk(0.0))
    var stream_length:float = runtime.get_slots()[0].stream.get_length()
    var elapsed:float = 0.0
    while elapsed < 2.0 * stream_length:
        runtime.update(TICK)
        elapsed += TICK
    assert_true(runtime.is_playing(&"outer_noise"))


## A vehicle's sound is a MaszynaAudioStream that reads its file on the first playback - after
## gnd-sfx has already placed the voice's start in it; the length known at build is what puts the
## start there at all (FINDINGS.md, 2026-10-01)
func test_maszyna_stream_starts_at_the_start_fraction_before_its_first_playback() -> void:
    var previous:String = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    var clip:SfxClip = SfxClip.new()
    clip.stream = MmdSoundEventBuilder.build_stream(FIXTURE_LOOP, true, "")
    var runtime:SfxPlaybackRuntime = _play_clip(clip)
    UserSettings.save_maszyna_game_dir(previous)
    assert_gt(clip.stream.get_length(), 0.0)
    assert_almost_eq(_start_position(runtime), START_FRACTION, 0.001)


func test_pitch_variation_scales_every_voice() -> void:
    var runtime:SfxPlaybackRuntime = SfxPlaybackRuntime.new()
    runtime.set_slot_capacity(VOICE_SLOTS)
    var clip:SfxClip = _chunk(0.0)
    var event:SfxEvent = SfxEvent.new()
    event.name = &"outer_noise"
    var clips:Array[SfxClip] = [clip]
    event.clips = clips
    event.pitch_variation = PITCH_VARIATION
    runtime.play(event, 0.0, {})
    runtime.update(TICK)
    assert_almost_eq(runtime.get_slots()[0].pitch_scale, PITCH_VARIATION, 0.001)


## A one-shot is a single sample too, and takes the shift (audiorenderer_extra.h)
func test_one_shot_starts_at_the_start_fraction() -> void:
    var clip:SfxClip = _chunk(0.0)
    (clip.stream as AudioStreamWAV).loop_mode = AudioStreamWAV.LOOP_DISABLED
    assert_almost_eq(_start_position(_play_clip(clip)), START_FRACTION, 0.001)


## An opening bookend plays from its own start (audiorenderer_extra.h, is_bookend())
func test_bookend_is_not_shifted() -> void:
    var clip:SfxClip = _chunk(0.0)
    clip.bookend = true
    assert_almost_eq(_start_position(_play_clip(clip)), 0.0, 0.001)


func _play_clip(clip:SfxClip) -> SfxPlaybackRuntime:
    var event:SfxEvent = SfxEvent.new()
    event.name = &"outer_noise"
    var clips:Array[SfxClip] = [clip]
    event.clips = clips
    event.start_fraction = START_FRACTION
    var runtime:SfxPlaybackRuntime = SfxPlaybackRuntime.new()
    runtime.set_slot_capacity(VOICE_SLOTS)
    runtime.play(event, 0.0, {})
    runtime.update(TICK)
    return runtime
