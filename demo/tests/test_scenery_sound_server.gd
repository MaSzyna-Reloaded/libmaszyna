extends MaszynaGutTest

## A second of silence, enough for an event to be playing when asked
const MIX_RATE:int = 22050


func test_a_sound_heard_everywhere_loops_until_stopped() -> void:
    var sound:RID = _create_sound(0.0)

    ScenerySoundServer.sound_play(sound, ScenerySoundServer.Playback.LOOP)
    assert_true(ScenerySoundServer.sound_is_playing(sound))
    ScenerySoundServer.sound_stop(sound)
    assert_false(ScenerySoundServer.sound_is_playing(sound))
    ScenerySoundServer.sound_free(sound)


## sound.cpp:350-421 - played once while it loops, the sound goes on as it is (exclusive) and is not
## started a second time
func test_a_loop_played_once_goes_on_without_a_second_voice() -> void:
    var sound:RID = _create_sound(0.0)
    assert_true(ScenerySoundServer.sound_play(sound, ScenerySoundServer.Playback.LOOP))

    assert_false(ScenerySoundServer.sound_play(sound, ScenerySoundServer.Playback.ONCE))
    assert_true(ScenerySoundServer.sound_is_playing(sound))
    ScenerySoundServer.sound_free(sound)


func test_a_sound_keeps_its_place_and_its_event_until_freed() -> void:
    var position:Vector3 = Vector3(10.0, 0.0, -20.0)
    var sound:RID = _create_sound(0.0, position)

    assert_eq(ScenerySoundServer.sound_get_position(sound), position)
    var event:SfxEvent = ScenerySoundServer.sound_get_play_event(sound)
    assert_not_null(event)
    assert_eq(event.spatial_config.position, position, "the voice stands at the sound's place")
    ScenerySoundServer.sound_free(sound)
    assert_null(ScenerySoundServer.sound_get_play_event(sound))


func _create_sound(reach:float, position:Vector3 = Vector3.ZERO) -> RID:
    var config:SfxSpatialConfig = SfxSpatialConfig.new()
    config.position = position
    return ScenerySoundServer.sound_create(_event(config, false), _event(config, true), position, reach)


func _event(config:SfxSpatialConfig, loop:bool) -> SfxEvent:
    var stream:AudioStreamWAV = AudioStreamWAV.new()
    stream.mix_rate = MIX_RATE
    stream.data = PackedByteArray()
    stream.data.resize(MIX_RATE)
    stream.loop_mode = AudioStreamWAV.LOOP_FORWARD if loop else AudioStreamWAV.LOOP_DISABLED
    stream.loop_end = MIX_RATE
    var clip:SfxClip = SfxClip.new()
    clip.stream = stream
    var event:SfxEvent = SfxEvent.new()
    event.spatial_config = config
    var clips:Array[SfxClip] = [clip]
    event.clips = clips
    return event
