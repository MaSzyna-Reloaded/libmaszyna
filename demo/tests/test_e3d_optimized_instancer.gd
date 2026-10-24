extends MaszynaGutTest

## MaszynaE3DModelInstance with the OPTIMIZED instancer (scenery models) renders through
## MaszynaE3DRenderingServer and must not create any child nodes.


func test_optimized_instance_loads_without_child_nodes() -> void:
    var instance: MaszynaE3DModelInstance = _create_optimized_instance()
    add_child_autoqfree(instance)

    assert_true(instance.is_e3d_loaded())
    assert_eq(instance.get_child_count(true), 0)


func test_optimized_instance_survives_tree_reentry_and_light_changes() -> void:
    var instance: MaszynaE3DModelInstance = _create_optimized_instance()
    add_child_autoqfree(instance)

    remove_child(instance)
    # A parent removal can deliver a transform notification while the node has no global transform.
    instance.position = Vector3(5, 0, 0)
    add_child(instance)
    instance.lights_state = {"00": true}
    instance.position = Vector3(10, 0, 0)
    instance.visible = false

    assert_true(instance.is_e3d_loaded())
    assert_eq(instance.get_child_count(true), 0)
    assert_true(instance.lights_state["00"])


func _create_optimized_instance() -> MaszynaE3DModelInstance:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var light_on: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    light_on.resource_name = "light_on00"
    light_on.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    light_on.visible = false
    var mesh_submodel: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    mesh_submodel.mesh = _create_array_mesh()
    light_on.submodels = [mesh_submodel]
    model.submodels = [light_on]
    var light_definition: MaszynaE3DModelLightDefinition = MaszynaE3DModelLightDefinition.new()
    light_definition.on_submodel_path = NodePath("light_on00")
    model.register_light("00", light_definition)

    var instance: MaszynaE3DModelInstance = MaszynaE3DModelInstance.new()
    instance.instancer = MaszynaE3DModelInstance.Instancer.OPTIMIZED
    instance.model = model
    return instance


func _create_array_mesh() -> ArrayMesh:
    var mesh: ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    return mesh
