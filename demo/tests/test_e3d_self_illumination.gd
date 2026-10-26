extends MaszynaGutTest

## A submodel self-illuminates while the light outside is below its threshold, as the original
## decides it for every submodel (opengl33renderer.cpp:3452 - Global.fLuminance < fLight, #334):
## its material self-illuminates and carries the threshold the shaders compare the light level
## with; SetSelfIllum() sets one over a whole subtree (Model3d.cpp, DynObj.cpp:2452).

## A threshold lit only once it gets dark, one never lit, and one always lit (Model3d.cpp:346-350)
const DUSK_THRESHOLD:float = 0.5
const NEVER_LIT:float = -1.0
const ALWAYS_LIT:float = 2.0
## No threshold of the client's: the submodel's own (MaszynaE3DInstanceData::SubmodelSettings)
const OWN_THRESHOLD:float = -2.0
const SECTION:String = "przedzial01"


func test_a_submodel_lit_once_it_gets_dark_self_illuminates_below_its_threshold() -> void:
    var parent:Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid:RID = _build(parent, DUSK_THRESHOLD)

    var mesh:MeshInstance3D = parent.get_node(NodePath(SECTION + "/mesh"))
    var material:ShaderMaterial = mesh.material_override as ShaderMaterial
    assert_not_null(material)
    if material:
        assert_true(material.get_shader_parameter("emission_enabled"), "a threshold under 1 lights it too")
    assert_almost_eq(float((mesh.material_override as ShaderMaterial).get_shader_parameter("lights_on_threshold")), DUSK_THRESHOLD, 0.001)
    MaszynaE3DRenderingServer.instance_free(rid)


func test_a_threshold_set_on_a_part_lights_everything_under_it_and_goes() -> void:
    var parent:Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid:RID = _build(parent, DUSK_THRESHOLD)

    MaszynaE3DRenderingServer.instance_set_part_lights_on_threshold(rid, SECTION, ALWAYS_LIT)
    var mesh:MeshInstance3D = parent.get_node(NodePath(SECTION + "/mesh"))
    assert_almost_eq(float((mesh.material_override as ShaderMaterial).get_shader_parameter("lights_on_threshold")), ALWAYS_LIT, 0.001,
            "SetSelfIllum(2.0) on the section reaches its children")

    MaszynaE3DRenderingServer.instance_set_part_lights_on_threshold(rid, SECTION, OWN_THRESHOLD)
    mesh = parent.get_node(NodePath(SECTION + "/mesh"))
    assert_almost_eq(float((mesh.material_override as ShaderMaterial).get_shader_parameter("lights_on_threshold")), DUSK_THRESHOLD, 0.001,
            "its own again")
    MaszynaE3DRenderingServer.instance_free(rid)


func test_a_submodel_never_lit_gets_no_self_illumination() -> void:
    var parent:Node3D = Node3D.new()
    add_child_autoqfree(parent)
    var rid:RID = _build(parent, NEVER_LIT)

    var mesh:MeshInstance3D = parent.get_node(NodePath(SECTION + "/mesh"))
    var material:ShaderMaterial = mesh.material_override as ShaderMaterial
    if material:
        assert_false(material.get_shader_parameter("emission_enabled"))
    MaszynaE3DRenderingServer.instance_free(rid)


## A section with a self-illuminated mesh under it, of the threshold given
func _build(parent:Node3D, threshold:float) -> RID:
    var model:MaszynaE3DModel = MaszynaE3DModel.new()
    var section:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    section.resource_name = SECTION
    section.submodel_type = MaszynaE3DSubModel.SUBMODEL_TRANSFORM
    var mesh_submodel:MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    mesh_submodel.resource_name = "mesh"
    mesh_submodel.submodel_type = MaszynaE3DSubModel.SUBMODEL_GL_TRIANGLES
    mesh_submodel.material_name = SECTION
    mesh_submodel.self_illumination = Color(1.0, 1.0, 1.0, 1.0)
    mesh_submodel.lights_on_threshold = threshold
    var mesh:ArrayMesh = ArrayMesh.new()
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, BoxMesh.new().get_mesh_arrays())
    mesh_submodel.mesh = mesh
    section.submodels = [mesh_submodel]
    model.submodels = [section]
    var rid:RID = MaszynaE3DRenderingServer.instance_create(
            model, ModelRenderingServer.INSTANCER_NODES, ModelRenderingServer.INSTANCE_KIND_STATIC)
    MaszynaE3DRenderingServer.instance_attach_object_instance_id(rid, parent.get_instance_id())
    MaszynaE3DRenderingServer.instance_build(rid)
    return rid
