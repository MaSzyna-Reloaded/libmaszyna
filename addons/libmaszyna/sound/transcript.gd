@tool
extends Resource
class_name Transcript

## What is said in a sound, line by line, shown on screen while it plays (TTranscript,
## uitranscripts.h:10). Each line is shown and hidden at its own time [s] from the sound's start.

@export var texts:PackedStringArray = []
@export var shows:PackedFloat64Array = []
@export var hides:PackedFloat64Array = []


func add_line(text:String, show:float, hide:float) -> void:
    texts.append(text)
    shows.append(show)
    hides.append(hide)
