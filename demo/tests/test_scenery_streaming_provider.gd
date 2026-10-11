extends MaszynaGutTest

## SceneryStreamingServer's providers: a provider's cell is asked for as the camera comes within the
## draw distance of it, its content handed to the consumer of its kind, and let go once the camera
## has left or the provider is freed.

## Frames given to the streaming worker and the frame-budgeted apply before a check fails
const STREAMING_FRAMES:int = 60
const CHUNK_SIZE_M:float = 1000.0
## Beyond the draw distance (SceneryStreamingServer.DEFAULT_DRAW_DISTANCE_M) and its hysteresis
const FAR_AWAY:Vector3 = Vector3(20.0 * CHUNK_SIZE_M, 0.0, 0.0)


class TestProvider extends SceneryStreamingProvider:
    var cells:Array[Vector2i] = [Vector2i.ZERO]
    var kinds:PackedInt32Array = [SceneryStreamingProvider.CONTENT_TERRAIN]
    var items:Dictionary[int, Array] = {}
    var asked_kinds:Array[int] = []
    var _mutex:Mutex = Mutex.new()

    func _get_chunk_cells() -> Array[Vector2i]:
        return cells

    func _chunk_get_overhang(_cell:Vector2i) -> float:
        return 0.0

    func _get_content_kinds() -> PackedInt32Array:
        return kinds

    ## On the streaming worker
    func _chunk_load(_cell:Vector2i, kind:int) -> Array:
        _mutex.lock()
        asked_kinds.append(kind)
        _mutex.unlock()
        return items.get(kind, [])


var _camera:Camera3D
var _providers:Array[RID] = []
var _adopted:Array[Variant] = []
var _released:Array[RID] = []


func before_each() -> void:
    _camera = Camera3D.new()
    add_child_autoqfree(_camera)
    _camera.global_position = FAR_AWAY
    SceneryStreamingServer.content_set_consumer(SceneryStreamingProvider.CONTENT_TERRAIN, _adopt, _release)
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())


func after_each() -> void:
    for provider:RID in _providers:
        SceneryStreamingServer.provider_free(provider)
    _providers.clear()
    _adopted.clear()
    _released.clear()
    SceneryStreamingServer.streaming_set_camera(0)
    SceneryStreamingServer.content_set_consumer(
        SceneryStreamingProvider.CONTENT_TERRAIN, MaszynaSceneryChunkRenderingServer.adopt_terrain,
        MaszynaSceneryChunkRenderingServer.free_chunk)


func test_a_cell_is_supplied_within_the_draw_distance_and_let_go_beyond_it() -> void:
    var provider:TestProvider = _terrain_provider()
    await _move_camera(FAR_AWAY)
    assert_eq(_adopted.size(), 0, "supplied while out of reach")

    await _move_camera(Vector3.ZERO)
    assert_eq(_adopted.size(), 1, "not supplied in reach")
    assert_same(_adopted[0], provider.items[SceneryStreamingProvider.CONTENT_TERRAIN][0])

    await _move_camera(FAR_AWAY)
    assert_eq(_released.size(), 1, "not let go after the camera left")


func test_freeing_the_provider_lets_go_of_what_it_supplied() -> void:
    _terrain_provider()
    await _move_camera(Vector3.ZERO)
    assert_eq(_adopted.size(), 1)

    SceneryStreamingServer.provider_free(_providers.pop_back())
    assert_eq(_released.size(), 1, "supplied content outlived its provider")


func test_a_kind_the_provider_does_not_supply_is_never_asked_for() -> void:
    var provider:TestProvider = _terrain_provider()
    await _move_camera(Vector3.ZERO)
    assert_eq(provider.asked_kinds, [SceneryStreamingProvider.CONTENT_TERRAIN] as Array[int])


func test_the_area_is_ready_once_the_cells_around_the_camera_are_supplied() -> void:
    _terrain_provider()
    await _move_camera(Vector3.ZERO)
    assert_true(SceneryStreamingServer.area_is_ready(1))
    assert_eq(SceneryStreamingServer.streaming_get_statistics()["supplied_cells"], 1)


func test_a_supplied_model_is_drawn_by_the_model_consumer() -> void:
    E3DRenderingServer.model_set_loader(_load_test_model)
    var placement:SceneryModelPlacement = SceneryModelPlacement.new()
    placement.data_path = "models/test"
    placement.model_filename = "supplied"
    placement.transform = Transform3D(Basis(), Vector3(10.0, 0.0, 10.0))
    var provider:TestProvider = TestProvider.new()
    provider.kinds = [SceneryStreamingProvider.CONTENT_MODELS]
    var models:Array = [placement]
    provider.items[SceneryStreamingProvider.CONTENT_MODELS] = models
    _providers.append(SceneryStreamingServer.provider_register(provider, get_tree().root.world_3d.scenario))

    await _move_camera(Vector3.ZERO)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 1, "the supplied model was not built")
    SceneryStreamingServer.provider_free(_providers.pop_back())
    await wait_streaming(STREAMING_FRAMES)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "the model outlived its provider")
    E3DRenderingServer.model_set_loader(E3DModelManager.load_model)


func _terrain_provider() -> TestProvider:
    var provider:TestProvider = TestProvider.new()
    var geometry:MaszynaTrianglesChunkGeometry = MaszynaTrianglesChunkGeometry.new()
    var terrain:Array = [geometry]
    provider.items[SceneryStreamingProvider.CONTENT_TERRAIN] = terrain
    _providers.append(SceneryStreamingServer.provider_register(provider, get_tree().root.world_3d.scenario))
    return provider


func _adopt(item:Variant, _scenario:RID) -> RID:
    _adopted.append(item)
    return rid_from_int64(_adopted.size() + 20000)


func _release(rid:RID) -> void:
    _released.append(rid)


## Planning runs on a worker thread and the apply is spread over frames, so a check waits for the
## streaming to settle instead of assuming it happened in one frame
func _move_camera(position:Vector3) -> void:
    _camera.global_position = position
    await wait_streaming(STREAMING_FRAMES)


func _load_test_model(_data_path:String, _filename:String) -> E3DModel:
    var mesh_submodel:E3DSubModel = E3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    var mesh:ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    mesh_submodel.mesh = mesh
    var model:E3DModel = E3DModel.new()
    model.submodels = [mesh_submodel]
    return model
