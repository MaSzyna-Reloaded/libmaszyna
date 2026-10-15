@tool
extends Resource
class_name MaszynaLinesData

## Scenery "node lines", "line_strip" or "line_loop" (lines_node::import(), scenenode.cpp:561-649) -
## the wires of power lines, mostly. Drawn by SceneryLines, merged with its neighbours as the
## original merges them (MaszynaLinesGeometry).

## The lines in the world, pairs of points - a strip or a loop turned into pairs as it was read
@export var points:PackedVector3Array = []
## The node's r g b, divided by 255 (scenenode.cpp:572-578)
@export var color:Color = Color.BLACK
## The original's line width, at most 30 (scenenode.cpp:579-582); a negative one is a line
## that does not fade with the distance (opengl33renderer.cpp:4143)
@export var width:float = 0.0
@export var range_min:float = 0.0
## Visible up to this distance from the node's centre, 0 - no limit
@export var range_max:float = 0.0


## A copy of the lines moved by `offset`
func translated(offset:Vector3) -> MaszynaLinesData:
    var moved:MaszynaLinesData = duplicate() as MaszynaLinesData
    var moved_points:PackedVector3Array = points.duplicate()
    for index:int in moved_points.size():
        moved_points[index] += offset
    moved.points = moved_points
    return moved
