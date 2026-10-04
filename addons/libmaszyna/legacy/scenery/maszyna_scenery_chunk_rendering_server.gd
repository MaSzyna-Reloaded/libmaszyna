@tool
extends Node

## RID based renderer of merged scenery triangle meshes (MaszynaTrianglesChunkData). A real
## scenery merges tens of thousands of "triangles" nodes into a few thousand chunks, and 40% of
## them declare no visibility range at all - drawn from anywhere on the map they alone cost more
## than the rest of the scenery. Chunks are registered with SceneryStreamingServer and their
## RenderingServer instance exists only while the camera is within range of their chunk.
##
## The triangles are read from the cache only while the chunk is built (ResourceLazyLoader), and
## its mesh exists only as long.
##
## FIXME: not a server - only a SceneryStreamingServer owner that turns one legacy data type
## into RenderingServer instances. To go once SceneryStreamingServer streams a triangle chunk
## (mesh + transform) itself, the way it streams E3DRenderingServer's models (see TODO.md).
## @deprecated: streaming glue for legacy "triangles" chunks, to be replaced by SceneryStreamingServer.

class ChunkState:
    ## ResourceLazyLoader's MaszynaTrianglesChunkGeometry, read on the streaming worker
    var geometry:RID
    ## RenderingServer's mesh, made from the worker's arrays as the chunk is built and freed as it is
    ## cleared
    var mesh:RID
    var transform:Transform3D
    var material_name:String
    var range_min:float
    var range_max:float
    var scenario:RID
    var stream_rid:RID
    var mesh_instance:RID
    ## MaterialManager only keeps a weakref (material_manager.gd), so the chunk holds the material
    ## for as long as it is built - like E3DInstanceData.materials does for models
    var material:Material

var _chunks:Dictionary[RID, ChunkState] = {}
## The geometry of each chunk, for _stream_preload() on the streaming worker, where _chunks is not
## safe to read
var _geometries:Dictionary[RID, RID] = {}
var _geometries_mutex:Mutex = Mutex.new()
var _next_id:int = 0
var _stream_owner:int = -1


func _ready() -> void:
    GameDataServer.data_reload_requested.connect(_on_data_reload_requested)
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_TERRAIN, adopt_terrain, free_chunk)


## A chunk takes its material again as it is streamed again
func _on_data_reload_requested() -> void:
    if _stream_owner >= 0:
        SceneryStreamingServer.owner_rebuild(_stream_owner)


## Registers a chunk for streaming; nothing is rendered until the camera comes within its range.
## `geometry_loader() -> MaszynaTrianglesChunkGeometry` reads its triangles, on the streaming worker.
func create_chunk(chunk:MaszynaTrianglesChunkData, scenario:RID, geometry_loader:Callable) -> RID:
    if _stream_owner < 0:
        _stream_owner = SceneryStreamingServer.owner_create("terrain", _stream_preload, _stream_build, _stream_clear)

    var state := ChunkState.new()
    state.geometry = ResourceLazyLoader.resource_register(chunk.geometry_path, geometry_loader)
    state.transform = Transform3D(Basis(), chunk.position)
    state.material_name = chunk.material_name
    state.range_min = chunk.range_min
    state.range_max = chunk.range_max
    state.scenario = scenario

    _next_id += 1
    var rid:RID = rid_from_int64(_next_id)
    _chunks[rid] = state
    _geometries_mutex.lock()
    _geometries[rid] = state.geometry
    _geometries_mutex.unlock()
    state.stream_rid = SceneryStreamingServer.stream_register(
        _stream_owner, rid, chunk.position, chunk.range_max
    )
    return rid


## Terrain a SceneryStreamingProvider supplies: a chunk whose triangles are in memory already, and
## held by it until the provider lets the cell go
func adopt_terrain(geometry:MaszynaTrianglesChunkGeometry, scenario:RID) -> RID:
    var chunk:MaszynaTrianglesChunkData = MaszynaTrianglesChunkData.new()
    # ResourceLazyLoader's key - the geometry has no file of its own
    chunk.geometry_path = "supplied://%d" % geometry.get_instance_id()
    chunk.position = SceneryTrianglesSink.cell_get_origin(geometry.cell)
    chunk.material_name = geometry.texture
    chunk.range_min = geometry.range_min
    chunk.range_max = maxf(geometry.range_max, 0.0)
    return create_chunk(chunk, scenario, func() -> MaszynaTrianglesChunkGeometry: return geometry)


func free_chunk(rid:RID) -> void:
    var state:ChunkState = _chunks.get(rid)
    if not state:
        return
    SceneryStreamingServer.stream_free(state.stream_rid)
    _stream_clear(rid)
    _chunks.erase(rid)
    _geometries_mutex.lock()
    _geometries.erase(rid)
    _geometries_mutex.unlock()
    ResourceLazyLoader.resource_free(state.geometry)


## Streaming worker thread: the geometry read and made mesh arrays, so the main thread only makes the
## mesh of them - a chunk is at most MaszynaTrianglesChunkGeometry.MAX_VERTICES, the upload of one fits
## the frame budget
func _stream_preload(rid:RID) -> Variant:
    _geometries_mutex.lock()
    var geometry_rid:RID = _geometries.get(rid, RID())
    _geometries_mutex.unlock()
    if not geometry_rid.is_valid():
        return null
    var geometry:MaszynaTrianglesChunkGeometry = ResourceLazyLoader.resource_load(geometry_rid)
    if not geometry:
        return null
    return geometry.to_mesh_arrays()


## The world the chunk is drawn in, an empty RID for none (its scenery out of the tree)
func chunk_set_scenario(rid:RID, scenario:RID) -> void:
    var state:ChunkState = _chunks.get(rid)
    state.scenario = scenario
    if state.mesh_instance.is_valid():
        RenderingServer.instance_set_scenario(state.mesh_instance, scenario)


func _stream_build(rid:RID, preloaded:Variant) -> void:
    var state:ChunkState = _chunks.get(rid)
    if not state or state.mesh_instance.is_valid() or not preloaded:
        return
    # an unresolved texture leaves nothing worth drawing, like a scenery model that fails to load
    var material:Material = MaterialManager.get_material("", state.material_name)
    if not material:
        return
    state.mesh = RenderingServer.mesh_create()
    RenderingServer.mesh_add_surface_from_arrays(state.mesh, RenderingServer.PRIMITIVE_TRIANGLES, preloaded)
    state.mesh_instance = RenderingServer.instance_create()
    RenderingServer.instance_set_base(state.mesh_instance, state.mesh)
    RenderingServer.instance_set_scenario(state.mesh_instance, state.scenario)
    RenderingServer.instance_set_transform(state.mesh_instance, state.transform)
    state.material = material
    RenderingServer.instance_geometry_set_material_override(state.mesh_instance, material.get_rid())
    # the chunk's own range still culls it inside the streamed area, as it did on the node
    if state.range_max > 0:
        RenderingServer.instance_geometry_set_visibility_range(
            state.mesh_instance, state.range_min, state.range_max, 0.0, 0.0,
            RenderingServer.VISIBILITY_RANGE_FADE_DISABLED
        )


func _stream_clear(rid:RID) -> void:
    var state:ChunkState = _chunks.get(rid)
    if not state or not state.mesh_instance.is_valid():
        return
    RenderingServer.free_rid(state.mesh_instance)
    state.mesh_instance = RID()
    state.material = null
    RenderingServer.free_rid(state.mesh)
    state.mesh = RID()
