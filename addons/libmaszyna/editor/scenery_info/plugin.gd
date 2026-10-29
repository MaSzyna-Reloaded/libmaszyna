@tool
extends EditorPlugin

## The panel of the scenery's files (SceneryInfoPanel), beside the Nodebank in the bottom panel,
## following the scene edited

const PANEL_SCENE: PackedScene = preload("./scenery_info_panel.tscn")

var _panel: Control


func _enter_tree() -> void:
    _panel = PANEL_SCENE.instantiate()
    add_control_to_bottom_panel(_panel, "Scenery Info")
    scene_changed.connect(_panel.show_scenery)
    _panel.show_scenery(EditorInterface.get_edited_scene_root())


func _exit_tree() -> void:
    scene_changed.disconnect(_panel.show_scenery)
    remove_control_from_bottom_panel(_panel)
    _panel.free()
    _panel = null
