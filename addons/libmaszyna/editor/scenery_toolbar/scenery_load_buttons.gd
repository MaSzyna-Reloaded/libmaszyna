@tool
extends HBoxContainer

## "Load" and "Clear" of a scenery, side by side in the inspector, in place of its "Load" button
## (scenery_load_inspector.gd)

var _scenery:MaszynaSceneryNode = null


func set_scenery(scenery:MaszynaSceneryNode) -> void:
    _scenery = scenery


func _on_load_pressed() -> void:
    _scenery.load()


func _on_clear_pressed() -> void:
    _scenery.clear()
