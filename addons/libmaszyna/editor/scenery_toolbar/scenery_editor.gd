@tool
extends RefCounted


static func update_owners(root:Node3D) -> void:
    var scene_owner:Node = null
    if root.editable_in_editor:
        scene_owner = root.owner if root.owner else root
    var children:Array[Node] = root.get_children(true)
    for child:Node in children:
        _set_owner_recursive(child, scene_owner)

    # Setting .owner alone never fires a tree structure change, so the Scene
    # dock (which rebuilds its view off add_child()/remove_child()) never
    # notices root's children gained or lost an owner and keeps showing root
    # as childless. Reparenting one child in place (a real
    # add_child()/remove_child() pair) is enough to trigger a full dock resync.
    if Engine.is_editor_hint() and children and root.is_inside_tree():
        var nudge:Node = children[0]
        var idx:int = nudge.get_index()
        root.remove_child(nudge)
        root.add_child(nudge)
        root.move_child(nudge, idx)


static func _set_owner_recursive(node:Node, scene_owner:Node) -> void:
    node.owner = scene_owner
    if node is SfxPlayer3D:
        return
    for child:Node in node.get_children(true):
        _set_owner_recursive(child, scene_owner)
