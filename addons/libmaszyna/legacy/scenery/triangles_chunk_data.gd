@tool
extends Resource
class_name MaszynaTrianglesChunkData

## One merged mesh of scenery "triangles" nodes sharing a texture, a 1 km cell and a visibility
## range (SceneryTrianglesSink). The material is resolved through
## MaterialManager when the chunk is built, never stored here, so a cached scenery still follows
## season/weather material variants (material_manager::on_season_change, material.cpp:571).
## The triangles themselves are a MaszynaTrianglesChunkGeometry of their own, loaded only while the
## chunk is built.

## The resource path of the geometry (MaszynaTrianglesChunkGeometry), unique to the scenery and the
## chunk
@export var geometry_path:String = ""
@export var position:Vector3 = Vector3.ZERO
@export var material_name:String = ""
@export var range_min:float = 0.0
## Visible up to this distance, 0 - no limit of its own (capped by the streaming draw distance)
@export var range_max:float = 0.0
