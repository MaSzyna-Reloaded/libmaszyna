@tool
extends Node
class_name MaszynaWorldSector

## A sector of a world (MaszynaWorld): one square of the grid, a scene of its own named by its id
## (sectors/<id>.scn). Its data - MaszynaTracksNode, MaszynaTractionNode children - is in the
## sector's own coordinates: metres from its corner, X west (-(E - E corner)), Z north
## (N - N corner), heights as they are; the world moves it to its place as it composes its sectors.

## The square's id on the grid: <projection>-E<column>-N<row>, the column and the row its east and
## north corner metres divided by MaszynaWorld.SECTOR_SIZE (e.g. PL92-E0437-N0700)
@export var id:String = ""


## The column (x) and the row (y) of the square, as its id gives them
func get_cell() -> Vector2i:
    return cell_of(id)


## The column (x) and the row (y) of the square `sector_id` names
static func cell_of(sector_id:String) -> Vector2i:
    # the last two parts: the projection's name is the rest
    var parts:PackedStringArray = sector_id.rsplit("-", true, 2)
    return Vector2i(parts[1].trim_prefix("E").to_int(), parts[2].trim_prefix("N").to_int())
