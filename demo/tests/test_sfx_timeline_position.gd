extends GutTest

## An event started at a position on its timeline - a running engine heard for the first time
## starts past its ignition - plays only the clips whose span reaches that position, as FMOD's
## setTimelinePosition does. The span is the clip's own length, known before its stream is loaded.

const SAMPLE_RATE:int = 1000
const BEGIN_LENGTH:float = 1.0
const VOICE_SLOTS:int = 4
const TICK:float = 0.016


## A stream that does not know its length until it is loaded, as MaszynaAudioStream
class UnloadedStream extends AudioStream:
    var sample:AudioStreamWAV

    func _get_length() -> float:
        return 0.0

    func _instantiate_playback() -> AudioStreamPlayback:
        return sample.instantiate_playback()


func _clip(offset:float) -> SfxClip:
    var sample:AudioStreamWAV = AudioStreamWAV.new()
    sample.format = AudioStreamWAV.FORMAT_16_BITS
    sample.mix_rate = SAMPLE_RATE
    var data:PackedByteArray = PackedByteArray()
    data.resize(int(BEGIN_LENGTH * SAMPLE_RATE) * 2)
    sample.data = data
    var stream:UnloadedStream = UnloadedStream.new()
    stream.sample = sample
    var clip:SfxClip = SfxClip.new()
    clip.stream = stream
    clip.offset = offset
    clip.length = BEGIN_LENGTH
    return clip


func _play(begin_clip:SfxClip, main_clip:SfxClip, position:float) -> SfxPlaybackRuntime:
    var event:SfxEvent = SfxEvent.new()
    event.name = &"engine"
    var clips:Array[SfxClip] = [begin_clip, main_clip]
    event.clips = clips
    var runtime:SfxPlaybackRuntime = SfxPlaybackRuntime.new()
    runtime.set_slot_capacity(VOICE_SLOTS)
    runtime.play(event, position)
    # the ticks after the start look at the timeline again
    runtime.update(TICK)
    return runtime


func _active(runtime:SfxPlaybackRuntime, clip:SfxClip) -> bool:
    return runtime.get_event_visualization_state(&"engine")["clips"][clip]["active"]


func test_started_past_a_clip_does_not_play_it() -> void:
    var begin_clip:SfxClip = _clip(0.0)
    var main_clip:SfxClip = _clip(BEGIN_LENGTH)
    var runtime:SfxPlaybackRuntime = _play(begin_clip, main_clip, BEGIN_LENGTH)

    assert_false(_active(runtime, begin_clip), "the opening clip is over at this position")
    assert_true(_active(runtime, main_clip), "the clip starting at this position plays")


func test_started_at_zero_plays_the_opening_clip() -> void:
    var begin_clip:SfxClip = _clip(0.0)
    var main_clip:SfxClip = _clip(BEGIN_LENGTH)
    var runtime:SfxPlaybackRuntime = _play(begin_clip, main_clip, 0.0)

    assert_true(_active(runtime, begin_clip))
    assert_false(_active(runtime, main_clip), "not yet at its position")
