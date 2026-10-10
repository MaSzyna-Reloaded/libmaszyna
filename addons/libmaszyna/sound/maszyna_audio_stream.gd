@tool
extends AudioStream
class_name MaszynaAudioStream

@export var file_path:String = "":
    set(x):
        if not file_path == x:
            file_path = x
            _real_stream = null

## The directory of the vehicle the sound belongs to, where its file is looked for first
## (audio.cpp:115); "" for a sound of no vehicle
@export var vehicle_dir:String = "":
    set(x):
        if not vehicle_dir == x:
            vehicle_dir = x
            _real_stream = null

@export var loop:bool = false:
    set(x):
        if not loop == x:
            loop = x
            _real_stream = null

## The file's duration [s], set when the stream is built: gnd-sfx places a voice's start in the
## stream (a start fraction, a seek) before the first playback reads the file, and a length of 0
## put every copy of a looping sound at its very start
@export var length:float = 0.0

var _real_stream:AudioStream


## A resource has no tree to leave: the connection goes with the stream when it is freed
func _init() -> void:
    GameDataServer.data_unload_requested.connect(unload)


## Lets the sound file go; it is read again, from the game directory set then, when the stream is
## next played - the game's data read again, or a scenery sound out of reach (ScenerySoundServer)
func unload() -> void:
    _real_stream = null

func _get_stream_name() -> String:
    return file_path

func _get_length() -> float:
    return _real_stream.get_length() if _real_stream else length

func _instantiate_playback() -> AudioStreamPlayback:
    if file_path and not _real_stream:
        _real_stream = AudioStreamManager.get_stream(file_path, loop, vehicle_dir)

    if _real_stream:
        return _real_stream.instantiate_playback()
    else:
        return null
