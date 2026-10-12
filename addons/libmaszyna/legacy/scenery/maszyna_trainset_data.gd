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
## vehicle with an id there is one of its own, its declared ones otherwise. An entry with an id is
## that vehicle with the entry's files, skin and direction - its couplings, crew and load stay. One
## without is a vehicle added to it: the couplings and velocity of the vehicle before it (of the
## first one with an id, when it comes first), nobody aboard, no load, no gap, an id of its own and
## no name.
func get_arranged_dynamics(arranged:Array[MaszynaDynamicData]) -> Array[MaszynaDynamicData]:
    var by_id:Dictionary[String, MaszynaDynamicData] = {}
    for dynamic:MaszynaDynamicData in dynamics:
        by_id[dynamic.vehicle_id] = dynamic
    var first_id:String = ""
    for entry:MaszynaDynamicData in arranged:
        if entry.vehicle_id:
            first_id = entry.vehicle_id
            break
    if not by_id.has(first_id):
        return dynamics
    var result:Array[MaszynaDynamicData] = []
    for index:int in arranged.size():
        var entry:MaszynaDynamicData = arranged[index]
        var vehicle:MaszynaDynamicData
        if by_id.has(entry.vehicle_id):
            vehicle = by_id[entry.vehicle_id].duplicate()
        else:
            var before:MaszynaDynamicData = result.back() if result else by_id[first_id]
            vehicle = before.duplicate()
            vehicle.vehicle_id = "%s_%d" % [first_id, index]
            vehicle.name = ""
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
