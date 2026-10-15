extends MaszynaGutTest

## MaszynaLinesGeometry - a section's "lines" nodes merged as the original merges them in its cells
## (basic_cell::insert(), scene.cpp:302-326; lines_node::merge(), scenenode.cpp:653-672) and the
## mesh scenery_lines.gdshader draws them from

## The section of the grid around the origin of the world
const ORIGIN:Vector3 = Vector3(500, 0, 500)


func _geometry() -> MaszynaLinesGeometry:
    var geometry:MaszynaLinesGeometry = MaszynaLinesGeometry.new()
    geometry.origin = ORIGIN
    return geometry


func _line(from:Vector3, to:Vector3) -> PackedVector3Array:
    return PackedVector3Array([from, to])


func test_the_section_of_a_place_is_the_one_of_the_1_km_grid() -> void:
    assert_eq(MaszynaLinesGeometry.position_get_section_origin(Vector3(10, 5, 999)), ORIGIN)
    assert_eq(MaszynaLinesGeometry.position_get_section_origin(Vector3(-10, 5, 1001)), Vector3(-500, 0, 1500))


func test_nodes_of_the_same_look_and_range_closer_than_10_m_in_a_cell_merge() -> void:
    var geometry:MaszynaLinesGeometry = _geometry()
    geometry.add_lines(_line(Vector3(100, 10, 100), Vector3(110, 10, 100)), Color.BLACK, 5.0, 0.0, 500.0)
    geometry.add_lines(_line(Vector3(105, 10, 104), Vector3(115, 10, 104)), Color.BLACK, 5.0, 0.0, 500.0)

    assert_eq(geometry.get_node_count(), 1)


func test_nodes_of_another_look_range_place_or_cell_stay_apart() -> void:
    var geometry:MaszynaLinesGeometry = _geometry()
    geometry.add_lines(_line(Vector3(240, 10, 100), Vector3(246, 10, 100)), Color.BLACK, 5.0, 0.0, 500.0)
    geometry.add_lines(_line(Vector3(240, 10, 100), Vector3(246, 10, 100)), Color.BLACK, 0.9, 0.0, 500.0)
    geometry.add_lines(_line(Vector3(240, 10, 100), Vector3(246, 10, 100)), Color.WHITE, 5.0, 0.0, 500.0)
    geometry.add_lines(_line(Vector3(240, 10, 100), Vector3(246, 10, 100)), Color.BLACK, 5.0, 0.0, 2000.0)
    geometry.add_lines(_line(Vector3(240, 10, 120), Vector3(246, 10, 120)), Color.BLACK, 5.0, 0.0, 500.0)
    # 4 m away, in the next cell of the 250 m grid
    geometry.add_lines(_line(Vector3(250, 10, 100), Vector3(254, 10, 100)), Color.BLACK, 5.0, 0.0, 500.0)

    assert_eq(geometry.get_node_count(), 6)


## A quad a line; its corners carry what the shader draws it from, relative to the origin
func test_the_mesh_is_a_quad_a_line_with_its_node() -> void:
    var geometry:MaszynaLinesGeometry = _geometry()
    geometry.add_lines(_line(Vector3(500, 20, 500), Vector3(510, 20, 500)), Color(0.2, 0.4, 0.6), 1.5, 10.0, 500.0)

    var arrays:Array = geometry.build_mesh().surface_get_arrays(0)
    var vertices:PackedVector3Array = arrays[Mesh.ARRAY_VERTEX]
    assert_eq(vertices.size(), MaszynaLinesGeometry.QUAD_VERTICES)
    assert_eq((arrays[Mesh.ARRAY_INDEX] as PackedInt32Array).size(), MaszynaLinesGeometry.QUAD_INDICES)
    assert_eq(vertices[0], Vector3(0, 20, 0))
    assert_eq(vertices[2], Vector3(10, 20, 0))
    var other_ends:PackedFloat32Array = arrays[Mesh.ARRAY_CUSTOM0]
    assert_eq(Vector3(other_ends[0], other_ends[1], other_ends[2]), Vector3(10, 20, 0), "the other end")
    var centres:PackedFloat32Array = arrays[Mesh.ARRAY_CUSTOM1]
    assert_eq(Vector4(centres[0], centres[1], centres[2], centres[3]), Vector4(5, 20, 0, 5), "the centre and the radius")
    var looks:PackedFloat32Array = arrays[Mesh.ARRAY_CUSTOM2]
    assert_eq(Vector3(looks[0], looks[1], looks[2]), Vector3(1.5, 10, 500), "the width and the range")
    assert_true((arrays[Mesh.ARRAY_COLOR] as PackedColorArray)[0].is_equal_approx(Color(0.2, 0.4, 0.6)), "the colour")


## The mesh is streamed by the centre of its box, a node's range is measured from its own centre
func test_the_range_end_reaches_every_node_and_none_when_one_has_no_limit() -> void:
    var geometry:MaszynaLinesGeometry = _geometry()
    geometry.add_lines(_line(Vector3(100, 0, 500), Vector3(120, 0, 500)), Color.BLACK, 1.0, 0.0, 500.0)
    geometry.add_lines(_line(Vector3(880, 0, 500), Vector3(900, 0, 500)), Color.BLACK, 1.0, 0.0, 2000.0)
    assert_almost_eq(geometry.get_range_end(), 2000.0 + 390.0, 0.001)

    geometry.add_lines(_line(Vector3(500, 0, 500), Vector3(510, 0, 500)), Color.BLACK, 1.0, 0.0, 0.0)
    assert_eq(geometry.get_range_end(), 0.0)
