@tool
extends HBoxContainer

## "Edit SCN" switched on for the scenery - its sectors are shown (scenery_sector_inspector.gd)
signal editing_started(scenery:MaszynaIncludeNode)
## "Edit SCN" switched off
signal editing_stopped

var _selected_scenery:MaszynaIncludeNode

@onready var btn = $Editable


func _ready():
    EditorInterface.get_selection().selection_changed.connect(_on_selection_changed)
    btn.disabled = true


func _exit_tree():
    EditorInterface.get_selection().selection_changed.disconnect(_on_selection_changed)


func _find_parent_scenery(node: Node):
    if not node:
        return null

    if node is MaszynaIncludeNode:
        return node
    return _find_parent_scenery(node.get_parent())


func _on_selection_changed():
    var sel:EditorSelection = EditorInterface.get_selection()
    var nodes = sel.get_selected_nodes()

    _selected_scenery = null

    if nodes.size() == 1:
        var n:Node = nodes[0]
        _selected_scenery = _find_parent_scenery(n)

    btn.disabled = false if _selected_scenery else true
    # shown, not switched: a selection - a proxy of a sector among them - must not toggle it
    btn.set_pressed_no_signal(_selected_scenery and _selected_scenery.editable_in_editor)

func _on_editable_toggled(toggled_on):
    if _selected_scenery:
        _selected_scenery.editable_in_editor = toggled_on
        if toggled_on:
            editing_started.emit(_selected_scenery)
        else:
            editing_stopped.emit()
