@tool
extends Node
class_name MaszynaMeshesNode

## Geometry of a world's sector (MaszynaWorldSector) - its buildings, platforms or bridges - as
## meshes in the sector's own coordinates. A surface is named after the game's material it is drawn
## with (MaterialManager), or carries a material of its own; MaszynaWorld gives the named ones that
## material and places the mesh with
## SceneryMeshRenderingServer, which draws it while the camera is within `range_end`.

@export var meshes:Array[ArrayMesh] = []
## How far the meshes are drawn from [m]; 0: the draw distance
@export var range_end:float = 0.0
