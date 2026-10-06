@tool
extends EditorPlugin

## The vehicles of the edited scene - a scenery's trainsets - in a bottom panel tab, next to
## Scenery Streaming, with the side views of their vehicles; "Show" takes the 3D view to one.
## Next to it the browser of every vehicle of the game, to add one to the scene; the trainsets'
## "Spawn..." opens it.

const TrainsetsDock = preload("./trainsets_dock.gd")
const TRAINSETS_DOCK:PackedScene = preload("./trainsets_dock.tscn")
const VEHICLES_PANEL:PackedScene = preload("./vehicles_panel.tscn")

var _trainsets_dock:TrainsetsDock = null
var _vehicles_panel:Control = null


func _enter_tree() -> void:
    _trainsets_dock = TRAINSETS_DOCK.instantiate()
    _vehicles_panel = VEHICLES_PANEL.instantiate()
    add_control_to_bottom_panel(_trainsets_dock, "Trainsets")
    add_control_to_bottom_panel(_vehicles_panel, "Vehicles")
    scene_changed.connect(_trainsets_dock.set_scene_root)
    _trainsets_dock.spawn_requested.connect(_on_spawn_requested)
    _trainsets_dock.set_scene_root(EditorInterface.get_edited_scene_root())


func _exit_tree() -> void:
    scene_changed.disconnect(_trainsets_dock.set_scene_root)
    _trainsets_dock.spawn_requested.disconnect(_on_spawn_requested)
    remove_control_from_bottom_panel(_trainsets_dock)
    remove_control_from_bottom_panel(_vehicles_panel)
    _trainsets_dock.free()
    _trainsets_dock = null
    _vehicles_panel.free()
    _vehicles_panel = null


func _on_spawn_requested() -> void:
    make_bottom_panel_item_visible(_vehicles_panel)
