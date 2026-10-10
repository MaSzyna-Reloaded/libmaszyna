@tool
extends Resource
class_name MaszynaTrainsetData

## Scenery `trainset <name> <track> <offset> <velocity> ... endtrainset` (deserialize_trainset(),
## simulationstateserializer.cpp) - its vehicles stand on the track one after another from the
## offset and are coupled (RailVehicleServer.trainset_place()). A `dynamic` outside a `trainset` is
## a trainset of one, standing by its own track and offset.

## The track its front stands on, and the distance along it [m]
@export var track_name:String = ""
@export var offset:float = 0.0
## Its name; empty for a `dynamic` outside a `trainset`
@export var name:String = ""
## The timetable of its driver (the original takes the trainset's name for it: a file in the
## scenery's directory, `none` for none) and the velocity it starts with - the driver's first
## orders (deserialize_endtrainset(), simulationstateserializer.cpp:839-848). Empty for a
## `dynamic` outside a `trainset`.
@export var timetable:String = ""
@export var velocity:float = 0.0
## Its vehicles, front first
@export var dynamics:Array[MaszynaDynamicData] = []


## Its vehicles as `arranged` lists them (MaszynaIncludeNode.trainset_override) when the first
## named vehicle there is one of its own, its declared ones otherwise. A named entry is that
## vehicle with the entry's files, skin and direction - its couplings, crew and load stay. An
## unnamed one is a vehicle added to it: the couplings and velocity of the vehicle before it (of
## the first named one, when it comes first), nobody aboard, no load, no gap, and a name of its own.
func get_arranged_dynamics(arranged:Array[MaszynaDynamicData]) -> Array[MaszynaDynamicData]:
    var by_name:Dictionary[String, MaszynaDynamicData] = {}
    for dynamic:MaszynaDynamicData in dynamics:
        by_name[dynamic.name] = dynamic
    var first_named:String = ""
    for entry:MaszynaDynamicData in arranged:
        if entry.name:
            first_named = entry.name
            break
    if not by_name.has(first_named):
        return dynamics
    var result:Array[MaszynaDynamicData] = []
    for index:int in arranged.size():
        var entry:MaszynaDynamicData = arranged[index]
        var vehicle:MaszynaDynamicData
        if by_name.has(entry.name):
            vehicle = by_name[entry.name].duplicate()
        else:
            var before:MaszynaDynamicData = result.back() if result else by_name[first_named]
            vehicle = before.duplicate()
            vehicle.name = "%s_%d" % [first_named, index]
            vehicle.driver_type = MaszynaDynamicData.DriverType.DRIVER_NOBODY
            vehicle.load_name = ""
            vehicle.load_amount = 0.0
            vehicle.gap = 0.0
        vehicle.data_path = entry.data_path
        vehicle.file_name = entry.file_name
        vehicle.skin = entry.skin
        vehicle.direction = entry.direction
        result.append(vehicle)
    return result
