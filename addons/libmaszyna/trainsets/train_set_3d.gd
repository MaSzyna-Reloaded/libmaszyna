@tool
extends Node3D
class_name TrainSet3D

## A trainset assembled by hand - a node over RailVehicleServer's trainset (get_rid()), as a loaded
## scenery's "trainset:" block is one built by SceneryInstancer without a node. Its RailVehicle3D
## children are the trainset's vehicles, in order: the server stands them on the trainset's track
## one after another from its front, each as long as it is, and couples each with the next as
## "endtrainset" does. Reordered, they stand anew. A vehicle of a trainset has no start track of
## its own.

## Where the trainset's front stands - the track and the distance along it [m]
@export var start_track_name:String = "":
    set(x):
        if not x == start_track_name:
            start_track_name = x
            _dirty = true
            set_process(true)
@export var start_track_offset:float = 0.0:
    set(x):
        if not x == start_track_offset:
            start_track_offset = x
            _dirty = true
            set_process(true)
## How far each vehicle stands behind the one before it [m], by child - the `offset` of its
## `dynamic`; a reversed vehicle stands right behind (simulationstateserializer.cpp:1066)
@export var vehicle_gaps:Array[float] = []:
    set(x):
        if not x == vehicle_gaps:
            vehicle_gaps = x
            _dirty = true
            set_process(true)
## Coupling between child vehicle i and i + 1 (the scenery "couplingdata" of vehicle i), a mask of
## RailVehicleController.CouplingFlags (coupling::, MOVER.h:162)
@export_flags("Coupler:1", "BrakeHose:2", "Control:4", "HighVoltage:8", "Gangway:16", "MainHose:32",
        "Heating:64", "Permanent:128", "Power24V:256", "Power110V:512", "Power3x400V:1024")
var couplings:Array[int] = []:
    set(x):
        if not x == couplings:
            couplings = x
            _dirty = true
            set_process(true)
## The trainset's timetable (a file in the scenery's directory, `none` for none) and the velocity
## it starts with - its driver's first orders (deserialize_endtrainset(),
## simulationstateserializer.cpp:839-848)
@export var timetable:String = ""
@export var velocity:float = 0.0

var _trainset:RID = RID()
## The track the trainset was last handed - another one registered under its name moves it there
var _track:RID = RID()
## The trainset is handed over once, whatever changed in the frame; the node processes only then
var _dirty:bool = false
## The trainset got its initial placement from one of its MaszynaRailVehicle3D children.
var _start_track_taken_from_child:bool = false


## The trainset's handle in RailVehicleServer
func get_rid() -> RID:
    return _trainset


## The children come and go at any time and each takes its vehicle's handle only once built, so the
## trainset follows them from here rather than from a scene. They enter the tree after it and leave
## before it, so each is followed from its own entry to its own exit.
func _enter_tree() -> void:
    _trainset = RailVehicleServer.trainset_create()
    RailVehicleServer.trainset_set_name(_trainset, name)
    TrackServer.tracks_changed.connect(_on_tracks_changed)
    child_order_changed.connect(_on_child_order_changed)
    child_entered_tree.connect(_on_child_entered_tree)
    child_exiting_tree.connect(_on_child_exiting_tree)
    _dirty = true
    set_process(true)


func _exit_tree() -> void:
    TrackServer.tracks_changed.disconnect(_on_tracks_changed)
    child_order_changed.disconnect(_on_child_order_changed)
    child_entered_tree.disconnect(_on_child_entered_tree)
    child_exiting_tree.disconnect(_on_child_exiting_tree)
    RailVehicleServer.trainset_free(_trainset)
    _trainset = RID()


func _on_child_entered_tree(node:Node) -> void:
    var vehicle:RailVehicle3D = node as RailVehicle3D
    if vehicle:
        vehicle.vehicle_changed.connect(_on_child_order_changed)
    var maszyna_vehicle:MaszynaRailVehicle3D = node as MaszynaRailVehicle3D
    if not maszyna_vehicle:
        return
    if not start_track_name and maszyna_vehicle.start_track_name:
        start_track_name = maszyna_vehicle.start_track_name
        start_track_offset = maszyna_vehicle.start_track_offset
        _start_track_taken_from_child = true
    if _start_track_taken_from_child:
        maszyna_vehicle.start_track_name = ""
        maszyna_vehicle.start_track_offset = 0.0


func _on_child_exiting_tree(node:Node) -> void:
    var vehicle:RailVehicle3D = node as RailVehicle3D
    if vehicle:
        vehicle.vehicle_changed.disconnect(_on_child_order_changed)


func _on_child_order_changed() -> void:
    _dirty = true
    set_process(true)


func _on_tracks_changed() -> void:
    if not TrackServer.track_get_rid_by_name(start_track_name) == _track:
        _dirty = true
        set_process(true)


func _process(_delta:float) -> void:
    set_process(false)
    if not _dirty:
        return
    _dirty = false
    _track = TrackServer.track_get_rid_by_name(start_track_name)
    RailVehicleServer.trainset_set_track(_trainset, _track, start_track_offset)
    RailVehicleServer.trainset_clear(_trainset)
    var index:int = 0
    for child:Node in get_children():
        var vehicle:RailVehicle3D = child as RailVehicle3D
        if not vehicle:
            continue
        # a vehicle not built yet stands the trainset anew once it is (vehicle_changed)
        if not vehicle.get_rid().is_valid():
            return
        RailVehicleServer.trainset_add_vehicle(
                _trainset, vehicle.get_rid(), vehicle.start_direction,
                vehicle_gaps[index] if index < vehicle_gaps.size() else 0.0,
                couplings[index] if index < couplings.size() else 0)
        index += 1
    RailVehicleServer.trainset_place(_trainset)
