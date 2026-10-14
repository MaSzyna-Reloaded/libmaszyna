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


## Its vehicles as `arranged` lists them (MaszynaIncludeNode.trainset_override) when every declared
## vehicle there stands at its place in this trainset, its declared ones otherwise. An entry is paired
## with its declared vehicle by its place in the trainset (MaszynaDynamicData.trainset_index), as
## the original couples a trainset by position (simulationstateserializer.cpp:818-834) - an id may
## repeat (linia61's container wagons). A declared vehicle comes with the entry's files, skin and
## direction - its couplings, crew and load stay. An added one (no place) gets the couplings and
## velocity of the vehicle before it (of the first declared one, when it comes first), nobody
## aboard, no load, no gap, an id of its own and no name.
func get_arranged_dynamics(arranged:Array[MaszynaDynamicData]) -> Array[MaszynaDynamicData]:
    var first_declared:MaszynaDynamicData = null
    for entry:MaszynaDynamicData in arranged:
        if entry.trainset_index < 0:
            continue
        if (entry.trainset_index >= dynamics.size()
                or not dynamics[entry.trainset_index].vehicle_id == entry.vehicle_id):
            return dynamics
        if not first_declared:
            first_declared = dynamics[entry.trainset_index]
    if not first_declared:
        return dynamics
    var result:Array[MaszynaDynamicData] = []
    for index:int in arranged.size():
        var entry:MaszynaDynamicData = arranged[index]
        var vehicle:MaszynaDynamicData
        if entry.trainset_index >= 0:
            vehicle = dynamics[entry.trainset_index].duplicate()
        else:
            var before:MaszynaDynamicData = result.back() if result else first_declared
            vehicle = before.duplicate()
            vehicle.vehicle_id = "%s_%d" % [first_declared.vehicle_id, index]
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
