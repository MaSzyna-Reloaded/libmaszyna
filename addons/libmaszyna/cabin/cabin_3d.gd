extends Node3D
class_name Cabin3D

signal cabin_ready

var _dirty = true
var _cabin_ready:bool = false
var _e3d_instances:Array[E3DModelInstance] = []
var _e3d_loaded_count:int = 0

@export_node_path("TrainController") var controller_path:NodePath = NodePath(""):
    set(x):
        if not x == controller_path:
            _dirty = true
            controller_path = x

@export var camera_bound_min = Vector3.ZERO
@export var camera_bound_max = Vector3.ZERO
@export var camera_bound_enabled:bool = false
@export var driver_position = Vector3.ZERO

func get_camera_transform():
    return global_transform.translated_local(driver_position)

func _propagate_train_controller(node: Node, controller: TrainController):
    for child in node.get_children():
        _propagate_train_controller(child, controller)
        if "controller_path" in child:
            if controller:
                child.controller_path = child.get_path_to(controller)
            else:
                child.controller_path = NodePath("")

func set_train_controller(controller:TrainController):
    _propagate_train_controller(self, controller)

func _process(delta):
    if _dirty:
        _dirty = false
        if controller_path:
            var controller:TrainController = get_node(controller_path)
            set_train_controller(controller)

func _ready() -> void:
    _cabin_ready = true
    cabin_ready.emit()

func is_cabin_ready() -> bool:
    return _cabin_ready
