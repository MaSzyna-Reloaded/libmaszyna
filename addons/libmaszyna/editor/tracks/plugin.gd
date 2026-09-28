@tool
extends EditorPlugin

var toolbar:PackedScene = preload("./toolbar.tscn")
var toolbar_instance:Control

func _enter_tree() -> void:
    toolbar_instance = toolbar.instantiate()
    add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_instance)

func _exit_tree() -> void:
    remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, toolbar_instance)
    toolbar_instance.free()
    toolbar_instance = null
