extends MaszynaGutTest

## The original's binary region file (.sbt): its sections are listed as streaming cells, and a
## cell's shapes are supplied as terrain chunks the way "triangles" nodes make them. With a region
## file the scenery's own "triangles", terrain models and "_ter.scm" includes are left out.

const FIXTURES:String = "res://tests/fixtures/region"
## The fixture's one section has the original's index 7 - column 7 of row 0
## (MaszynaLegacySBTTerrainProvider.REGION_SIDE_SECTION_COUNT)
const FIXTURE_CELL:Vector2i = Vector2i(7 - 250, -250)
## The fixture's section shape: three vertices around this origin
const GROUND_ORIGIN:Vector3 = Vector3(100.5, 2.0, 200.25)


func _region_path(file_name:String) -> String:
    return ProjectSettings.globalize_path(FIXTURES.path_join(file_name))


func _provider() -> MaszynaLegacySBTTerrainProvider:
    var provider:MaszynaLegacySBTTerrainProvider = MaszynaLegacySBTTerrainProvider.new()
    assert_true(provider.open(_region_path("test_region.sbt")))
    return provider


func _geometry(geometries:Array, texture:String) -> MaszynaTrianglesChunkGeometry:
    for geometry:MaszynaTrianglesChunkGeometry in geometries:
        if geometry.texture == texture:
            return geometry
    return null


func _world_vertex(geometry:MaszynaTrianglesChunkGeometry, index:int) -> Vector3:
    var vertices:PackedFloat32Array = geometry.vertices
    return SceneryTrianglesSink.cell_get_origin(geometry.cell) + Vector3(
            vertices[index * 3], vertices[index * 3 + 1], vertices[index * 3 + 2])


func test_a_region_file_of_another_version_is_not_one() -> void:
    assert_true(MaszynaLegacySBTTerrainProvider.is_region(_region_path("test_region.sbt")))
    assert_false(MaszynaLegacySBTTerrainProvider.is_region(_region_path("bad_version.sbt")))
    assert_false(MaszynaLegacySBTTerrainProvider.is_region(_region_path("missing.sbt")))


func test_the_sections_are_listed_as_streaming_cells_without_being_read() -> void:
    var provider:MaszynaLegacySBTTerrainProvider = _provider()
    assert_eq(provider.get_chunk_cells(), [FIXTURE_CELL] as Array[Vector2i])
    assert_eq(provider.chunk_get_overhang(FIXTURE_CELL), 0.0, "a section of the default radius reaches out")
    assert_eq(provider.get_content_kinds(), PackedInt32Array([SceneryStreamingProvider.CONTENT_TERRAIN]))


func test_a_cell_is_its_visible_shapes_one_chunk_per_texture_and_range() -> void:
    var geometries:Array = _provider().chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_TERRAIN)
    assert_eq(geometries.size(), 2, "the hidden shape and the lines are left out")
    assert_not_null(_geometry(geometries, "test_grass"), "textures/ is not trimmed")
    assert_not_null(_geometry(geometries, "test_bush"), "a cell's shape with user data is read past its user data")
    assert_eq(_geometry(geometries, "test_grass").cell, FIXTURE_CELL)


func test_vertices_are_in_world_space_and_turned_around_like_a_triangles_node() -> void:
    var geometries:Array = _provider().chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_TERRAIN)
    var ground:MaszynaTrianglesChunkGeometry = _geometry(geometries, "test_grass")
    assert_almost_eq(_world_vertex(ground, 0), GROUND_ORIGIN + Vector3(0.0, 0.0, 1.0), Vector3.ONE * 0.001)
    assert_almost_eq(_world_vertex(ground, 2), GROUND_ORIGIN, Vector3.ONE * 0.001)


func test_visibility_ranges_are_the_square_roots_and_no_limit_is_none() -> void:
    var geometries:Array = _provider().chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_TERRAIN)
    assert_eq(_geometry(geometries, "test_grass").range_max, -1.0)
    assert_almost_eq(_geometry(geometries, "test_bush").range_max, 500.0, 0.001)


func test_a_cell_without_a_section_or_another_kind_has_nothing() -> void:
    var provider:MaszynaLegacySBTTerrainProvider = _provider()
    assert_eq(provider.chunk_load(Vector2i.ZERO, SceneryStreamingProvider.CONTENT_TERRAIN), [])
    assert_eq(provider.chunk_load(FIXTURE_CELL, SceneryStreamingProvider.CONTENT_MODELS), [])


func test_a_region_file_leaves_out_the_terrain_after_it_and_is_recorded() -> void:
    var context := MaszynaImporterContext.new()
    context.load_binary_terrain(_region_path("test_region.sbt"))
    assert_true(context.binary_terrain)
    assert_true(context.binary_terrain_state)
    assert_eq(context.region_files, [_region_path("test_region.sbt")] as Array[String])
    var included := MaszynaImporterContext.from_state(context.get_state())
    assert_true(included.binary_terrain, "an include inherits it")
    context.push_state()
    context.pop_state()
    assert_true(context.binary_terrain, "it holds for the rest of the load")


func test_a_missing_region_file_still_leaves_out_the_terrain_includes() -> void:
    var context := MaszynaImporterContext.new()
    context.load_binary_terrain(_region_path("missing.sbt"))
    assert_false(context.binary_terrain)
    assert_true(context.binary_terrain_state, "deserialize_terrain() sets it whatever the file")
    assert_eq(context.region_files, [] as Array[String])
