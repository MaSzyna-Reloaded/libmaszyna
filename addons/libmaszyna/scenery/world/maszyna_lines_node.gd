@tool
extends Node
class_name MaszynaLinesNode

## The lines of a world's sector (MaszynaWorldSector) - the wires of its power lines - as data in
## the sector's own coordinates. MaszynaWorld draws them as it composes its sectors, the way a
## .scn's "lines" nodes are drawn (SceneryInstancer.instantiate_compiled(), SceneryLines).

@export var lines:Array[MaszynaLinesData] = []
