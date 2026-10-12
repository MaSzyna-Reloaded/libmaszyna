extends MaszynaGutTest

## E3DMesh: an E3D submodel's surface keeps its arrays, so its faces, its arrays and a saved copy of
## it come from memory, never read back from the RenderingServer (a GPU stall on every frame in
## flight).

const SAVE_DIRECTORY:String = "user://test_e3d_mesh"
const SAVE_PATH:String = SAVE_DIRECTORY + "/mesh.res"
## One quad: four corners, two clockwise triangles
const QUAD_VERTICES:PackedFloat32Array = [0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 2.0, 0.0, 0.0, 2.0, 0.0]
const QUAD_NORMALS:PackedFloat32Array = [0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0, 0.0, 0.0, 1.0]
const QUAD_UVS:PackedFloat32Array = [0.0, 0.0, 1.0, 0.0, 1.0, 1.0, 0.0, 1.0]
const QUAD_INDICES:PackedInt32Array = [0, 2, 1, 0, 3, 2]
const QUAD_CORNERS:int = 4
const QUAD_TRIANGLE_CORNERS:int = 6


func after_each() -> void:
    if FileAccess.file_exists(SAVE_PATH):
        DirAccess.remove_absolute(SAVE_PATH)
    if DirAccess.dir_exists_absolute(SAVE_DIRECTORY):
        DirAccess.remove_absolute(SAVE_DIRECTORY)


func _quad() -> E3DMesh:
    var mesh:E3DMesh = E3DMesh.new()
    mesh.surface = {
        "vertices": QUAD_VERTICES,
        "normals": QUAD_NORMALS,
        "uvs": QUAD_UVS,
        "indices": QUAD_INDICES,
    }
    return mesh


func test_the_surface_is_answered_from_its_arrays() -> void:
    var mesh:E3DMesh = _quad()
    assert_eq(mesh.get_surface_count(), 1)
    var arrays:Array = mesh.surface_get_arrays(0)
    var vertices:PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
    assert_eq(vertices.size(), QUAD_CORNERS)
    assert_eq(vertices[2], Vector3(1.0, 2.0, 0.0))
    assert_eq(arrays[Mesh.ARRAY_INDEX], QUAD_INDICES)
    assert_eq(mesh.get_faces().size(), QUAD_TRIANGLE_CORNERS, "the faces come from the indices")
    assert_eq(mesh.get_aabb(), AABB(Vector3.ZERO, Vector3(1.0, 2.0, 0.0)))
    assert_true(mesh.get_rid().is_valid(), "a RenderingServer mesh is made of the arrays")


func test_a_saved_mesh_keeps_its_arrays() -> void:
    DirAccess.make_dir_recursive_absolute(SAVE_DIRECTORY)
    assert_eq(ResourceCache.resource_save(_quad(), SAVE_PATH), OK)
    var loaded:E3DMesh = ResourceLoader.load(SAVE_PATH, "", ResourceLoader.CACHE_MODE_IGNORE) as E3DMesh
    assert_not_null(loaded)
    assert_eq(loaded.surface["vertices"], QUAD_VERTICES)
    assert_eq(loaded.surface["indices"], QUAD_INDICES)
    assert_true(loaded.get_rid().is_valid(), "a loaded mesh is made again of its arrays")


func test_an_empty_surface_has_no_mesh() -> void:
    var mesh:E3DMesh = E3DMesh.new()
    mesh.surface = {}
    assert_eq(mesh.get_surface_count(), 0)
    assert_false(mesh.get_rid().is_valid())
