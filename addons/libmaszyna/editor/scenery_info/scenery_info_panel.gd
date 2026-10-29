@tool
extends VBoxContainer

## What the scenery of the edited scene (its MaszynaSceneryNode) found of the game's data at its last
## load, and what its readers gave up on (MaszynaIncludeNode.get_missing_files_by_type()) - shown again as
## another scene is edited, as the scenery loads or is cleared, and on Refresh

## A missing file's MaszynaFileIndexer.FileType as the list names it
const TYPE_NAMES: Dictionary[MaszynaFileIndexer.FileType, String] = {
    MaszynaFileIndexer.MASZYNA_FILE_INCLUDE: "Include",
    MaszynaFileIndexer.MASZYNA_FILE_FIZ: "FIZ",
    MaszynaFileIndexer.MASZYNA_FILE_MMD: "MMD",
    MaszynaFileIndexer.MASZYNA_FILE_MODEL: "Model",
    MaszynaFileIndexer.MASZYNA_FILE_TEXTURE: "Texture",
    MaszynaFileIndexer.MASZYNA_FILE_SOUND: "Sound",
}

## The scenery shown, none when the edited scene has no MaszynaSceneryNode
var _scenery: MaszynaSceneryNode = null


## Shows the scenery of [param scene_root], the scene edited now (EditorPlugin.scene_changed)
func show_scenery(scene_root: Node) -> void:
    if is_instance_valid(_scenery):
        _scenery.load_ended.disconnect(refresh)
        _scenery.cleared.disconnect(refresh)
    _scenery = null
    if scene_root:
        var sceneries: Array[Node] = scene_root.find_children("", "MaszynaSceneryNode", true, false)
        if scene_root is MaszynaSceneryNode:
            sceneries.push_front(scene_root)
        if sceneries:
            _scenery = sceneries[0] as MaszynaSceneryNode
            _scenery.load_ended.connect(refresh)
            _scenery.cleared.connect(refresh)
    refresh()


func refresh() -> void:
    var missing: Dictionary = _scenery.get_missing_files_by_type() if is_instance_valid(_scenery) else {}
    var found: int = _scenery.get_found_files_count() if is_instance_valid(_scenery) else 0
    %Summary.text = "Files: %d   Found: %d   Missing: %d" % [found + missing.size(), found, missing.size()]
    var paths: Array = missing.keys()
    paths.sort()
    %Missing.clear()
    var root: TreeItem = %Missing.create_item()
    for path: String in paths:
        var item: TreeItem = %Missing.create_item(root)
        item.set_text(0, TYPE_NAMES[missing[path]])
        item.set_text(1, path)


func _on_visibility_changed() -> void:
    if is_visible_in_tree():
        refresh()
