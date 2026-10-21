@tool
extends Resource
class_name MaszynaRoadSurfaceRule

## A road's surface and centre line, told by its first texture's name (MaszynaTrackFeatures)

## A pattern of the texture's name (String.match(): * and ?)
@export var material: String = ""
@export var surface: TrackData.Surface = TrackData.SURFACE_ASPHALT
@export var centre_line: bool = false
