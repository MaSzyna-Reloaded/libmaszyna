extends MaszynaGutTest

## apply_geometry() builds the submodels' ArrayMesh in the RenderingServer; MaszynaE3DModelManager
## runs it off load_model(), on a WorkerTaskQueue worker - MaszynaE3DRenderingServer's own doc
## comment says so. The RenderingServer's RID owner is only safe to touch from one thread at a
## time: many workers building a mesh at once corrupted it and crashed the engine within a second
## of real play (reports#91, dynamic/cd/181_v2's skins loading together).

const CONCURRENT_MODELS: int = 32
## A real vehicle's model has many submodels, each its own mesh_create() call inside one
## apply_geometry() - a single submodel finishes too fast for two threads to ever overlap in it
const SUBMODELS_PER_MODEL: int = 60
## apply_geometry() off the main thread blocks on the main thread's own deferred call queue, which
## only drains between real frames - a real frame wait, never thread.wait_to_finish() straight off
## the test's own thread, which would block the very queue the workers wait on
const MAX_FRAMES_TO_WAIT: int = 300


func test_apply_geometry_from_many_threads_at_once_does_not_corrupt_the_renderer() -> void:
    var threads: Array[Thread] = []
    var models: Array[MaszynaE3DModel] = []
    var submodels: Array[Array] = []
    for i in range(CONCURRENT_MODELS):
        var surfaces: Array[Dictionary] = []
        var model_submodels: Array[MaszynaE3DSubModel] = []
        for j in range(SUBMODELS_PER_MODEL):
            surfaces.append({
                "vertices": PackedFloat32Array([0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0, 0.0]),
                "uvs": PackedFloat32Array([0.0, 0.0, 1.0, 0.0, 0.0, 1.0]),
            })
            var submodel := MaszynaE3DSubModel.new()
            submodel.geometry_index = j
            model_submodels.append(submodel)
        var geometry := MaszynaE3DModelGeometry.new()
        geometry.surfaces = surfaces
        var model := MaszynaE3DModel.new()
        model.submodels = model_submodels
        models.append(model)
        submodels.append(model_submodels)
        var thread := Thread.new()
        thread.start(model.apply_geometry.bind(geometry))
        threads.append(thread)

    var frames_waited := 0
    while threads.any(func(thread: Thread) -> bool: return thread.is_alive()):
        frames_waited += 1
        if frames_waited > MAX_FRAMES_TO_WAIT:
            fail_test("a thread never finished apply_geometry() - the main thread's deferred queue never caught up")
            break
        await get_tree().process_frame
    for thread: Thread in threads:
        thread.wait_to_finish()

    for i in range(CONCURRENT_MODELS):
        assert_true(models[i].has_geometry(), "model %d has geometry" % i)
        for j in range(SUBMODELS_PER_MODEL):
            var submodel: MaszynaE3DSubModel = submodels[i][j]
            assert_not_null(submodel.mesh, "model %d submodel %d made a mesh" % [i, j])
