extends MaszynaGutTest

## A low-poly interior with a passenger section gets the lights the original lights it by
## self-illumination alone (#333, MaszynaE3DLightFactory::make_interior_lights()): they light only
## the interior's layer, so the car lights no track under it.

const INTERIOR_LIGHT:String = "interior"


func test_an_interior_with_a_section_has_its_lights_and_stands_on_its_layer() -> void:
    var parent:Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid:RID = _build(parent, "przedzial01")

    assert_true(ModelRenderingServer.instance_has_light(rid, INTERIOR_LIGHT))
    var mesh:MeshInstance3D = parent.get_node(NodePath("przedzial01/mesh"))
    assert_eq(mesh.layers, MaszynaE3DRenderingServer.VEHICLE_INTERIOR_LAYER,
            "the interior stands on the one layer its lights light")
    MaszynaE3DRenderingServer.instance_free(rid)


func test_a_model_with_no_section_gets_no_interior_lights() -> void:
    var parent:Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid:RID = _build(parent, "pudlo")

    assert_false(ModelRenderingServer.instance_has_light(rid, INTERIOR_LIGHT))
    var mesh:MeshInstance3D = parent.get_node(NodePath("pudlo/mesh"))
    assert_false(bool(mesh.layers & MaszynaE3DRenderingServer.VEHICLE_INTERIOR_LAYER))
    MaszynaE3DRenderingServer.instance_free(rid)


## A part of the name given with a mesh under it
func _build(parent:Node3D, part_name:String) -> RID:
    var model:MaszynaE3DModel = MaszynaE3DModel.new()
    var part:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    part.resource_name = part_name
    part.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    var mesh_submodel:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    var mesh:ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    mesh_submodel.mesh = mesh
    part.submodels = [mesh_submodel]
    model.submodels = [part]
    var rid:RID = MaszynaE3DRenderingServer.instance_create(
            model, ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_DYNAMIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)
    return rid
