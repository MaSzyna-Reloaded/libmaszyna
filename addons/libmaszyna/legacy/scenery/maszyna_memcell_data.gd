@tool
extends Resource
class_name MaszynaMemcellData

## Scenery `node <rmax> <rmin> <name> memcell <x> <y> <z> <text> <value1> <value2> <track|none>
## endmemcell` (TMemCell::Load(), MemCell.cpp:101-122) - one memory of ScenarioEventServer

@export var name:String = ""
@export var position:Vector3 = Vector3.ZERO
## Case sensitive, as the original reads it (MemCell.cpp:109)
@export var text:String = ""
@export var value1:float = 0.0
@export var value2:float = 0.0
## Lower case name of the track the memory stands at, empty for none
@export var track:String = ""
