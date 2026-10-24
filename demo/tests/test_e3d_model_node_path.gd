extends MaszynaGutTest


func test_get_node_or_null_resolves_nested_path() -> void:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var root: MaszynaE3DSubModel = _create_submodel("root")
    var child: MaszynaE3DSubModel = _create_submodel("child")
    var leaf: MaszynaE3DSubModel = _create_submodel("leaf")
    child.submodels = [leaf]
    root.submodels = [child]
    model.submodels = [root]

    var path: NodePath = NodePath("root/child/leaf")

    assert_same(model.get_node_or_null(path), leaf, "Nested path should resolve to matching submodel")


func test_get_node_returns_root_node() -> void:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var root: MaszynaE3DSubModel = _create_submodel("root")
    model.submodels = [root]

    var path: NodePath = NodePath("root")

    assert_same(model.get_node(path), root, "Single segment path should resolve root submodel")


func test_get_node_or_null_returns_null_for_invalid_path() -> void:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var root: MaszynaE3DSubModel = _create_submodel("root")
    model.submodels = [root]

    var path: NodePath = NodePath("root/missing")

    assert_null(model.get_node_or_null(path), "Invalid path should return null")


func test_get_node_or_null_returns_null_for_empty_path() -> void:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var path: NodePath = NodePath("")

    assert_null(model.get_node_or_null(path), "Empty path should return null")


func test_get_node_or_null_returns_null_for_duplicate_sibling_names() -> void:
    var model: MaszynaE3DModel = MaszynaE3DModel.new()
    var first_root: MaszynaE3DSubModel = _create_submodel("dup")
    var second_root: MaszynaE3DSubModel = _create_submodel("dup")
    model.submodels = [first_root, second_root]

    var path: NodePath = NodePath("dup")

    assert_null(model.get_node_or_null(path), "Duplicate sibling names should make path invalid")


func test_light_definition_stores_node_paths() -> void:
    var light: MaszynaE3DModelLightDefinition = MaszynaE3DModelLightDefinition.new()
    var on_path: NodePath = NodePath("root/on")
    var off_path: NodePath = NodePath("root/off")

    light.on_submodel_path = on_path
    light.off_submodel_path = off_path

    assert_same(light.on_submodel_path, on_path, "On path should be stored on light definition")
    assert_same(light.off_submodel_path, off_path, "Off path should be stored on light definition")


func _create_submodel(name: String) -> MaszynaE3DSubModel:
    var submodel: MaszynaE3DSubModel = MaszynaE3DSubModel.new()
    submodel.resource_name = name
    return submodel
