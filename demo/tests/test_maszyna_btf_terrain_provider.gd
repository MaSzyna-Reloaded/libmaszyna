extends MaszynaGutTest

## The original's binary terrain file (.btf, terrain_file, sceneterrain.cpp): its sections are
## listed as streaming cells, and a section's shapes are supplied as terrain chunks merged as the
## original merges them (terrain_file::load()). The fixtures are made as terrain_file::write()
## lays a file out.

const FIXTURES:String = "res://tests/fixtures/scenery/terrain"
## The fixture's one section has the original's index 7 - column 7 of row 0
## (MaszynaBTFTerrainProvider.REGION_SIDE_SECTION_COUNT)
const FIXTURE_CELL:Vector2i = Vector2i(7 - 250, -250)
## The fixture's section reaches 800 m from its centre, past the 707 m of its corners
const FIXTURE_OVERHANG:float = 800.0 - 0.5 * sqrt(2.0) * 1000.0
## Where the bush shape stands in its section, and the centre of its cell, the sixth: column 1 of
## row 1 of the section's 4 x 4 cells of 250 m (basic_section::center(), scene.cpp:996)
const BUSH_OFFSET:Vector3 = Vector3(-100.0, 0.0, -120.0)
const BUSH_CELL_OFFSET:Vector3 = Vector3(-125.0, 0.0, -125.0)
## The bush's range, 100 m - a shape within 300 m is drawn by its cell (scene.cpp:894)
const BUSH_RANGE:float = 100.0


func _path(file_name:String) -> String:
    return ProjectSettings.globalize_path(FIXTURES.path_join(file_name))


func _provider() -> MaszynaBTFTerrainProvider:
    var provider:MaszynaBTFTerrainProvider = MaszynaBTFTerrainProvider.new()
    assert_true(provider.open(_path("provider.btf"), func(material:String) -> bool: return material == "test_glass"))
    return provider


func _geometry(geometries:Array, texture:String) -> MaszynaTrianglesChunkGeometry:
    for geometry:MaszynaTrianglesChunkGeometry in geometries:
        if geometry.texture == texture:
            return geometry
    return null


func _world_vertex(geometry:MaszynaTrianglesChunkGeometry, index:int) -> Vector3:
    var vertices:PackedFloat32Array = geometry.vertices
    return geometry.origin + Vector3(vertices[index * 3], vertices[index * 3 + 1], vertices[index * 3 + 2])


func test_the_source_length_is_read_from_a_terrain_file_only() -> void:
    assert_eq(MaszynaBTFTerrainProvider.get_source_length(_path("alone.btf")), 123)
    assert_eq(MaszynaBTFTerrainProvider.get_source_length(_path("binary.txtf")), -1, "a text file is none")
    assert_eq(MaszynaBTFTerrainProvider.get_source_length(_path("missing.btf")), -1)


func test_the_sections_are_listed_as_streaming_cells_without_being_read() -> void:
    var provider:MaszynaBTFTerrainProvider = _provider()
    assert_eq(provider.get_chunk_cells(), [FIXTURE_CELL] as Array[Vector2i])
    assert_almost_eq(provider.chunk_get_overhang(FIXTURE_CELL), FIXTURE_OVERHANG, 0.01)
    assert_eq(provider.get_content_kinds(), PackedInt32Array([SceneryStreamingProvider.CONTENT_TERRAIN]))


## An opaque shape seen at any distance is merged by texture over the section, around its centre; a
## shape seen within a range, or a translucent one, is a group of a chunk per texture and range
## around its cell's centre, the group around the shape's own centre (terrain_file::load())
func test_a_section_is_its_shapes_merged_as_the_original_merges_them() -> void:
    var geometries:Array = _provider().chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_TERRAIN)
    assert_eq(geometries.size(), 3)
    var section_origin:Vector3 = MaszynaTrianglesSink.cell_get_origin(FIXTURE_CELL)
    var grass:MaszynaTrianglesChunkGeometry = _geometry(geometries, "test_grass")
    assert_eq(grass.origin, section_origin, "merged over the section")
    assert_eq(grass.range_max, -1.0, "seen at any distance")
    var bush:MaszynaTrianglesChunkGeometry = _geometry(geometries, "test_bush")
    assert_eq(bush.origin, section_origin + BUSH_CELL_OFFSET, "around its cell's centre")
    assert_almost_eq(bush.range_max, BUSH_RANGE, 0.001)
    assert_eq(bush.origin + bush.group_centres[0], section_origin + BUSH_OFFSET, "a group around its own centre")
    var glass:MaszynaTrianglesChunkGeometry = _geometry(geometries, "test_glass")
    assert_eq(glass.origin, section_origin + Vector3(-375.0, 0.0, -375.0), "a translucent one in its cell, the first")


func test_vertices_are_in_world_space_and_turned_around_like_a_triangles_node() -> void:
    var geometries:Array = _provider().chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_TERRAIN)
    var section_origin:Vector3 = MaszynaTrianglesSink.cell_get_origin(FIXTURE_CELL)
    var grass:MaszynaTrianglesChunkGeometry = _geometry(geometries, "test_grass")
    assert_almost_eq(_world_vertex(grass, 0), section_origin + Vector3(1.0, 0.0, 0.0), Vector3.ONE * 0.001)
    assert_almost_eq(_world_vertex(grass, 2), section_origin, Vector3.ONE * 0.001)


func test_a_cell_without_a_section_or_another_kind_has_nothing() -> void:
    var provider:MaszynaBTFTerrainProvider = _provider()
    assert_eq(provider.chunk_load(Vector2i.ZERO, SceneryStreamingProvider.CONTENT_TERRAIN), [])
    assert_eq(provider.chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_LINES), [])


func test_a_file_of_another_kind_is_not_opened() -> void:
    var provider:MaszynaBTFTerrainProvider = MaszynaBTFTerrainProvider.new()
    assert_false(provider.open(_path("binary.txtf"), func(_material:String) -> bool: return false))
    assert_push_error("unrecognized type or version")
