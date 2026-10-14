@tool
extends Node
class_name MaszynaModelsNode

## The models of a world's sector (MaszynaWorldSector) - the game's own (E3DRenderingServer) - as
## data in the sector's own coordinates. MaszynaWorld registers them as it composes its sectors,
## the way a .scn's models are (SceneryInstancer.instantiate_compiled()).

@export var models:Array[MaszynaModelData] = []
