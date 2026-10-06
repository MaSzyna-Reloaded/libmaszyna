extends AudioListener3D
class_name TrainSoundListener3D

signal context_changed

const EXTERIOR_CONTEXT:int = 5

## The vehicle whose cab the listener is in (VehicleServer's), none outside
var listener_vehicle:RID = RID()
var listener_cabin:Cabin3D
var listener_context:int = EXTERIOR_CONTEXT

var _camera:Camera3D


func _ready() -> void:
    _camera = get_viewport().get_camera_3d()
    make_current()
    _refresh_context()
    TrainSoundSystem.set_listener(self)


func _exit_tree() -> void:
    TrainSoundSystem.clear_listener(self)


func _process(_delta:float) -> void:
    # follows the cab/free camera or the external view camera, whichever is current
    _camera = get_viewport().get_camera_3d()
    global_transform = _camera.global_transform
    _refresh_context()


func is_inside_vehicle(vehicle:RID) -> bool:
    return not listener_cabin == null and listener_vehicle == vehicle


func _refresh_context() -> void:
    var cabin:Cabin3D = _camera_cabin()
    var vehicle:RID = cabin.get_vehicle_rid() if not cabin == null else RID()
    var context:int = cabin.get_sound_listener_context() if not cabin == null else EXTERIOR_CONTEXT
    if listener_vehicle == vehicle and listener_cabin == cabin and listener_context == context:
        return
    listener_vehicle = vehicle
    listener_cabin = cabin
    listener_context = context
    context_changed.emit()


func _camera_cabin() -> Cabin3D:
    var node:Node = _camera
    while node:
        if node is Cabin3D:
            return node as Cabin3D
        node = node.get_parent()
    return null
