@tool
extends Node
class_name MaszynaTractionNode

## The catenary of a world's sector (MaszynaWorldSector) and its power sources, as data in the
## sector's own coordinates. MaszynaWorld registers them with TractionRenderingServer and
## TractionServer as it composes its sectors (SceneryInstancer.instantiate_compiled()).

@export var traction:Array[MaszynaTractionData] = []
@export var power_sources:Array[MaszynaPowerSourceData] = []
