@tool
extends RailVehicle3D
class_name MaszynaRailVehicle3D

## A complete, driveable MaSzyna vehicle from nothing but a data_path/file_name/skin triple, placed
## by hand - a node over a vehicle of MaszynaLegacyVehicleSystem, which builds it through the
## servers as it builds a scenery's `dynamic`s (those have no node). The node rides on its vehicle
## (RailVehicleRenderingServer moves it). Everything it stores is its own data below and what
## RailVehicle3D has of a vehicle's place (start_track_*, head_display_material); the vehicle is
## never saved with the scene.

## The vehicle has been (re)built: it has its handle now - or none when its data cannot be read
signal vehicle_built

@export var data_path:String = "":
    set(x):
        if not x == data_path:
            data_path = x
            _dirty = true
            set_process(true)

## Base filename, without extension, shared by this vehicle's .e3d (exterior model), .fiz
## (physics) and .mmd (cabin) files under data_path.
@export var file_name:String = "":
    set(x):
        if not x == file_name:
            file_name = x
            _dirty = true
            set_process(true)

## Base skin name expanded to numbered dynamic-material slots, or an explicit pipe-separated
## slot list when a vehicle uses mixed material names.
@export var skin:String = "":
    set(x):
        if not x == skin:
            skin = x
            _dirty = true
            set_process(true)

## The scenery's name for this vehicle, registered with VehicleServer.vehicle_set_name(), which is
## how an event, a scenario or the console find a vehicle by name. It may be empty or repeated -
## everything that holds the vehicle uses its RID, so only a lookup by that name is affected.
@export var vehicle_id:String = "":
    set(x):
        if not x == vehicle_id:
            vehicle_id = x
            _dirty = true
            set_process(true)

## 0.0 (default) means the vehicle starts not-ready-to-depart (battery off, matching the original
## engine's scenery velocity token); a non-zero value marks it ready (battery on per
## battery_start_mode).
@export var initial_velocity:float = 0.0:
    set(x):
        if not x == initial_velocity:
            initial_velocity = x
            _dirty = true
            set_process(true)

## Who is aboard, in the words the `.scn` uses for it (MaszynaDynamicData.DriverType)
@export var driver_type:MaszynaDynamicData.DriverType = MaszynaDynamicData.DriverType.DRIVER_NOBODY:
    set(x):
        if not x == driver_type:
            driver_type = x
            _dirty = true
            set_process(true)

## What the scenery loaded the vehicle with: the cargo's own name and how much of it
## (`loadcount` and `loadtype` of a `dynamic`).
@export var load_name:String = "":
    set(x):
        if not x == load_name:
            load_name = x
            _dirty = true
            set_process(true)

@export var load_amount:float = 0.0:
    set(x):
        if not x == load_amount:
            load_amount = x
            _dirty = true
            set_process(true)

## A change of the data rebuilds the vehicle once, whatever else changes in the same frame; the node
## processes only while a rebuild is pending
var _dirty:bool = true
## The vehicle this node made (MaszynaLegacyVehicleSystem), none before its first build
var _vehicle:RID = RID()


func _enter_tree() -> void:
    MaszynaLegacyVehicleSystem.vehicle_built.connect(_on_vehicle_built)
    VehicleServer.vehicle_freed.connect(_on_vehicle_freed)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)


func _exit_tree() -> void:
    MaszynaLegacyVehicleSystem.vehicle_built.disconnect(_on_vehicle_built)
    VehicleServer.vehicle_freed.disconnect(_on_vehicle_freed)
    GameDataServer.data_unload_requested.disconnect(_on_data_unload_requested)


## Out of the tree the vehicle stays (the editor takes a scene out of the tree when another one's
## tab is shown); it goes with the node
func _notification(what:int) -> void:
    if what == NOTIFICATION_PREDELETE and _vehicle.is_valid():
        MaszynaLegacyVehicleSystem.vehicle_free(_vehicle)


## false while a rebuild is pending - true once it ran, even when the vehicle failed to load
func is_built() -> bool:
    return not _dirty and (not _vehicle.is_valid() or MaszynaLegacyVehicleSystem.vehicle_is_built(_vehicle))


## A change makes the vehicle anew from what the node is set to; MaszynaLegacyVehicleSystem builds
## it in its turn
func _process(_delta:float) -> void:
    set_process(false)
    if not _dirty:
        return
    _dirty = false
    _free_vehicle()
    var dynamic:MaszynaDynamicData = MaszynaDynamicData.new()
    dynamic.name = vehicle_id
    dynamic.data_path = data_path
    dynamic.file_name = file_name
    dynamic.skin = skin
    dynamic.velocity = initial_velocity
    dynamic.driver_type = driver_type
    dynamic.load_name = load_name
    dynamic.load_amount = load_amount
    _vehicle = MaszynaLegacyVehicleSystem.vehicle_create(dynamic, get_instance_id())


## Built, the vehicle is this node's: the node rides on it and places it on its start track
func _on_vehicle_built(vehicle:RID) -> void:
    if not vehicle == _vehicle:
        return
    if VehicleServer.vehicle_is_simulation_ready(vehicle):
        set_vehicle(vehicle)
    vehicle_built.emit()


## The vehicle freed by somebody else (the HUD removes a trainset) is not this node's any more
func _on_vehicle_freed(vehicle:RID) -> void:
    if vehicle == _vehicle:
        _vehicle = RID()
        set_vehicle(RID())


## The vehicle goes at once with the data it was built of - nothing of it is built again from the new
## data before it is itself - and is built anew in the next frame
func _on_data_unload_requested() -> void:
    _free_vehicle()
    _dirty = true
    set_process(true)


## The node hears of it as of any other freeing of its vehicle (_on_vehicle_freed())
func _free_vehicle() -> void:
    if _vehicle.is_valid():
        MaszynaLegacyVehicleSystem.vehicle_free(_vehicle)
