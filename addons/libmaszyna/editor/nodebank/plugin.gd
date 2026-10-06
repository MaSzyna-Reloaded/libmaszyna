@tool
extends EditorPlugin

var nodebank_panel_scene = preload("./nodebank_panel.tscn")
var nodebank_panel_instance
var nodebank_bottom_button
var _viewport_drop: MaszynaEditorViewportDrop

func drag_start(item_data: NodebankGridItem) -> void:
    if item_data:
        _viewport_drop.start(item_data.model.duplicate() as E3DModelInstance, drag_end.bind(item_data))

func drag_end(world_position: Vector3, item_data: NodebankGridItem) -> void:
    var scene_root: Node = EditorInterface.get_edited_scene_root()
    if not scene_root or not scene_root is Node3D:
        push_warning("Nodebank item can only be added to a 3D scene root.")
        return

    var parent: Node3D = scene_root as Node3D
    var instance: E3DModelInstance = item_data.model.duplicate()
    instance.position = parent.global_transform.affine_inverse() * world_position

    var undo_redo: EditorUndoRedoManager = get_undo_redo()
    undo_redo.create_action("Add nodebank E3D model")
    undo_redo.add_do_method(parent, "add_child", instance, true)
    # the dropped node alone: the model it builds is generated, and owned it would be saved into
    # the scene - every mesh, material and texture of it
    undo_redo.add_do_property(instance, "owner", scene_root)
    undo_redo.add_undo_method(parent, "remove_child", instance)
    undo_redo.add_do_reference(instance)
    undo_redo.commit_action()

    EditorInterface.get_selection().clear()
    EditorInterface.get_selection().add_node(instance)

func _enter_tree() -> void:
    set_input_event_forwarding_always_enabled()

    _viewport_drop = MaszynaEditorViewportDrop.new()
    add_child(_viewport_drop)
    nodebank_panel_instance = nodebank_panel_scene.instantiate()
    nodebank_panel_instance.set_plugin_runtime()
    nodebank_panel_instance.item_drag_started.connect(drag_start)
    nodebank_bottom_button = add_control_to_bottom_panel(nodebank_panel_instance, "Nodebank")

func _exit_tree() -> void:
    nodebank_panel_instance.item_drag_started.disconnect(drag_start)
    remove_control_from_bottom_panel(nodebank_panel_instance)
    nodebank_panel_instance.free()
    nodebank_panel_instance = null
    _viewport_drop.stop()
    _viewport_drop.free()
    _viewport_drop = null
