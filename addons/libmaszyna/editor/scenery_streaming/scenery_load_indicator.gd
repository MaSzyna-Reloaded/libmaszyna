@tool
extends PanelContainer

## Shows that a scenery of the edited scene is loading, and how far it is: what the load is doing,
## a progress bar and "Stop" to its right, floating over the editor's 3D view - there only while a
## load is under way, and taking no place in the editor's layout. Its size is fixed and its colours
## its own: what the load is doing is cut to fit, so nothing moves as the text changes, and it
## reads on any view.

## Shown while the load says nothing of what it is doing
const CAPTION:String = "Loading scenery"
## Shown while the content loaded before is freed - "Load" of a loaded scenery, or "Clear"
const CLEAR_CAPTION:String = "Unloading scenery"

## The sceneries of the edited scene, by instance id - a closed scene frees them
var _include_ids:Array[int] = []


## The edited scene: the loads of its sceneries are shown
func set_scene_root(root:Node) -> void:
    for include_id:int in _include_ids:
        var include:MaszynaIncludeNode = instance_from_id(include_id) as MaszynaIncludeNode
        if include:
            include.load_progress.disconnect(_on_load_progress)
            include.load_ended.disconnect(_on_load_ended)
            include.clear_progress.disconnect(_on_clear_progress)
            include.cleared.disconnect(_on_load_ended)
    _include_ids.clear()
    visible = false
    if not root:
        return
    # the sceneries the scene itself holds; the includes of a scenery are its content
    var includes:Array[Node] = root.find_children("", "MaszynaIncludeNode", true, true)
    if root is MaszynaIncludeNode:
        includes.append(root)
    for node:Node in includes:
        var include:MaszynaIncludeNode = node as MaszynaIncludeNode
        include.load_progress.connect(_on_load_progress)
        include.load_ended.connect(_on_load_ended)
        include.clear_progress.connect(_on_clear_progress)
        # "Clear" ends with no load after it; a load shows itself again with its first step
        include.cleared.connect(_on_load_ended)
        _include_ids.append(include.get_instance_id())


func _on_load_progress(progress:float, _stage:MaszynaIncludeNode.LoadStage, message:String) -> void:
    visible = true
    %Progress.value = progress
    %Message.text = tr(message) if message else CAPTION


func _on_clear_progress(progress:float) -> void:
    visible = true
    %Progress.value = progress
    %Message.text = tr(CLEAR_CAPTION)


func _on_load_ended() -> void:
    visible = false


## "Stop": the load under way is given up (a scenery that is not loading has nothing to stop)
func _on_stop_pressed() -> void:
    for include_id:int in _include_ids:
        (instance_from_id(include_id) as MaszynaIncludeNode).stop_loading()
