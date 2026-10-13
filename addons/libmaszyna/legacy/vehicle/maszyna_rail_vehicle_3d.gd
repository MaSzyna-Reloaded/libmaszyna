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

## The vehicle's id, registered with VehicleServer.vehicle_set_id(), which is how an event, a
## scenario or the console find a vehicle. It may be empty or repeated - everything that holds the
## vehicle uses its RID, so only a lookup by that id is affected.
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

## A change of the data in the tree rebuilds the vehicle once, whatever else changes in the same
## frame; the node processes only while a rebuild is pending
var _dirty:bool = false
## The vehicle this node made (MaszynaLegacyVehicleSystem), none out of the tree - or the one it
## stands for (set_vehicle_instance())
var _vehicle:RID = RID()
## The node stands for a vehicle somebody else made and frees (set_vehicle_instance())
var _proxy:bool = false


## The vehicle exists while the node is in the tree: made on entering it from what the node is set
## to - the data set before is in it, so no rebuild is pending
func _enter_tree() -> void:
    MaszynaLegacyVehicleSystem.vehicle_built.connect(_on_vehicle_built)
    VehicleServer.vehicle_freed.connect(_on_vehicle_freed)
    GameDataServer.data_unload_requested.connect(_on_data_unload_requested)
    if _proxy:
        set_vehicle(_vehicle)
    else:
        _create_vehicle()
    _dirty = false


## ... and freed on leaving it (the editor takes a scene out of the tree when another one's tab is
## shown), while the node still hears of it (_on_vehicle_freed())
func _exit_tree() -> void:
    if not _proxy:
        _free_vehicle()
    MaszynaLegacyVehicleSystem.vehicle_built.disconnect(_on_vehicle_built)
    VehicleServer.vehicle_freed.disconnect(_on_vehicle_freed)
    GameDataServer.data_unload_requested.disconnect(_on_data_unload_requested)


## Makes the node stand for [param vehicle], made by somebody else (a scenery's dynamic): called
## before the node enters the tree, it takes the vehicle's data into its own and neither makes nor
## frees the vehicle. A change of the data stays in the node - the vehicle is its owner's, built
## anew when the owner builds it from the changed data (a scenery written and loaded again).
func set_vehicle_instance(vehicle:RID) -> void:
    var dynamic:MaszynaDynamicData = MaszynaLegacyVehicleSystem.vehicle_get_dynamic(vehicle)
    vehicle_id = dynamic.vehicle_id
    data_path = dynamic.data_path
    file_name = dynamic.file_name
    skin = dynamic.skin
    initial_velocity = dynamic.velocity
    driver_type = dynamic.driver_type
    load_name = dynamic.load_name
    load_amount = dynamic.load_amount
    _proxy = true
    _vehicle = vehicle
    _dirty = false


## false while a rebuild is pending - true once it ran, even when the vehicle failed to load
func is_built() -> bool:
    return not _dirty and (not _vehicle.is_valid() or MaszynaLegacyVehicleSystem.vehicle_is_built(_vehicle))


## A change makes the vehicle anew - a vehicle of the node's own (set_vehicle_instance())
func _process(_delta:float) -> void:
    set_process(false)
    if not _dirty:
        return
    _dirty = false
    if _proxy:
        return
    _free_vehicle()
    _create_vehicle()


## The vehicle made from what the node is set to; MaszynaLegacyVehicleSystem builds it in its turn
func _create_vehicle() -> void:
    var dynamic:MaszynaDynamicData = MaszynaDynamicData.new()
    dynamic.vehicle_id = vehicle_id
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
    if _proxy:
        return
    _free_vehicle()
    _dirty = true
    set_process(true)


## The node hears of it as of any other freeing of its vehicle (_on_vehicle_freed())
func _free_vehicle() -> void:
    if _vehicle.is_valid():
        MaszynaLegacyVehicleSystem.vehicle_free(_vehicle)
