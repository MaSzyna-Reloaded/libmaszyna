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
## (mesh + transform) itself, the way it streams MaszynaE3DRenderingServer's models (see TODO.md).
## @deprecated: streaming glue for legacy "triangles" chunks, to be replaced by SceneryStreamingServer.

class ChunkState:
    ## ResourceLazyLoader's MaszynaTrianglesSector, read on the streaming worker and shared by the
    ## chunks of the section
    var geometry:RID
    ## RenderingServer's meshes and their instances, one per group of the chunk
    ## (MaszynaTrianglesChunkGeometry), made from the worker's arrays as the chunk is built and freed
    ## as it is cleared
    var meshes:Array[RID] = []
    var mesh_instances:Array[RID] = []
    var transform:Transform3D
    var material_name:String
    var range_min:float
    var range_max:float
    var scenario:RID
    var stream_rid:RID
    ## MaszynaMaterialManager only keeps a weakref (material_manager.gd), so the chunk holds the material
    ## for as long as it is built - like MaszynaE3DInstanceData.materials does for models
    var material:Material

## What the streaming worker reads of a chunk for the main thread, per group
class PreloadedChunk:
    var arrays:Array[Array] = []
    ## The box Godot measures the group's visibility range to the centre of - the group's centre
    var visibility_aabbs:Array[AABB] = []
    ## The chunk's material's files, read on the worker and held until the build makes the material
    var material_files:MaszynaMaterialManager.MaterialPreload = MaszynaMaterialManager.MaterialPreload.new()

var _chunks:Dictionary[RID, ChunkState] = {}
## The sector of each chunk and the chunk's index in it, for _stream_preload() on the streaming
## worker, where _chunks is not safe to read
var _geometries:Dictionary[RID, RID] = {}
var _chunk_indices:Dictionary[RID, int] = {}
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
## `geometry_loader() -> MaszynaTrianglesSector` reads its section's file, on the streaming worker -
## registered by the file's path, so the chunks of a section read it once.
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
    _chunk_indices[rid] = chunk.chunk_index
    _geometries_mutex.unlock()
    state.stream_rid = SceneryStreamingServer.stream_register(
        _stream_owner, rid, chunk.position, chunk.range_max
    )
    return rid


## Terrain a SceneryStreamingProvider supplies: a chunk whose triangles are in memory already, and
## held by it until the provider lets the cell go - a sector of its own, with the chunk alone
func adopt_terrain(geometry:MaszynaTrianglesChunkGeometry, scenario:RID) -> RID:
    var chunk:MaszynaTrianglesChunkData = MaszynaTrianglesChunkData.new()
    # ResourceLazyLoader's key - the geometry has no file of its own
    chunk.geometry_path = "supplied://%d" % geometry.get_instance_id()
    var sector:MaszynaTrianglesSector = MaszynaTrianglesSector.new()
    var chunks:Array[MaszynaTrianglesChunkGeometry] = [geometry]
    sector.chunks = chunks
    chunk.position = geometry.origin
    chunk.material_name = geometry.texture
    chunk.range_min = geometry.range_min
    chunk.range_max = maxf(geometry.range_max, 0.0)
    return create_chunk(chunk, scenario, func() -> MaszynaTrianglesSector: return sector)


func free_chunk(rid:RID) -> void:
    var state:ChunkState = _chunks.get(rid)
    if not state:
        return
    SceneryStreamingServer.stream_free(state.stream_rid)
    _stream_clear(rid)
    _chunks.erase(rid)
    _geometries_mutex.lock()
    _geometries.erase(rid)
    _chunk_indices.erase(rid)
    _geometries_mutex.unlock()
    ResourceLazyLoader.resource_free(state.geometry)


## Streaming worker thread: the geometry read and made mesh arrays, so the main thread only makes the
## mesh of them - a chunk is at most MaszynaTrianglesChunkGeometry.MAX_VERTICES, the upload of one fits
## the frame budget
func _stream_preload(rid:RID) -> Variant:
    _geometries_mutex.lock()
    var geometry_rid:RID = _geometries.get(rid, RID())
    var chunk_index:int = _chunk_indices.get(rid, 0)
    _geometries_mutex.unlock()
    if not geometry_rid.is_valid():
        return null
    var sector:MaszynaTrianglesSector = ResourceLazyLoader.resource_load(geometry_rid)
    if not sector or chunk_index >= sector.chunks.size():
        return null
    var geometry:MaszynaTrianglesChunkGeometry = sector.chunks[chunk_index]
    var preloaded:PreloadedChunk = PreloadedChunk.new()
    for group:int in geometry.get_group_count():
        preloaded.arrays.append(geometry.get_group_mesh_arrays(group))
        preloaded.visibility_aabbs.append(geometry.get_group_visibility_aabb(group))
    # a terrain chunk draws with the project's texture size limit, as get_material() gives it
    MaszynaMaterialManager.material_preload("", geometry.texture, MaszynaMaterialManager.MAX_TEXTURE_SIZE_PROJECT_DEFAULT, preloaded.material_files)
    return preloaded


## The world the chunk is drawn in, an empty RID for none (its scenery out of the tree)
func chunk_set_scenario(rid:RID, scenario:RID) -> void:
    var state:ChunkState = _chunks.get(rid)
    state.scenario = scenario
    for mesh_instance:RID in state.mesh_instances:
        RenderingServer.instance_set_scenario(mesh_instance, scenario)


## A mesh and an instance per group: the original draws each merged shape of a cell by its own
## range, measured from its own centre (opengl33renderer.cpp:2814); a chunk of a section is one
## group without a range - the original draws those whatever their range (opengl33renderer.cpp:2495)
func _stream_build(rid:RID, preloaded:PreloadedChunk) -> void:
    var state:ChunkState = _chunks.get(rid)
    if not state or state.mesh_instances or not preloaded:
        return
    # an unresolved texture leaves nothing worth drawing, like a scenery model that fails to load
    var material:Material = MaszynaMaterialManager.get_material("", state.material_name)
    if not material:
        return
    state.material = material
    for group:int in preloaded.arrays.size():
        var mesh:RID = RenderingServer.mesh_create()
        RenderingServer.mesh_add_surface_from_arrays(mesh, RenderingServer.PRIMITIVE_TRIANGLES, preloaded.arrays[group])
        var mesh_instance:RID = RenderingServer.instance_create()
        RenderingServer.instance_set_base(mesh_instance, mesh)
        RenderingServer.instance_set_scenario(mesh_instance, state.scenario)
        RenderingServer.instance_set_transform(mesh_instance, state.transform)
        RenderingServer.instance_geometry_set_material_override(mesh_instance, material.get_rid())
        if state.range_max > 0:
            RenderingServer.instance_set_custom_aabb(mesh_instance, preloaded.visibility_aabbs[group])
            RenderingServer.instance_geometry_set_visibility_range(
                mesh_instance, state.range_min, state.range_max, 0.0, 0.0,
                RenderingServer.VISIBILITY_RANGE_FADE_DISABLED
            )
        state.meshes.append(mesh)
        state.mesh_instances.append(mesh_instance)


func _stream_clear(rid:RID) -> void:
    var state:ChunkState = _chunks.get(rid)
    if not state or not state.mesh_instances:
        return
    for mesh_instance:RID in state.mesh_instances:
        RenderingServer.free_rid(mesh_instance)
    for mesh:RID in state.meshes:
        RenderingServer.free_rid(mesh)
    state.mesh_instances.clear()
    state.meshes.clear()
    state.material = null
