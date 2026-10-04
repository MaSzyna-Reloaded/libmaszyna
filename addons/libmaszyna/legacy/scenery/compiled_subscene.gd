@tool
extends MaszynaCompiledScenery
class_name MaszynaCompiledSubscene

## The subscene's triangles, one MaszynaTrianglesChunkGeometry file per chunk beside the cache entry
## (SceneryTrianglesSink) - added to the scenery's sink wherever the subscene is included
@export var triangle_chunk_paths:PackedStringArray = []
