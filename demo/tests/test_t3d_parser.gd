extends MaszynaGutTest

## A text model (.t3d) is read into the same E3DModel as a binary one, turned into the scenery frame
## the way the original does it before saving an .e3d (TSubModel::InitialRotate(), Model3d.cpp:818),
## and found by E3DModelManager when there is no .e3d next to it (MdlMngr.cpp:144).

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const DATA_PATH:String = "models/t3d"
## 3ds Max to scenery: X negated, Y and Z swapped (float4x4::InitialRotate(), Float3d.h:238)
const SCENERY_FRAME:Basis = Basis(Vector3(-1, 0, 0), Vector3(0, 0, 1), Vector3(0, 1, 0))
const HALF_SQRT_2:float = 0.70710678
## Mesh normals are stored compressed
const TOLERANCE:float = 0.001

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func _vertices(submodel:E3DSubModel) -> PackedVector3Array:
    return submodel.mesh.surface_get_arrays(0)[Mesh.ARRAY_VERTEX]


func _normals(submodel:E3DSubModel) -> PackedVector3Array:
    return submodel.mesh.surface_get_arrays(0)[Mesh.ARRAY_NORMAL]


func _assert_vectors(actual:PackedVector3Array, expected:PackedVector3Array, message:String) -> void:
    assert_eq(actual.size(), expected.size(), message + " (count)")
    for i:int in mini(actual.size(), expected.size()):
        assert_lt(actual[i].distance_to(expected[i]), TOLERANCE, "%s [%d]: %s, expected %s" % [message, i, actual[i], expected[i]])


func test_text_model_is_loaded_when_there_is_no_binary_one() -> void:
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")

    assert_not_null(model, "legacy.t3d should load as an E3DModel")


func test_roots_come_in_the_order_of_the_main_chain() -> void:
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")

    # each new root goes first in the chain (TModel3d::AddTo(), Model3d.cpp:1550)
    assert_eq(model.submodels.size(), 2)
    assert_eq(model.submodels[0].get_name(), "Reflektor")
    assert_eq(model.submodels[1].get_name(), "Body")


func test_geometry_without_transform_is_turned_into_the_scenery_frame() -> void:
    # the model is held: freeing it clears its submodels
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")
    var body:E3DSubModel = model.get_node(NodePath("Body"))

    assert_eq(body.transform, Transform3D(), "the transform stays identity")
    assert_true(body.material_colored, "map: none is the diffuse colour")
    assert_almost_eq(body.visibility_range_begin, 10.0, 0.001)
    assert_almost_eq(body.visibility_range_end, 500.0, 0.001)
    # the third, degenerate triangle is dropped (Model3d.cpp:621)
    _assert_vectors(_vertices(body), PackedVector3Array([
        Vector3(0, 0, 0), Vector3(-1, 0, 0), Vector3(0, 0, 1),
        Vector3(0, 0, 0), Vector3(0, 0, 1), Vector3(0, 1, 0),
    ]), "Body vertices")


func test_normals_are_smoothed_within_a_smoothing_group() -> void:
    # the model is held: freeing it clears its submodels
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")
    var body:E3DSubModel = model.get_node(NodePath("Body"))

    # the two faces share an edge and mask 1: its vertices take the sum of both face normals
    var shared:Vector3 = Vector3(-HALF_SQRT_2, HALF_SQRT_2, 0)
    _assert_vectors(_normals(body), PackedVector3Array([
        shared, Vector3(0, 1, 0), shared,
        shared, shared, Vector3(-1, 0, 0),
    ]), "Body normals")


func test_child_with_own_transform_is_turned_by_the_transform() -> void:
    # the model is held: freeing it clears its submodels
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")
    var door:E3DSubModel = model.get_node(NodePath("Body/Door"))

    # the parent name is matched case-insensitively (TSubModel::GetFromName(), Model3d.cpp:1092)
    assert_not_null(door)
    assert_eq(door.transform.basis, SCENERY_FRAME)
    assert_true(door.transform.origin.is_equal_approx(Vector3(-5, 3, 2)))
    assert_true(door.material_transparent, "opacity 0 is drawn translucent")
    assert_eq(door.material_name, "tex/door")
    _assert_vectors(_vertices(door), PackedVector3Array([
        Vector3(1, 0, 0), Vector3(0, 1, 0), Vector3(0, 0, 1),
    ]), "Door vertices")


func test_children_of_a_transformed_submodel_stay_as_they_are() -> void:
    # the model is held: freeing it clears its submodels
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")
    var handle:E3DSubModel = model.get_node(NodePath("Body/Door/Handle"))

    assert_eq(handle.transform, Transform3D())
    assert_almost_eq(handle.visibility_range_end, 15000.0, 0.001, "no max distance means 15 km")
    _assert_vectors(_normals(handle), PackedVector3Array([
        Vector3(0, 0, 1), Vector3(0, 0, 1), Vector3(0, 0, 1),
    ]), "explicit Handle normals")


func test_free_spotlight_parameters() -> void:
    # the model is held: freeing it clears its submodels
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "legacy")
    var light:E3DSubModel = model.get_node(NodePath("Reflektor"))

    assert_eq(light.submodel_type, E3DSubModel.SUBMODEL_FREE_SPOTLIGHT)
    assert_eq(light.transform.basis, SCENERY_FRAME, "a light is turned by its transform")
    assert_almost_eq(light.light_range, 50.0, 0.001)
    # cone angles in degrees are full angles (Model3d.cpp:382-389)
    assert_almost_eq(light.light_angle, 30.0, 0.01)
    assert_almost_eq(light.cos_hotspot_angle, cos(deg_to_rad(20.0)), 0.001)
    assert_almost_eq(light.visibility_range_end, 100.0, 0.001)
    assert_true(light.diffuse_color.is_equal_approx(Color(1, 0.2, 0)))


func test_indexed_geometry_with_animation_keeps_its_vertices() -> void:
    # the model is held: freeing it clears its submodels
    var model:E3DModel = E3DModelManager.load_model(DATA_PATH, "indexed")
    var grass:E3DSubModel = model.get_node(NodePath("Grass"))

    assert_eq(grass.animation, E3DSubModel.ANIMATION_WIND)
    assert_eq(grass.transform.basis, SCENERY_FRAME, "an animated submodel is turned by its transform")
    assert_true(grass.dynamic_material, "replacableskin is the first replaceable skin")
    assert_eq(grass.dynamic_material_index, 0)
    assert_almost_eq(grass.visibility_range_end, 300.0, 0.001)
    assert_eq(grass.mesh.surface_get_arrays(0)[Mesh.ARRAY_INDEX].size(), 6)
    assert_not_null(grass.mesh.surface_get_arrays(0)[Mesh.ARRAY_TANGENT], "tangents are read")
    _assert_vectors(_vertices(grass), PackedVector3Array([
        Vector3(0, 0, 0), Vector3(1, 0, 0), Vector3(0, 0, 2), Vector3(1, 0, 2),
    ]), "Grass vertices")


## A translucent replaceable skin has its own phase bit, not 0x20 (Model3d.cpp:421-441) - a vehicle's
## windows painted on its skin; an opaque one has none
func test_a_translucent_skin_is_told_apart_from_a_translucent_texture() -> void:
    var glass:E3DSubModel = E3DModelManager.load_model(DATA_PATH, "skin_glass").get_node(NodePath("Grass"))
    var opaque:E3DSubModel = E3DModelManager.load_model(DATA_PATH, "indexed").get_node(NodePath("Grass"))

    assert_true(glass.skin_translucent, "opacity 0 on a replaceable skin")
    assert_false(glass.material_transparent, "which is not flag 0x20")
    assert_false(opaque.skin_translucent, "opacity 1 is the opaque phase")

