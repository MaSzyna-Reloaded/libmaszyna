@tool
extends ScenarioEventAction
class_name MaszynaSoundAction

## The original's `sound` event (sound_event, Event.cpp:1394-1429): plays, loops or stops its
## scenery sounds. Played with a radio channel it is a radio message instead, transmitted from the
## sound's place on the scenery's radio (simulation::radio_message(), SceneryVirtualRadio).

enum Mode {
    ## 0
    STOP,
    ## 1
    PLAY,
    ## -1
    LOOP,
}

## The scenery's sounds, and the names of those it plays
var scenery_sounds:MaszynaScenerySounds = null
var targets:PackedStringArray = []
@export var mode:Mode = Mode.PLAY
## The radio channel the sound is a message on, 0 for none
@export var radio_channel:int = 0
## The scenery's radio a message is transmitted on
var radio:SceneryVirtualRadio = null


func _run(_event:RID, _activator:RID) -> void:
    for target:String in targets:
        var sound:RID = scenery_sounds.get_sound(target)
        if mode == Mode.PLAY and radio_channel > 0:
            radio.transmit(ScenerySoundServer.sound_get_position(sound), radio_channel, scenery_sounds.get_reach(target),
                    RadioServer.KIND_MESSAGE, {
                        "sound": ScenerySoundServer.sound_get_play_event(sound),
                        "transcript": scenery_sounds.get_transcript(target),
                    })
            continue
        if mode == Mode.STOP:
            ScenerySoundServer.sound_stop(sound)
            continue
        # a sound already playing goes on as it is, and shows no transcript again
        var started:bool = ScenerySoundServer.sound_play(
                sound, ScenerySoundServer.Playback.ONCE if mode == Mode.PLAY else ScenerySoundServer.Playback.LOOP)
        var transcript:Transcript = scenery_sounds.get_transcript(target)
        if started and transcript:
            TranscriptSystem.add(transcript)
