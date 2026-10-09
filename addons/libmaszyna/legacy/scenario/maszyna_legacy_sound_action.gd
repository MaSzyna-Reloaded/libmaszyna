@tool
extends ScenarioEventAction
class_name MaszynaLegacySoundAction

## The original's `sound` event (sound_event, Event.cpp:1394-1429): plays, loops or stops its
## scenery sounds. Played with a radio channel it is a radio message instead, heard only on the
## player's cab radio (simulation::radio_message(), CabinSystem.send_radio_message()).

enum Mode {
    ## 0
    STOP,
    ## 1
    PLAY,
    ## -1
    LOOP,
}

## The scenery's sounds, and the names of those it plays
var scenery_sounds:MaszynaLegacyScenerySounds = null
var targets:PackedStringArray = []
@export var mode:Mode = Mode.PLAY
## The radio channel the sound is a message on, 0 for none
@export var radio_channel:int = 0


func _run(_event:RID, _activator:RID) -> void:
    for target:String in targets:
        var sound:RID = scenery_sounds.get_sound(target)
        if mode == Mode.PLAY and radio_channel > 0:
            CabinSystem.send_radio_message(
                    ScenerySoundServer.sound_get_play_event(sound), scenery_sounds.get_transcript(target),
                    radio_channel, ScenerySoundServer.sound_get_position(sound), scenery_sounds.get_reach(target))
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
