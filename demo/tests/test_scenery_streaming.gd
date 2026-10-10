extends MaszynaGutTest

## SceneryStreamingServer: registered placements are built only while the streaming camera is
## within their range of the chunk they fall into.

## Frames given to the streaming worker and the frame-budgeted build/clear before a check fails
const STREAMING_FRAMES:int = 60
## Real seconds to wait for the streaming to look at a moved camera - a few of its watch intervals
const CAMERA_WATCH_TIMEOUT:float = 1.0
const CHUNK_SIZE_M:float = 1000.0

var _camera:Camera3D
var _rids:Array[RID] = []
var _stream_rids:Array[RID] = []
var _build_order:Array[RID] = []
var _previous_lazy_loading:bool = false


func before_each() -> void:
    # what is loaded and let go follows the camera only with lazy loading
    _previous_lazy_loading = ResourceLazyLoader.lazy_loading
    ResourceLazyLoader.lazy_loading = true
    _camera = Camera3D.new()
    add_child_autoqfree(_camera)
    E3DRenderingServer.model_set_loader(_load_test_model)
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())


func after_each() -> void:
    for rid:RID in _rids:
        E3DRenderingServer.instance_free(rid)
    _rids.clear()
    for rid:RID in _stream_rids:
        SceneryStreamingServer.stream_free(rid)
    _stream_rids.clear()
    _build_order.clear()
    SceneryStreamingServer.streaming_set_camera(0)
    E3DRenderingServer.model_set_loader(E3DModelManager.load_model)
    ResourceLazyLoader.lazy_loading = _previous_lazy_loading


func test_registered_instance_is_built_only_within_range() -> void:
    _register(Vector3.ZERO, 200.0)

    await _move_camera(Vector3(4 * CHUNK_SIZE_M, 0, 0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "built while out of range")

    await _move_camera(Vector3.ZERO)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 1, "not built inside the range")

    await _move_camera(Vector3(4 * CHUNK_SIZE_M, 0, 0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "not cleared after leaving")


func test_model_is_held_only_while_an_instance_of_it_is_built() -> void:
    _register(Vector3.ZERO, 200.0)
    # the same key is the same resource; this registration only lets the test look at it
    var model_resource:RID = ResourceLazyLoader.resource_register("models/test/streamed", Callable())

    await _move_camera(Vector3.ZERO)
    assert_true(ResourceLazyLoader.resource_is_resident(model_resource), "built without holding its model")

    await _move_camera(Vector3(4 * CHUNK_SIZE_M, 0, 0))
    assert_false(ResourceLazyLoader.resource_is_resident(model_resource), "model held after leaving")
    ResourceLazyLoader.resource_free(model_resource)


func test_range_of_a_node_without_one_is_capped_by_the_draw_distance() -> void:
    var draw_distance:float = SceneryStreamingServer.streaming_get_draw_distance()
    _register(Vector3.ZERO, 0.0)

    await _move_camera(Vector3(draw_distance + 2 * CHUNK_SIZE_M, 0, 0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "built beyond the draw distance")

    await _move_camera(Vector3(draw_distance * 0.5, 0, 0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 1, "not built within the draw distance")


func test_registered_instance_is_freed_before_it_is_ever_built() -> void:
    var rid:RID = _register(Vector3.ZERO, 200.0)

    await _move_camera(Vector3(4 * CHUNK_SIZE_M, 0, 0))
    E3DRenderingServer.instance_free(rid)
    _rids.erase(rid)

    await _move_camera(Vector3.ZERO)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "a freed registration was built")


## Building a chunk needs a real renderer (a headless material has no shader), so this covers the
## registration side only: an out-of-range chunk stays unbuilt and frees cleanly
func test_triangle_chunk_out_of_range_is_not_built() -> void:
    var chunk := MaszynaTrianglesChunkData.new()
    chunk.geometry_path = "test/chunk_out_of_range.res"
    chunk.position = Vector3.ZERO
    chunk.range_max = 200.0
    var rid:RID = MaszynaSceneryChunkRenderingServer.create_chunk(
        chunk, get_tree().root.world_3d.scenario,
        func() -> MaszynaTrianglesChunkGeometry: return MaszynaTrianglesChunkGeometry.new()
    )

    await _move_camera(Vector3(4 * CHUNK_SIZE_M, 0, 0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "chunk built while out of range")

    MaszynaSceneryChunkRenderingServer.free_chunk(rid)
    await _move_camera(Vector3.ZERO)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "a freed chunk was built")


func test_camera_can_pause_registration_until_the_final_start_position() -> void:
    SceneryStreamingServer.streaming_set_camera(0)
    var owner:int = SceneryStreamingServer.owner_create("test", Callable(), _record_build, _record_clear)
    var menu_rid:RID = _stream_register(owner, Vector3.ZERO)
    var cabin_rid:RID = _stream_register(owner, Vector3(4 * CHUNK_SIZE_M, 0, 0))

    await wait_streaming(STREAMING_FRAMES)
    assert_eq(_build_order.size(), 0, "built scenery while streaming was paused")

    _camera.global_position = Vector3(4 * CHUNK_SIZE_M, 0, 0)
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())
    await wait_streaming(STREAMING_FRAMES)
    assert_has(_build_order, cabin_rid, "did not build around the final camera")
    assert_does_not_have(_build_order, menu_rid, "built around the stale menu camera")


func test_nearest_chunk_is_built_first_and_neighbourhood_becomes_ready() -> void:
    SceneryStreamingServer.streaming_set_camera(0)
    var owner:int = SceneryStreamingServer.owner_create("test", Callable(), _record_build, _record_clear)
    var far_rid:RID = _stream_register(owner, Vector3(CHUNK_SIZE_M, 0, 0))
    var near_rid:RID = _stream_register(owner, Vector3.ZERO)
    assert_false(SceneryStreamingServer.area_is_ready(1), "paused streaming reported ready")

    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())
    await wait_streaming(STREAMING_FRAMES)
    assert_eq(_build_order[0], near_rid, "farther chunk was built before the camera chunk")
    assert_has(_build_order, far_rid, "neighbour chunk was not built")
    assert_true(SceneryStreamingServer.area_is_ready(1), "camera neighbourhood did not become ready")


## The catch-up budget fills the area around a new camera, or one that jumped - never the world a
## train drives through
func test_the_area_is_filled_after_a_new_camera_and_a_jump_only() -> void:
    _register(Vector3.ZERO, 200.0)
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())
    assert_true(SceneryStreamingServer.streaming_get_statistics()["filling"], "a new camera is not filling")
    await _move_camera(Vector3.ZERO)
    assert_false(SceneryStreamingServer.streaming_get_statistics()["filling"], "still filling a ready area")

    # the camera is looked at every SceneryStreamingServer.WATCH_INTERVAL_SEC: the jump is seen when
    # the plan for it moves the camera's chunk - read there, as an empty area is filled at once
    var filling_at_jump:Array[bool] = []
    var on_chunk_changed:Callable = func(_chunk:Vector2i) -> void:
        filling_at_jump.append(SceneryStreamingServer.streaming_get_statistics()["filling"])
    SceneryStreamingServer.streaming_camera_chunk_changed.connect(on_chunk_changed)
    _camera.global_position = Vector3(10.0 * CHUNK_SIZE_M, 0, 0)
    await wait_for_signal(SceneryStreamingServer.streaming_camera_chunk_changed, CAMERA_WATCH_TIMEOUT)
    SceneryStreamingServer.streaming_camera_chunk_changed.disconnect(on_chunk_changed)
    assert_eq(filling_at_jump, [true] as Array[bool], "a jump is not filling")
    await _move_camera(Vector3(10.0 * CHUNK_SIZE_M + 40.0, 0, 0))
    assert_false(SceneryStreamingServer.streaming_get_statistics()["filling"], "still filling after the jump")


## A piece just past its range is built ahead, with what is left of a frame, and nothing waits for it
func test_a_piece_just_past_its_range_is_built_ahead_without_holding_the_area() -> void:
    _register(Vector3(10.0, 0, 10.0), 200.0)
    # 500 m past the chunk's edge: past the range, within the prefetch ring
    await _move_camera(Vector3(CHUNK_SIZE_M + 500.0, 0, 10.0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 1, "not built ahead")
    assert_true(SceneryStreamingServer.area_is_ready(1), "the area waited for a piece out of range")
    assert_eq(SceneryStreamingServer.streaming_get_statistics()["pending_builds"], 0)


## The anchor's chunk (the player's vehicle) is kept built wherever the camera is, follows the anchor
## into another chunk, and is let go with it
func test_the_anchor_chunk_is_kept_built_wherever_the_camera_is() -> void:
    _register(Vector3(10.0, 0, 10.0), 200.0)
    await _move_camera(Vector3(20.0 * CHUNK_SIZE_M, 0, 0))
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0)

    SceneryStreamingServer.streaming_set_anchor_position(Vector3(500.0, 0, 500.0))
    await wait_streaming(STREAMING_FRAMES)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 1, "the anchor's chunk was not built")

    # driven on by an AI into the next chunk: the one it left is no longer kept
    SceneryStreamingServer.streaming_set_anchor_position(Vector3(1500.0, 0, 500.0))
    await wait_streaming(STREAMING_FRAMES)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "the chunk left is still kept")

    SceneryStreamingServer.streaming_set_anchor_position(Vector3(500.0, 0, 500.0))
    await wait_streaming(STREAMING_FRAMES)
    SceneryStreamingServer.streaming_clear_anchor()
    await wait_streaming(STREAMING_FRAMES)
    assert_eq(SceneryStreamingServer.streaming_get_streamed_count(), 0, "kept after the anchor went")


func test_detached_camera_uses_the_last_valid_position() -> void:
    await _move_camera(Vector3(12.0, 3.0, 4.0))
    remove_child(_camera)
    assert_eq(SceneryStreamingServer.streaming_get_camera_position(), Vector3(12.0, 3.0, 4.0))
    add_child(_camera)


func test_a_chunk_lists_what_was_registered_in_it() -> void:
    var near:RID = _register(Vector3(CHUNK_SIZE_M * 0.5, 0, 0), 200.0)
    var far:RID = _register(Vector3(4.5 * CHUNK_SIZE_M, 0, 0), 200.0)

    var rids:Array[RID] = SceneryStreamingServer.chunk_get_rids(Vector2i.ZERO)
    assert_has(rids, near, "a placement missing from its chunk")
    assert_does_not_have(rids, far, "a placement listed in another chunk")
    assert_has(SceneryStreamingServer.chunk_get_rids(Vector2i(4, 0)), far)


func test_the_camera_moving_into_another_chunk_is_announced() -> void:
    watch_signals(SceneryStreamingServer)

    await _move_camera(Vector3(2.5 * CHUNK_SIZE_M, 0, 3.5 * CHUNK_SIZE_M))

    assert_signal_emitted_with_parameters(
        SceneryStreamingServer, "streaming_camera_chunk_changed", [Vector2i(2, 3)]
    )
    assert_eq(SceneryStreamingServer.streaming_get_camera_chunk(), Vector2i(2, 3))


func test_a_chunk_whose_last_piece_is_cleared_is_announced() -> void:
    _register(Vector3(CHUNK_SIZE_M * 0.5, 0, 0), 200.0)
    await _move_camera(Vector3(CHUNK_SIZE_M * 0.5, 0, 0))
    watch_signals(SceneryStreamingServer)

    await _move_camera(Vector3(4.5 * CHUNK_SIZE_M, 0, 0))

    assert_signal_emitted_with_parameters(SceneryStreamingServer, "chunk_cleared", [Vector2i.ZERO])


## E3DModelInstance.set_e3d_instance(): the node stands for an instance it does not own - a
## scenery's model ("Edit SCN" sectors)
func test_a_proxy_moves_the_instance_and_leaves_it_alive() -> void:
    var position := Vector3(CHUNK_SIZE_M * 0.5, 0, 0)
    var rid:RID = _register(position, 200.0)
    await _move_camera(position)
    var proxy := E3DModelInstance.new()
    proxy.set_e3d_instance(rid)
    add_child(proxy)

    assert_eq(proxy.transform.origin, position, "the proxy does not stand where its instance does")
    assert_eq(proxy.model_filename, "streamed")
    assert_true(proxy.submodels_aabb.has_volume(), "the proxy has no bounds of the built model")

    var moved := Vector3(CHUNK_SIZE_M * 0.5, 0, 10.0)
    proxy.global_position = moved
    await wait_idle_frames(1)
    assert_eq(E3DRenderingServer.instance_get_transform(rid).origin, moved, "the instance did not move")

    proxy.free()
    assert_eq(E3DRenderingServer.instance_get_data_path(rid), "models/test", "the instance went with its proxy")
    assert_eq(E3DRenderingServer.instance_get_transform(rid).origin, moved)


func _register(position:Vector3, range_max:float) -> RID:
    var rid:RID = E3DRenderingServer.instance_register(
        "models/test", "streamed", PackedStringArray(),
        Transform3D(Basis(), position), 0.0, range_max, get_tree().root.world_3d.scenario
    )
    _rids.append(rid)
    return rid


func _stream_register(owner:int, position:Vector3) -> RID:
    var user_rid:RID = rid_from_int64(_stream_rids.size() + 10000)
    var stream_rid:RID = SceneryStreamingServer.stream_register(owner, user_rid, position, 0.0)
    _stream_rids.append(stream_rid)
    return user_rid


func _record_build(user_rid:RID, _preloaded:Variant) -> void:
    _build_order.append(user_rid)


func _record_clear(_user_rid:RID) -> void:
    pass


## Planning runs on a worker thread and the builds are spread over frames, so a check waits for
## the streaming to settle instead of assuming it happened in one frame
func _move_camera(position:Vector3) -> void:
    _camera.global_position = position
    await wait_streaming(STREAMING_FRAMES)


func _load_test_model(_data_path:String, _filename:String) -> E3DModel:
    var mesh_submodel:E3DSubModel = E3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = E3DSubModel.SUBMODEL_GL_TRIANGLES
    mesh_submodel.mesh = _create_mesh()
    var model:E3DModel = E3DModel.new()
    model.submodels = [mesh_submodel]
    return model


func _create_mesh() -> ArrayMesh:
    var mesh:ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    return mesh
