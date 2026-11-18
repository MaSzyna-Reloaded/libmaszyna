extends MaszynaGutTest

## The binary terrain file the original makes of a text terrain file (terrain_file::write(),
## sceneterrain.cpp:400): its "triangles" in the cells of the original's sections - a triangle
## reaching more than 200 m out of its section cut at the section's edge, the shapes of one look and
## range within 25 m of each other merged - read back by MaszynaBTFTerrainProvider.

const FIXTURES:String = "res://tests/fixtures/scenery/terrain"
const OUTPUT:String = "user://test_btf_terrain_writer/triangles.btf"
## The fixture's sections: x 0-1000 m and 1000-2000 m of the row at z 0-1000 m
## (MaszynaBTFTerrainProvider.REGION_SIDE_SECTION_COUNT)
const FIRST_CELL:Vector2i = Vector2i(0, 0)
const NEXT_CELL:Vector2i = Vector2i(1, 0)
## Where the crossing triangle is cut: its side from x 500 m to 1500 m at the sections' edge
const CUT_POINT:Vector3 = Vector3(1000.0, 0.0, 100.0)
## The grass: two triangles merged, the triangle without an area left out, and the crossing one cut
## twice - its part past the edge still reaches 500 m back out of the next section, whose centre it
## stands on (basic_region::insert() divides every part again): two parts in the first section, one
## in the next
const FIRST_GRASS_VERTICES:int = 12
const NEXT_GRASS_VERTICES:int = 3
## The bushes: the two of one look merged, the red one a shape of its own
const BUSH_SHAPES:int = 2
const BUSH_VERTICES:int = 9
## CRC-32 of "123456789", the check value of zlib's
const CRC_CHECK:int = 0xCBF43926


func _text_path(file_name:String) -> String:
    return ProjectSettings.globalize_path(FIXTURES.path_join(file_name))


func _written() -> MaszynaBTFTerrainProvider:
    assert_true(MaszynaBTFTerrainWriter.write(_text_path("triangles.txtf"), ProjectSettings.globalize_path(OUTPUT)))
    var provider:MaszynaBTFTerrainProvider = MaszynaBTFTerrainProvider.new()
    assert_true(provider.open(ProjectSettings.globalize_path(OUTPUT), func(_material:String) -> bool: return false))
    return provider


func _geometry(geometries:Array, texture:String) -> MaszynaTrianglesChunkGeometry:
    for geometry:MaszynaTrianglesChunkGeometry in geometries:
        if geometry.texture == texture:
            return geometry
    return null


func test_the_checksum_is_zlib_crc32() -> void:
    assert_eq(MaszynaBTFTerrainWriter.checksum("123456789".to_ascii_buffer()), CRC_CHECK)


func test_the_file_is_made_from_the_whole_text() -> void:
    _written()
    var text_length:int = FileAccess.get_file_as_bytes(_text_path("triangles.txtf")).size()
    assert_eq(MaszynaBTFTerrainProvider.get_source_length(ProjectSettings.globalize_path(OUTPUT)), text_length)


func test_a_triangle_reaching_into_the_next_section_is_cut_at_its_edge() -> void:
    var provider:MaszynaBTFTerrainProvider = _written()
    var cells:Array[Vector2i] = provider.get_chunk_cells()
    cells.sort()
    assert_eq(cells, [FIRST_CELL, NEXT_CELL] as Array[Vector2i])
    var next:MaszynaTrianglesChunkGeometry = _geometry(
        provider.chunk_load(NEXT_CELL, SceneryStreamingProvider.CONTENT_TERRAIN), "test_grass")
    assert_eq(next.vertices.size() / 3, NEXT_GRASS_VERTICES)
    var cut:bool = false
    for index:int in NEXT_GRASS_VERTICES:
        var vertices:PackedFloat32Array = next.vertices
        var point:Vector3 = next.origin + Vector3(vertices[index * 3], vertices[index * 3 + 1], vertices[index * 3 + 2])
        cut = cut or point.is_equal_approx(CUT_POINT)
    assert_true(cut, "the new vertex where the side crosses the edge")


func test_shapes_of_one_look_near_each_other_are_merged() -> void:
    var geometries:Array = _written().chunk_load(FIRST_CELL, SceneryStreamingProvider.CONTENT_TERRAIN)
    assert_eq(_geometry(geometries, "test_grass").vertices.size() / 3, FIRST_GRASS_VERTICES)
    var bush:MaszynaTrianglesChunkGeometry = _geometry(geometries, "test_bush")
    assert_eq(bush.get_group_count(), BUSH_SHAPES, "the red one apart")
    assert_eq(bush.vertices.size() / 3, BUSH_VERTICES)


func test_a_text_of_other_nodes_is_not_written() -> void:
    assert_false(MaszynaBTFTerrainWriter.write(_text_path("ground.txtf"), ProjectSettings.globalize_path(OUTPUT)))
    assert_push_error("can't be a part of binary terrain")
