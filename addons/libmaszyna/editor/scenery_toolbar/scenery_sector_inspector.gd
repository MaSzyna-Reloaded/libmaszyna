@tool
extends RefCounted

## "Edit SCN": the models of the chunk the streaming camera is in stand under the scenery as a
## "Sector X,Y" node of E3DModelInstance proxies (E3DModelInstance.set_e3d_instance()) - selected
## (by a click too, scenery_model_gizmo_plugin.gd), moved and shown as nodes ("Edit E3D") in place
## of the scenery's own instances. A sector goes
## when the streaming clears its chunk, and every one of them before the scene is saved: they are a
## view of the scenery, not a part of the scene. One scenery is inspected at a time.

## A sector is named after its chunk (SceneryStreamingServer)
const SECTOR_NAME:String = "Sector %d,%d"

var _scenery:MaszynaIncludeNode = null
var _sectors:Dictionary[Vector2i, Node3D] = {}
## Every proxy of the sectors, by the model it stands for
var _proxies:Dictionary[RID, E3DModelInstance] = {}


## Inspects the scenery, and nothing else from now on: the sector of the camera's chunk is shown
func inspect(scenery:MaszynaIncludeNode) -> void:
    stop()
    _scenery = scenery
    SceneryStreamingServer.streaming_camera_chunk_changed.connect(show_sector)
    SceneryStreamingServer.chunk_cleared.connect(free_sector)
    E3DRenderingServer.instance_built.connect(_on_instance_built)
    # the proxies are given back while the instances they stand for still exist
    _scenery.unloading.connect(clear)
    # another scene's tab, or the scene closed - and, out of the tree, no longer in a parent's way
    _scenery.tree_exited.connect(stop)
    show_camera_sector()


## Ends the inspection: the sectors go
func stop() -> void:
    if not _scenery:
        return
    SceneryStreamingServer.streaming_camera_chunk_changed.disconnect(show_sector)
    SceneryStreamingServer.chunk_cleared.disconnect(free_sector)
    E3DRenderingServer.instance_built.disconnect(_on_instance_built)
    _scenery.unloading.disconnect(clear)
    _scenery.tree_exited.disconnect(stop)
    clear()
    _scenery = null


## The sector of the camera's chunk, while a scenery is inspected - again after the scene was saved
## without its sectors
func show_camera_sector() -> void:
    if _scenery:
        show_sector(SceneryStreamingServer.streaming_get_camera_chunk())


## The scenery's models registered in the chunk, as proxies under a sector node of the scenery
func show_sector(chunk:Vector2i) -> void:
    if _sectors.has(chunk):
        return
    var models:Dictionary[RID, bool] = {}
    for model:RID in _scenery.get_models():
        models[model] = true
    var scene_root:Node = EditorInterface.get_edited_scene_root()
    var sector := Node3D.new()
    sector.name = SECTOR_NAME % [chunk.x, chunk.y]
    _scenery.add_child(sector)
    sector.owner = scene_root
    for model:RID in SceneryStreamingServer.chunk_get_rids(chunk):
        if not models.has(model):
            continue
        var proxy := E3DModelInstance.new()
        proxy.instancer = E3DModelInstance.Instancer.OPTIMIZED
        proxy.set_e3d_instance(model)
        proxy.name = proxy.model_filename
        # a proxy before it enters the tree, where the editor asks for its gizmo (has_proxy())
        _proxies[model] = proxy
        sector.add_child(proxy, true)
        proxy.owner = scene_root
    _sectors[chunk] = sector


## The sector of the chunk goes, if there is one; its models stay the scenery's
func free_sector(chunk:Vector2i) -> void:
    var sector:Node3D = _sectors.get(chunk)
    if not sector:
        return
    _sectors.erase(chunk)
    for proxy:Node in sector.get_children():
        _proxies.erase((proxy as E3DModelInstance).get_e3d_instance())
    # a sector the operator deleted is kept by the editor's undo history
    if sector.get_parent():
        sector.free()


## Every sector goes
func clear() -> void:
    for chunk:Vector2i in _sectors.keys():
        free_sector(chunk)


## Whether the node is a proxy of a sector
func has_proxy(node:E3DModelInstance) -> bool:
    return _proxies.get(node.get_e3d_instance()) == node


## A model streamed in after its sector was shown: its proxy takes the built model (bounds, nodes)
func _on_instance_built(instance:RID) -> void:
    var proxy:E3DModelInstance = _proxies.get(instance)
    if proxy:
        proxy.reload()
