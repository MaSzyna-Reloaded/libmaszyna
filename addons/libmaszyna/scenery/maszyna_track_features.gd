@tool
extends Resource
class_name MaszynaTrackFeatures

## What a .scn track's textures say it is (TrackData's features): the first rule whose pattern its
## texture matches. The .scn knows only textures; a track no rule matches keeps TrackData's defaults.

## By a rail track's second texture
@export var bed_rules: Array[MaszynaTrackBedRule] = []
## By a road's first texture
@export var road_rules: Array[MaszynaRoadSurfaceRule] = []
