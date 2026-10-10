@tool
extends EditorPlugin

const SceneryLoadInspector = preload("./scenery_load_inspector.gd")
const ScenerySectorInspector = preload("./scenery_sector_inspector.gd")
const SceneryModelGizmoPlugin = preload("./scenery_model_gizmo_plugin.gd")

var scenery_toolbar = preload("./toolbar_scenery_instance.tscn")
var scenery_toolbar_instance
var _load_inspector:SceneryLoadInspector = null
var _sector_inspector:ScenerySectorInspector = null
var _model_gizmo_plugin:SceneryModelGizmoPlugin = null

func _enter_tree() -> void:
    _sector_inspector = ScenerySectorInspector.new()
    _model_gizmo_plugin = SceneryModelGizmoPlugin.new(_sector_inspector)
    add_node_3d_gizmo_plugin(_model_gizmo_plugin)
    scenery_toolbar_instance = scenery_toolbar.instantiate()
    scenery_toolbar_instance.editing_started.connect(_sector_inspector.inspect)
    scenery_toolbar_instance.editing_stopped.connect(_sector_inspector.stop)
    add_control_to_container(CONTAINER_SPATIAL_EDITOR_MENU, scenery_toolbar_instance)
    _load_inspector = SceneryLoadInspector.new()
    add_inspector_plugin(_load_inspector)
    scene_saved.connect(_on_scene_saved)

func _exit_tree() -> void:
    scene_saved.disconnect(_on_scene_saved)
    remove_inspector_plugin(_load_inspector)
    _load_inspector = null
    remove_control_from_container(CONTAINER_SPATIAL_EDITOR_MENU, scenery_toolbar_instance)
    scenery_toolbar_instance.free()
    scenery_toolbar_instance = null
    _sector_inspector.stop()
    remove_node_3d_gizmo_plugin(_model_gizmo_plugin)
    _model_gizmo_plugin = null
    _sector_inspector = null


## Before the scene is saved: the sectors are a view of the scenery, not a part of the scene
func _apply_changes() -> void:
    _sector_inspector.clear()


func _on_scene_saved(_filepath:String) -> void:
    _sector_inspector.show_camera_sector()
