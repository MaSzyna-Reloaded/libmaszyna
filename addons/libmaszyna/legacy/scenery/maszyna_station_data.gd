@tool
extends Resource
class_name MaszynaStationData
## A station or a halt of a world (OSM's railway=station|halt), for a list of the world's stations: its
## name, the codes it is known by and where it is

@export var station_name:String = ""
## Its code in the signalling and the timetables (OSM's railway:ref, e.g. "Tz")
@export var ref:String = ""
## Its UIC code (OSM's uic_ref, e.g. "5100072")
@export var uic_ref:String = ""
## Latitude and longitude, degrees
@export var lat_lon:Vector2 = Vector2.ZERO
## Its place in the world, on the ground
@export var position:Vector3 = Vector3.ZERO
