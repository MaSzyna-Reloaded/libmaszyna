extends MaszynaGutTest

## A model file is read apart as its description and its geometry (MaszynaE3DModelReader): the
## description holds no mesh - nothing of it in the GPU - and its geometry is data only, written and
## read without reading the GPU back, until it is applied to the description as its meshes.

const SAVE_DIRECTORY:String = "user://test_e3d_model_geometry"
const SAVE_PATH:String = SAVE_DIRECTORY + "/geometry.res"
const MODEL_PATH:String = "res://tests/fixtures/models/sem/karzelki/ktmnb.e3d"
const VECTOR3_FLOATS:int = 3


func after_each() -> void:
    if FileAccess.file_exists(SAVE_PATH):
        DirAccess.remove_absolute(SAVE_PATH)
    if DirAccess.dir_exists_absolute(SAVE_DIRECTORY):
        DirAccess.remove_absolute(SAVE_DIRECTORY)


## Every submodel of the tree, depth first
func _submodels(submodels:Array) -> Array[MaszynaE3DSubModel]:
    var all:Array[MaszynaE3DSubModel] = []
    for submodel:MaszynaE3DSubModel in submodels:
        all.append(submodel)
        all.append_array(_submodels(submodel.submodels))
    return all


## The submodels of triangles, by their geometry index
func _with_geometry(model:MaszynaE3DModel) -> Array[MaszynaE3DSubModel]:
    var found:Array[MaszynaE3DSubModel] = []
    for submodel:MaszynaE3DSubModel in _submodels(model.submodels):
        if submodel.geometry_index >= 0:
            found.append(submodel)
    return found


func test_a_description_has_no_mesh() -> void:
    var description:MaszynaE3DModel = MaszynaE3DModelReader.read_description(MODEL_PATH)
    assert_not_null(description)
    assert_false(description.has_geometry())
    assert_gt(_with_geometry(description).size(), 0, "its submodels of triangles know their surfaces")
    for submodel:MaszynaE3DSubModel in _submodels(description.submodels):
        assert_null(submodel.mesh, "%s has no mesh" % submodel.resource_name)


func test_the_geometry_applied_gives_every_submodel_of_triangles_its_mesh() -> void:
    var description:MaszynaE3DModel = MaszynaE3DModelReader.read_description(MODEL_PATH)
    description.apply_geometry(MaszynaE3DModelReader.read_geometry(MODEL_PATH))
    assert_true(description.has_geometry())
    for submodel:MaszynaE3DSubModel in _with_geometry(description):
        assert_true(submodel.mesh is ArrayMesh, "%s is drawn of a mesh" % submodel.resource_name)
        assert_eq(submodel.mesh.get_surface_count(), 1)


func test_a_saved_geometry_keeps_its_arrays() -> void:
    var geometry:MaszynaE3DModelGeometry = MaszynaE3DModelReader.read_geometry(MODEL_PATH)
    DirAccess.make_dir_recursive_absolute(SAVE_DIRECTORY)
    assert_eq(ResourceCache.resource_save(geometry, SAVE_PATH), OK)
    var loaded:MaszynaE3DModelGeometry = ResourceLoader.load(
            SAVE_PATH, "", ResourceLoader.CACHE_MODE_IGNORE) as MaszynaE3DModelGeometry
    assert_not_null(loaded)
    assert_eq(loaded.surfaces, geometry.surfaces)


func test_a_submodels_positions_are_read_alone() -> void:
    var description:MaszynaE3DModel = MaszynaE3DModelReader.read_description(MODEL_PATH)
    var geometry:MaszynaE3DModelGeometry = MaszynaE3DModelReader.read_geometry(MODEL_PATH)
    var submodel:MaszynaE3DSubModel = _with_geometry(description)[0]
    var positions:PackedVector3Array = MaszynaE3DModelReader.read_submodel_positions(MODEL_PATH, submodel.geometry_index)
    var vertices:PackedFloat32Array = geometry.surfaces[submodel.geometry_index]["vertices"]
    assert_eq(positions.size(), vertices.size() / VECTOR3_FLOATS, "as many as the geometry has")
    assert_almost_eq(positions[0], Vector3(vertices[0], vertices[1], vertices[2]), Vector3(0.0001, 0.0001, 0.0001))
