@tool
extends Resource
class_name MaszynaSignalBoxData
## A signal box of a world (OSM's railway=signal_box): its name, its code, its kind, where it is and the
## station it works for

@export var box_name:String = ""
## Its code (OSM's railway:ref, e.g. "Tz")
@export var ref:String = ""
## Its kind (OSM's railway:signal_box, e.g. "electronic", "relay", "mechanical")
@export var kind:String = ""
## Latitude and longitude, degrees
@export var lat_lon:Vector2 = Vector2.ZERO
## Its place in the world, on the ground
@export var position:Vector3 = Vector3.ZERO
## The ref of the station it works for (MaszynaStationData.ref): by its own code, else the nearest's
@export var station_ref:String = ""
