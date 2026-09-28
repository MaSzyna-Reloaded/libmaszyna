@tool
extends HBoxContainer

var _t = 0.0
var _selected_e3d:E3DModelInstance

@onready var btn = $Editable


func _ready():
    EditorInterface.get_selection().selection_changed.connect(_on_selection_changed)
    btn.disabled = true


func _exit_tree():
    EditorInterface.get_selection().selection_changed.disconnect(_on_selection_changed)


func _find_parent_e3d(node: Node):
    if not node:
        return null

    if node is E3DModelInstance:
        return node
    return _find_parent_e3d(node.get_parent())


func _on_selection_changed():
    var sel:EditorSelection = EditorInterface.get_selection()
    var previously_selected:E3DModelInstance = _selected_e3d

    var nodes = sel.get_selected_nodes()

    _selected_e3d = null
    btn.button_pressed = false

    if nodes.size() == 1:
        var n:Node = nodes[0]
        _selected_e3d = _find_parent_e3d(n)

    btn.disabled = false if _selected_e3d else true
    btn.button_pressed = _selected_e3d and _selected_e3d.editable_in_editor

func _on_editable_toggled(toggled_on):
    if _selected_e3d:
        _selected_e3d.editable_in_editor = toggled_on
