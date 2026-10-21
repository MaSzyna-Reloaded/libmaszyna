@tool
extends VehicleData
class_name MaszynaVehicleData

## Scenery `node ... <name> dynamic <datafolder> <skinfile> <mmdfile> ... enddynamic`
## (deserialize_dynamic(), simulationstateserializer.cpp:960-1076): the core's VehicleData and which
## of MaSzyna's vehicles it is. The `.scn` names its driver type `headdriver`, `reardriver` or
## `nobody` (DynObj.cpp:1812-1825); nobody's vehicle is not simulated at all (Driver.cpp:2126). An
## `offset` of -1 stands it reversed (simulationstateserializer.cpp:983, DynObj.cpp:1807), its
## `offset` is its gap (simulationstateserializer.cpp:1066), its `couplingdata` its coupling.

## The catalogue's provider of MaSzyna's vehicles (MaszynaVehicleCatalogProvider)
const VEHICLE_PROVIDER:StringName = &"maszyna"

## Where its files are, under the game directory (`dynamic/pkp/303e_v1`) - the directory of its
## name (VehicleData.vehicle)
var data_path:String:
    get:
        return vehicle.get_base_dir()
    set(value):
        vehicle = value.trim_prefix("/").path_join(file_name)
## Its MMD/FIZ file name, without the extension - the file of its name
var file_name:String:
    get:
        return vehicle.get_file()
    set(value):
        vehicle = data_path.path_join(value)
## The skin, MaSzyna's name of its livery (VehicleData.livery)
var skin:String:
    get:
        return livery
    set(value):
        livery = value


func _init() -> void:
    vehicle_provider = VEHICLE_PROVIDER
