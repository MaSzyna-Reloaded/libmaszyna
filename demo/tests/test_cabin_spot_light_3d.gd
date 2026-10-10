extends MaszynaGutTest

const SOUND_ON:StringName = &"click_on"
const SOUND_OFF:StringName = &"click_off"
## One second of silence per click
const SAMPLE_RATE:int = 44100


## A cab bank holding the lamp's two clicks - silent in-memory WAVs (not MaszynaAudioStream), so
## the real play() path runs with no file dependency.
func _build_sound_player() -> SfxPlayer3D:
    var events:Array[SfxEvent] = []
    for event_name:StringName in [SOUND_ON, SOUND_OFF]:
        var stream := AudioStreamWAV.new()
        stream.format = AudioStreamWAV.FORMAT_16_BITS
        stream.mix_rate = SAMPLE_RATE
        var data := PackedByteArray()
        data.resize(SAMPLE_RATE * 2)
        stream.data = data
        var clip := SfxClip.new()
        clip.stream = stream
        var clips:Array[SfxClip] = [clip]
        var event := SfxEvent.new()
        event.name = event_name
        event.clips = clips
        events.append(event)
    var bank := SfxBank.new()
    bank.events = events
    var sound_player := SfxPlayer3D.new()
    sound_player.bank = bank
    add_child_autofree(sound_player)
    return sound_player


func _build_widget(sound_player:SfxPlayer3D) -> CabinSpotLight3D:
    var widget := CabinSpotLight3D.new()
    widget.sound_player = sound_player
    widget.sound_on_event = SOUND_ON
    widget.sound_off_event = SOUND_OFF
    return widget


## sound_on_event/sound_off_event's play-on-transition must fire on every FLASH (matches SM42's own
## hand-authored reference exactly - cabin_blinker.gd's `blink` signal plays a click on every
## blink cycle while the alerter stays active, not once per alert session), not just on the
## overall enabled=false->true/true->false transition - confirmed real regression: binding the
## sound trigger to the static "enabled" flag instead of the per-flash "active_now" value meant
## the click only fired once when the alert started, never again while it kept blinking.
func test_sound_plays_on_every_blink_transition_not_just_session_start():
    var sound_player:SfxPlayer3D = _build_sound_player()
    var widget:CabinSpotLight3D = _build_widget(sound_player)
    widget.blink_time = 999.0 # long enough that the timer itself never fires during this test -
    # blink transitions below are all driven manually via _on_blink_timeout()
    add_child_autofree(widget)
    widget.enabled = true

    widget._update_state() # session start: active_now flips false -> true
    assert_true(sound_player.is_playing(SOUND_ON))

    widget._on_blink_timeout() # next flash: active_now flips true -> false
    assert_true(sound_player.is_playing(SOUND_OFF), "should click again on the very next flash, not stay silent for the rest of the session")

    sound_player.stop(true)
    widget._on_blink_timeout() # flips back false -> true
    assert_true(sound_player.is_playing(SOUND_ON))


func test_sound_does_not_replay_when_state_is_unchanged():
    var sound_player:SfxPlayer3D = _build_sound_player()
    var widget:CabinSpotLight3D = _build_widget(sound_player)
    add_child_autofree(widget)
    widget.enabled = true

    widget._update_state()
    sound_player.stop(true) # silence so a spurious replay would be observable
    widget._update_state() # no state change since the previous call

    assert_false(sound_player.is_playing(SOUND_ON))


## light_enabled=false is for indicator labels with no real per-vehicle lamp data (e.g. i-radio,
## i-security_cabsignal) - the light must stay dark even while active, but the on/off submodel
## pair and the click sound (both driven from the same active_now, not the light) must still work.
func test_light_enabled_false_suppresses_light_but_not_submodel_or_sound():
    var sound_player:SfxPlayer3D = _build_sound_player()
    var widget:CabinSpotLight3D = _build_widget(sound_player)
    widget.light_enabled = false
    widget.light_energy_on = 1.0
    add_child_autofree(widget)

    var on_target := Node3D.new()
    var off_target := Node3D.new()
    add_child_autofree(on_target)
    add_child_autofree(off_target)
    widget.on_target_path = widget.get_path_to(on_target)
    widget.off_target_path = widget.get_path_to(off_target)
    widget._on_target = on_target
    widget._off_target = off_target

    widget.enabled = true

    widget._update_state()

    assert_eq(widget._target_light_energy, 0.0, "light itself must stay dark with no real lamp data")
    assert_true(on_target.visible, "submodel toggling is independent of the light itself")
    assert_false(off_target.visible)
    assert_true(sound_player.is_playing(SOUND_ON), "click sound is independent of the light itself")
