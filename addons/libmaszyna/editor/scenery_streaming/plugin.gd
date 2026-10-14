@tool
extends EditorPlugin

## Shows streaming diagnostics and loading progress in the editor. The scene's SceneryStreamer
## owns the streaming camera and its active property; the editor supplies a temporary view camera
## without changing the scene's configured camera. A scene of a scenery alone - a MaszynaSceneryContentNode
## and no SceneryStreamer, as the scenery importer makes it - is streamed around the editor's view by
## the plugin itself; a scene with a SceneryStreamer, active or not, never is.

const STREAMING_DOCK:PackedScene = preload("./scenery_streaming_dock.tscn")
const LOAD_INDICATOR:PackedScene = preload("./scenery_load_indicator.tscn")
const SceneryLoadIndicator = preload("./scenery_load_indicator.gd")

var _streaming_dock:Control = null
## Floating over the 3D view while a scenery of the edited scene loads
var _load_indicator:SceneryLoadIndicator = null
## Streamers of the edited scene receiving the editor's temporary view.
var _streamer_ids:Array[int] = []
## The plugin streams the edited scene around the editor's view (no SceneryStreamer in it)
var _streams_edited_scene:bool = false


func _enter_tree() -> void:
    _streaming_dock = STREAMING_DOCK.instantiate()
    add_control_to_bottom_panel(_streaming_dock, "Scenery Streaming")
    _load_indicator = LOAD_INDICATOR.instantiate()
    # over the 3D view itself, where it moves nothing of the editor's layout. The editor has no
    # API for the control around the view: it is the Node3DEditorViewport holding the view's
    # SubViewportContainer.
    EditorInterface.get_editor_viewport_3d(0).get_parent().get_parent().add_child(_load_indicator)
    scene_changed.connect(_load_indicator.set_scene_root)
    _load_indicator.set_scene_root(EditorInterface.get_edited_scene_root())
    scene_changed.connect(set_scene_root)
    get_tree().node_added.connect(_on_node_added)
    set_scene_root(EditorInterface.get_edited_scene_root())


func set_scene_root(root:Node) -> void:
    for streamer_id:int in _streamer_ids:
        var streamer:SceneryStreamer = instance_from_id(streamer_id) as SceneryStreamer
        if streamer:
            streamer.editor_camera = null
    _streamer_ids.clear()
    # before the streamers of the new scene set their camera
    if _streams_edited_scene:
        _streams_edited_scene = false
        SceneryStreamingServer.streaming_set_camera(0)
    if not root:
        return
    var streamers:Array[Node] = root.find_children("", "SceneryStreamer", true, true)
    if root is SceneryStreamer:
        streamers.append(root)
    for node:Node in streamers:
        var streamer:SceneryStreamer = node as SceneryStreamer
        streamer.editor_camera = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
        _streamer_ids.append(streamer.get_instance_id())
    if not streamers and (root is MaszynaSceneryContentNode or root.find_children("", "MaszynaSceneryContentNode", true, true)):
        _streams_edited_scene = true
        SceneryStreamingServer.streaming_set_camera(EditorInterface.get_editor_viewport_3d(0).get_camera_3d().get_instance_id())


func _on_node_added(node:Node) -> void:
    if not node is SceneryStreamer:
        return
    var root:Node = EditorInterface.get_edited_scene_root()
    if root and root.is_ancestor_of(node):
        var streamer:SceneryStreamer = node as SceneryStreamer
        # a streamer added to the scene takes the streaming over
        _streams_edited_scene = false
        streamer.editor_camera = EditorInterface.get_editor_viewport_3d(0).get_camera_3d()
        _streamer_ids.append(streamer.get_instance_id())


func _exit_tree() -> void:
    get_tree().node_added.disconnect(_on_node_added)
    scene_changed.disconnect(set_scene_root)
    set_scene_root(null)
    remove_control_from_bottom_panel(_streaming_dock)
    _streaming_dock.free()
    _streaming_dock = null
    scene_changed.disconnect(_load_indicator.set_scene_root)
    _load_indicator.free()
    _load_indicator = null
