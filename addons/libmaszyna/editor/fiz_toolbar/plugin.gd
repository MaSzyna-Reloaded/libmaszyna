@tool
extends EditorPlugin

var fiz_instance_toolbar = preload("./toolbar_fiz_instance.tscn")
var fiz_instance_toolbar_instance
## The vehicles shown as nodes before the scene was saved, shown again once it is
var _saved_editable_vehicles:Array[RailVehicle3D] = []

func _enter_tree() -> void:
    fiz_instance_toolbar_instance = fiz_instance_toolbar.instantiate()
    add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, fiz_instance_toolbar_instance)
    scene_saved.connect(_on_scene_saved)

func _exit_tree() -> void:
    scene_saved.disconnect(_on_scene_saved)
    remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, fiz_instance_toolbar_instance)
    fiz_instance_toolbar_instance.free()
    fiz_instance_toolbar_instance = null


## Before the scene is saved: a vehicle's nodes are a view of it, not a part of the scene
func _apply_changes() -> void:
    _saved_editable_vehicles.clear()
    var scene_root:Node = EditorInterface.get_edited_scene_root()
    if not scene_root:
        return
    for node:Node in scene_root.find_children("", "RailVehicle3D", true, false):
        var vehicle:RailVehicle3D = node as RailVehicle3D
        if vehicle.is_editable_in_editor():
            vehicle.set_editable_in_editor(false)
            _saved_editable_vehicles.append(vehicle)


func _on_scene_saved(_filepath:String) -> void:
    for vehicle:RailVehicle3D in _saved_editable_vehicles:
        if is_instance_valid(vehicle):
            vehicle.set_editable_in_editor(true)
    _saved_editable_vehicles.clear()
