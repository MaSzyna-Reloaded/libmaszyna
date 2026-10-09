@tool
extends Resource
class_name MaszynaIsolatedData

## Scenery `isolated <name> <tracks...> endisolated` and `area <parent> <sections...> endarea`
## (deserialize_isolated(), deserialize_area(), simulationstateserializer.cpp:167-194) - an
## isolated track section with the tracks it adds, and the sections it is the parent of. Names are
## lower case.

@export var name:String = ""
@export var tracks:PackedStringArray = []
@export var children:PackedStringArray = []
