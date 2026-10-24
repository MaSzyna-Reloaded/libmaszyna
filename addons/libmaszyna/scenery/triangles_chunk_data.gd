@tool
extends Resource
class_name MaszynaTrianglesChunkData

## One chunk of scenery "triangles" nodes: a texture in a 1 km section, or a texture and a visibility
## range in a 250 m cell, its shapes merged within 25 m its groups (MaszynaTrianglesSink,
## MaszynaTrianglesChunkGeometry). The material is resolved through
## MaszynaMaterialManager when the chunk is built, never stored here, so a cached scenery still follows
## season/weather material variants (material_manager::on_season_change, material.cpp:571).
## The triangles themselves are a MaszynaTrianglesChunkGeometry in the file of their section
## (MaszynaTrianglesSector), loaded only while one of its chunks is built.

## The resource path of the section's file (MaszynaTrianglesSector), shared by its chunks
@export var geometry_path:String = ""
## The chunk's index in its section's file
@export var chunk_index:int = 0
@export var position:Vector3 = Vector3.ZERO
@export var material_name:String = ""
@export var range_min:float = 0.0
## Visible up to this distance, 0 - no limit of its own (capped by the streaming draw distance)
@export var range_max:float = 0.0
