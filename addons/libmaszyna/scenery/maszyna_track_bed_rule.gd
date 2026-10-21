@tool
extends Resource
class_name MaszynaTrackBedRule

## A rail track's bed, told by its second texture's name (MaszynaTrackFeatures)

## A pattern of the texture's name (String.match(): * and ?)
@export var material: String = ""
@export var bed: TrackData.Bed = TrackData.BED_CRUSHED_STONE
