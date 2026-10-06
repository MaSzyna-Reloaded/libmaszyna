@tool
extends RefCounted
class_name MaszynaEditorViewFocus

## The editor's 3D view brought to what is selected - a trainset's vehicles, a vehicle just spawned

## The editor shortcut of the 3D view's "Focus Selection" (node_3d_editor_plugin.cpp:10692)
const FOCUS_SELECTION_SHORTCUT:String = "spatial_editor/focus_selection"


## The 3D view is shown and centred on the selected nodes
static func focus_selection() -> void:
    EditorInterface.set_main_screen_editor("3D")
    # The 3D view has no API for its camera. What moves it is its own "Focus Selection"
    # (Node3DEditorViewport::focus_selection(), node_3d_editor_plugin.cpp:4421), an item of the
    # view's menu - found by the editor shortcut it carries (:6900), not by its translated text.
    # The menu is in the Node3DEditorViewport around the view's SubViewportContainer.
    var focus:Shortcut = EditorInterface.get_editor_settings().get_shortcut(FOCUS_SELECTION_SHORTCUT)
    var viewport_editor:Node = EditorInterface.get_editor_viewport_3d(0).get_parent().get_parent()
    for node:Node in viewport_editor.find_children("", "MenuButton", true, false):
        var menu:PopupMenu = (node as MenuButton).get_popup()
        for index:int in menu.item_count:
            if menu.get_item_shortcut(index) == focus:
                menu.id_pressed.emit(menu.get_item_id(index))
                return
