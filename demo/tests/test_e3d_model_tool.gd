extends MaszynaGutTest

var tool: Node


func before_each() -> void:
    tool = load("res://addons/libmaszyna/legacy/e3d/e3d_model_tool.gd").new()
    add_child_autoqfree(tool)


func test_returns_first_root_mesh() -> void:
    var model: E3DModel = E3DModel.new()
    var first_mesh: ArrayMesh = ArrayMesh.new()
    var second_mesh: ArrayMesh = ArrayMesh.new()

    model.submodels = [
        _create_submodel(first_mesh),
        _create_submodel(second_mesh),
    ]

    assert_same(tool.find_first_mesh(model), first_mesh, "First root mesh should be returned")


func test_returns_nested_mesh_when_root_has_no_mesh() -> void:
    var model: E3DModel = E3DModel.new()
    var nested_mesh: ArrayMesh = ArrayMesh.new()
    var root: E3DSubModel = _create_submodel()
    root.submodels = [_create_submodel(nested_mesh)]
    model.submodels = [root]

    assert_same(tool.find_first_mesh(model), nested_mesh, "Nested mesh should be returned")


func test_returns_null_when_model_has_no_mesh() -> void:
    var model: E3DModel = E3DModel.new()
    var root: E3DSubModel = _create_submodel()
    root.submodels = [_create_submodel()]
    model.submodels = [root]

    assert_null(tool.find_first_mesh(model), "Model without meshes should return null")


func test_uses_depth_first_preorder() -> void:
    var model: E3DModel = E3DModel.new()
    var nested_mesh: ArrayMesh = ArrayMesh.new()
    var later_root_mesh: ArrayMesh = ArrayMesh.new()
    var first_root: E3DSubModel = _create_submodel()
    first_root.submodels = [_create_submodel(nested_mesh)]
    model.submodels = [
        first_root,
        _create_submodel(later_root_mesh),
    ]

    assert_same(tool.find_first_mesh(model), nested_mesh, "Nested mesh from first branch should win")


func _create_submodel(mesh: ArrayMesh = null) -> E3DSubModel:
    var submodel: E3DSubModel = E3DSubModel.new()
    submodel.mesh = mesh
    return submodel
