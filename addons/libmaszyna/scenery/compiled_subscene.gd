@tool
extends MaszynaCompiledScenery
class_name MaszynaCompiledSubscene

## The subscene's triangles, one MaszynaTrianglesChunkGeometry file per chunk beside the cache entry
## (MaszynaTrianglesSink) - added to the scenery's sink wherever the subscene is included
@export var triangle_chunk_paths:PackedStringArray = []
## The joinduplicatedevents the subscene leaves for what follows its include
## (MaszynaImporterContext.join_duplicated_events)
@export var join_duplicated_events:bool = false
