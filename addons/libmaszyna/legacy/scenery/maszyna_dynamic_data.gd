@tool
extends Resource
class_name MaszynaDynamicData

## Scenery `node ... <name> dynamic <datafolder> <skinfile> <mmdfile> ... enddynamic`
## (deserialize_dynamic(), simulationstateserializer.cpp:960-1076) - a vehicle of a trainset
## (MaszynaTrainsetData), built by SceneryInstancer through the servers

## Who is aboard, in the words the `.scn` uses for it - a `dynamic` names `headdriver`,
## `reardriver` or `nobody` as its drivertype (DynObj.cpp:1812-1825): the driver sits down in the
## front or the rear cab (DynObj.cpp:1940-1963), nobody's vehicle is not simulated at all
## (Driver.cpp:2126)
enum DriverType {
    DRIVER_NOBODY,
    DRIVER_HEAD,
    DRIVER_REAR,
}

## The scenery's name of the vehicle; may be empty or repeated
@export var name:String = ""
## Where its files are, under the game directory (`dynamic/pkp/303e_v1`)
@export var data_path:String = ""
## Its MMD/FIZ file name, without the extension
@export var file_name:String = ""
@export var skin:String = ""
## An `offset` of -1 stands the vehicle reversed (simulationstateserializer.cpp:983, DynObj.cpp:1807)
@export var direction:TrackServer.Direction = TrackServer.DIRECTION_NORMAL
## How far it stands behind the vehicle before it [m] - its `offset`
## (simulationstateserializer.cpp:1066)
@export var gap:float = 0.0
## Its coupling with the next vehicle of the trainset (`couplingdata`), a mask of
## RailVehicleController.CouplingFlags
@export var coupling:int = 0
## The velocity it starts with [km/h]
@export var velocity:float = 0.0
@export var driver_type:DriverType = DriverType.DRIVER_NOBODY
@export var load_name:String = ""
@export var load_amount:float = 0.0
