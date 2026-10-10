extends RefCounted
class_name MmdPythonScreenDescriptor

## One `pyscreen:` of an MMD - a cab screen drawn by a Python 2 script (TTrain::screen_entry,
## Train.h:129).

## Absolute path of the script, without ".py"
var script_path:String = ""
## Submodel of the cab model whose texture the screen replaces; "none" draws nowhere
var target:String = ""
## Interval the screen is redrawn at, in milliseconds; -1 draws it once. As parsed it is the
## screen's own `updatetime:` (0 when it declares none) - MmdCabinInstancer.parse() resolves it.
var update_time_msec:int = 0
## The screen's `parameters:`, all strings, under lowercase keys (dictionary_source(), dictionary.cpp:18)
var parameters:Dictionary = {}
