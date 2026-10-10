@tool
extends Resource
class_name MaszynaSoundData

## Scenery `node <rmax> <rmin> <name> sound <x> <y> <z> <file> endsound` (deserialize_sound(),
## simulationstateserializer.cpp:1106-1123) - a sound a scenery `sound` event plays

@export var name:String = ""
@export var position:Vector3 = Vector3.ZERO
## The file in the game's `sounds/`, lower case, without its extension
@export var file:String = ""
## How far it is heard (the node's rmax, sound_source's range)
@export var range_max:float = 0.0
