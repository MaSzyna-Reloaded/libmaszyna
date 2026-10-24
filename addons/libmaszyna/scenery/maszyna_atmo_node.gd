@tool
extends Node
class_name MaszynaAtmoNode

## Marker produced by a scenery's "atmo" section (deserialize_atmo(),
## simulationstateserializer.cpp:198); MaszynaSceneryNode applies the values after a load.

## Fog range in metres; the original picks a random visibility between the two
@export var fog_range_start:float = 0.0
@export var fog_range_end:float = 0.0
## The overcast parameter is optional
@export var overcast_defined:bool = false
## 0-1 cloud cover, above 1 precipitation; negative - random up to the absolute value
@export var overcast:float = 0.0
