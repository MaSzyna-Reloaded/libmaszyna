@tool
extends Node
class_name MaszynaTracksNode

## The tracks of one kind of a world's sector (MaszynaWorldSector) - railways, or roads - as data, in
## the sector's own coordinates. Not a node per track: MaszynaWorld registers them with TrackServer
## and TrackRenderingServer as it composes its sectors, the way a .scn's tracks are
## (SceneryInstancer.instantiate_compiled()).

## What the tracks are (TrackServer.track_get_group() of each)
@export var type:TrackServer.TrackTypeGroup = TrackServer.GROUP_RAIL
@export var tracks:Array[MaszynaTrackData] = []
