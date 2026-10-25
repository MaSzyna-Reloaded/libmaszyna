@tool
extends EditorPlugin

var user_settings_dock
var user_settings_dock_scene = preload("res://addons/libmaszyna/editor/user_settings_dock/user_settings_dock.tscn")


func _enter_tree() -> void:
    user_settings_dock = user_settings_dock_scene.instantiate()
    add_control_to_dock(DOCK_SLOT_RIGHT_UL, user_settings_dock)

func _exit_tree() -> void:
    if user_settings_dock:
        remove_control_from_docks(user_settings_dock)
        user_settings_dock.free()
        user_settings_dock = null
