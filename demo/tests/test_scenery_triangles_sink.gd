extends MaszynaGutTest

## SceneryTrianglesSink: "triangles" are cut along the 1 km grid into chunks as they are added, and
## a sink with a directory writes them out, past its budget part by part.

## SceneryTrianglesSink::CHUNK_SIZE_M
const CELL_SIZE:float = 1000.0
const DIRECTORY:String = "user://test_scenery_triangles_sink"
## A budget of a few triangles, so that the sink writes parts many times over
const TEST_BUDGET_BYTES:int = 1024
const TEST_TRIANGLES:int = 100


func after_each() -> void:
    if not DirAccess.dir_exists_absolute(DIRECTORY):
        return
    for file:String in DirAccess.get_files_at(DIRECTORY):
        DirAccess.remove_absolute(DIRECTORY.path_join(file))
    DirAccess.remove_absolute(DIRECTORY)


## The chunks of the entries ([texture, vertices, normals, uvs]) as the old builder listed them,
## positions relative to "origin"
func _build_chunks(entries:Array) -> Array[Dictionary]:
    var sink:SceneryTrianglesSink = SceneryTrianglesSink.create("")
    for entry:Array in entries:
        sink.add_triangles(entry[0], entry[1], entry[2], entry[3], 0.0, 0.0)
    var chunks:Array[Dictionary] = []
    for geometry:MaszynaTrianglesChunkGeometry in sink.get_geometries():
        var arrays:Array = geometry.to_mesh_arrays()
        chunks.append({
            "texture": geometry.texture,
            "chunk_x": geometry.cell.x,
            "chunk_z": geometry.cell.y,
            "origin": SceneryTrianglesSink.cell_get_origin(geometry.cell),
            "vertices": arrays[Mesh.ARRAY_VERTEX],
            "normals": arrays[Mesh.ARRAY_NORMAL],
            "uvs": arrays[Mesh.ARRAY_TEX_UV],
        })
    return chunks


func _triangle_area(a:Vector3, b:Vector3, c:Vector3) -> float:
    return (b - a).cross(c - a).length() * 0.5


## A chunk is streamed and culled by its cell, so no piece of it may reach outside
func test_triangle_crossing_cells_is_cut_along_the_grid() -> void:
    var vertices:PackedVector3Array = PackedVector3Array([
        Vector3(-1500, 0, -1500), Vector3(0, 0, 1500), Vector3(1500, 0, -1500),
    ])
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.UP, Vector3.UP])
    var uvs:PackedVector2Array = PackedVector2Array([
        Vector2(-1.5, -1.5), Vector2(0.0, 1.5), Vector2(1.5, -1.5),
    ])
    var source_facing:Vector3 = (vertices[1] - vertices[0]).cross(vertices[2] - vertices[0])
    var chunks:Array = _build_chunks([["grass", vertices, normals, uvs]])
    assert_gt(chunks.size(), 1)
    var area:float = 0.0
    for chunk:Dictionary in chunks:
        var points:PackedVector3Array = chunk["vertices"]
        var cell_min:Vector2 = Vector2(chunk["chunk_x"], chunk["chunk_z"]) * CELL_SIZE
        for index:int in range(0, points.size(), 3):
            var corners:Array[Vector3] = []
            for offset:int in 3:
                var world_position:Vector3 = points[index + offset] + chunk["origin"]
                corners.append(world_position)
                assert_between(world_position.x, cell_min.x - 0.001, cell_min.x + CELL_SIZE + 0.001)
                assert_between(world_position.z, cell_min.y - 0.001, cell_min.y + CELL_SIZE + 0.001)
                # the source UVs are the position in kilometres, so a cut vertex must follow it
                var uv:Vector2 = chunk["uvs"][index + offset]
                assert_almost_eq(uv, Vector2(world_position.x, world_position.z) / CELL_SIZE, Vector2(0.0001, 0.0001))
                assert_almost_eq(chunk["normals"][index + offset], Vector3.UP, Vector3(0.0001, 0.0001, 0.0001))
            area += _triangle_area(corners[0], corners[1], corners[2])
            var facing:Vector3 = (corners[1] - corners[0]).cross(corners[2] - corners[0])
            assert_gt(facing.dot(source_facing), 0.0, "a piece must keep the winding of its triangle")
    assert_almost_eq(area, _triangle_area(vertices[0], vertices[1], vertices[2]), 0.01)


func test_triangle_within_one_cell_is_stored_unchanged() -> void:
    var vertices:PackedVector3Array = PackedVector3Array([
        Vector3(1100, 5, 1100), Vector3(1100, 7, 1900), Vector3(1900, 6, 1100),
    ])
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.RIGHT, Vector3.FORWARD])
    var uvs:PackedVector2Array = PackedVector2Array([Vector2(0, 0), Vector2(1, 0), Vector2(0, 1)])
    var chunks:Array = _build_chunks([["grass", vertices, normals, uvs]])
    assert_eq(chunks.size(), 1)
    var chunk:Dictionary = chunks[0]
    assert_eq(chunk["vertices"].size(), 3)
    for index:int in 3:
        assert_eq(chunk["vertices"][index] + chunk["origin"], vertices[index])
        assert_eq(chunk["normals"][index], normals[index])
        assert_eq(chunk["uvs"][index], uvs[index])


## Two triangles sharing an edge list its ends in opposite order; a cut computed from either end
## would differ in the last bits and open a crack along the grid line
func test_shared_edge_is_cut_at_identical_vertices() -> void:
    var edge_start:Vector3 = Vector3(310.7, 3.3, 120.9)
    var edge_end:Vector3 = Vector3(2790.1, 9.1, 2611.3)
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.UP, Vector3.UP])
    var uvs:PackedVector2Array = PackedVector2Array([Vector2.ZERO, Vector2.RIGHT, Vector2.DOWN])
    var cuts:Array[Array] = []
    for triangle:PackedVector3Array in [
        PackedVector3Array([edge_start, edge_end, Vector3(2900, 0, 150)]),
        PackedVector3Array([edge_end, edge_start, Vector3(200, 0, 2800)]),
    ]:
        var on_edge:Array[Vector3] = []
        for chunk:Dictionary in _build_chunks([["grass", triangle, normals, uvs]]):
            for point:Vector3 in chunk["vertices"]:
                var world_position:Vector3 = point + chunk["origin"]
                var along:float = (world_position - edge_start).dot(edge_end - edge_start) / (edge_end - edge_start).length_squared()
                var on_line:bool = world_position.distance_to(edge_start.lerp(edge_end, along)) < 0.0001
                if on_line and not on_edge.has(world_position):
                    on_edge.append(world_position)
        on_edge.sort()
        cuts.append(on_edge)
    assert_gt(cuts[0].size(), 2, "the edge crosses grid lines, so it must have been cut")
    assert_eq(cuts[0], cuts[1])


func test_entries_share_buffers_only_with_same_material_and_cell() -> void:
    var vertices:PackedVector3Array = PackedVector3Array([
        Vector3(-10, 0, -10), Vector3(-20, 0, -10), Vector3(-10, 0, -20),
    ])
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.UP, Vector3.UP])
    var uvs:PackedVector2Array = PackedVector2Array([Vector2.ZERO, Vector2.RIGHT, Vector2.DOWN])
    var chunks:Array = _build_chunks([
        ["grass", vertices, normals, uvs],
        ["grass", vertices, normals, uvs],
        ["sand", vertices, normals, uvs],
    ])
    assert_eq(chunks.size(), 2)
    for chunk:Dictionary in chunks:
        assert_eq(chunk["chunk_x"], -1)
        assert_eq(chunk["chunk_z"], -1)
        assert_eq(chunk["vertices"].size(), 6 if chunk["texture"] == "grass" else 3)


## Past the budget, chunks go to disk as parts; finish() puts every chunk back together whole
func test_finished_chunks_hold_every_triangle_also_past_the_budget() -> void:
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.UP, Vector3.UP])
    var uvs:PackedVector2Array = PackedVector2Array([Vector2.ZERO, Vector2.RIGHT, Vector2.DOWN])
    var triangle:PackedVector3Array = PackedVector3Array([
        Vector3(100, 0, 100), Vector3(100, 0, 200), Vector3(200, 0, 100),
    ])
    var sink:SceneryTrianglesSink = SceneryTrianglesSink.create(DIRECTORY, TEST_BUDGET_BYTES)
    for index:int in TEST_TRIANGLES:
        sink.add_triangles("grass", triangle, normals, uvs, 0.0, 0.0)
    assert_gt(DirAccess.get_files_at(DIRECTORY).size(), 0, "nothing went to disk past the budget")

    var descriptors:Array = sink.finish()
    assert_eq(descriptors.size(), 1)
    assert_eq(DirAccess.get_files_at(DIRECTORY).size(), 1, "parts were left beside the chunk")
    var geometry:MaszynaTrianglesChunkGeometry = ResourceLoader.load(descriptors[0]["path"])
    assert_eq(geometry.vertices.size(), TEST_TRIANGLES * 3 * 3, "triangles lost between the parts")
    assert_eq(descriptors[0]["position"], SceneryTrianglesSink.cell_get_origin(Vector2i.ZERO))


func test_added_geometry_joins_the_chunk_of_its_texture_cell_and_range() -> void:
    var vertices:PackedVector3Array = PackedVector3Array([
        Vector3(100, 0, 100), Vector3(100, 0, 200), Vector3(200, 0, 100),
    ])
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.UP, Vector3.UP])
    var uvs:PackedVector2Array = PackedVector2Array([Vector2.ZERO, Vector2.RIGHT, Vector2.DOWN])
    var subscene:SceneryTrianglesSink = SceneryTrianglesSink.create("")
    subscene.add_triangles("grass", vertices, normals, uvs, 0.0, 300.0)
    var scenery:SceneryTrianglesSink = SceneryTrianglesSink.create("")
    scenery.add_triangles("grass", vertices, normals, uvs, 0.0, 300.0)
    scenery.add_triangles("grass", vertices, normals, uvs, 0.0, 0.0)
    for geometry:MaszynaTrianglesChunkGeometry in subscene.get_geometries():
        scenery.add_geometry(geometry)
    var geometries:Array[MaszynaTrianglesChunkGeometry] = scenery.get_geometries()
    assert_eq(geometries.size(), 2, "another range is another chunk")
    assert_eq(geometries[0].range_max, 300.0)
    assert_eq(geometries[0].vertices.size(), 2 * 3 * 3)


## A subscene's finished chunks join the scenery's sink file by file
func test_finished_chunk_file_joins_another_sink() -> void:
    var vertices:PackedVector3Array = PackedVector3Array([
        Vector3(100, 0, 100), Vector3(100, 0, 200), Vector3(200, 0, 100),
    ])
    var normals:PackedVector3Array = PackedVector3Array([Vector3.UP, Vector3.UP, Vector3.UP])
    var uvs:PackedVector2Array = PackedVector2Array([Vector2.ZERO, Vector2.RIGHT, Vector2.DOWN])
    var subscene:SceneryTrianglesSink = SceneryTrianglesSink.create(DIRECTORY)
    subscene.add_triangles("grass", vertices, normals, uvs, 0.0, 300.0)
    var descriptors:Array = subscene.finish()
    var scenery:SceneryTrianglesSink = SceneryTrianglesSink.create("")
    scenery.add_triangles("grass", vertices, normals, uvs, 0.0, 300.0)
    scenery.add_geometry_file(descriptors[0]["path"])
    var geometries:Array[MaszynaTrianglesChunkGeometry] = scenery.get_geometries()
    assert_eq(geometries.size(), 1, "the same texture, cell and range is one chunk")
    assert_eq(geometries[0].vertices.size(), 2 * 3 * 3)



## A triangle with an edge on a cell border only touches the next cell: an empty chunk of it was
## written to the cache and could not be made a mesh when streamed
func test_a_cell_a_triangle_only_touches_gets_no_chunk() -> void:
    var vertices:PackedVector3Array = [
        Vector3(CELL_SIZE, 0.0, 10.0), Vector3(CELL_SIZE - 10.0, 0.0, 10.0), Vector3(CELL_SIZE, 0.0, 20.0)
    ]
    var normals:PackedVector3Array = [Vector3.UP, Vector3.UP, Vector3.UP]
    var uvs:PackedVector2Array = [Vector2.ZERO, Vector2.ZERO, Vector2.ZERO]
    var chunks:Array[Dictionary] = _build_chunks([["border", vertices, normals, uvs]])
    assert_eq(chunks.size(), 1)
    assert_eq(chunks[0]["chunk_x"], 0)


## A chunk is uploaded in one go when it is streamed, so a large one is cut into pieces of whole
## triangles that together are the same geometry
func test_a_chunk_larger_than_an_upload_is_split_into_whole_triangles() -> void:
    # whole triangles, one more than fit below the limit
    var vertex_count:int = (MaszynaTrianglesChunkGeometry.MAX_VERTICES / 3 + 1) * 3
    var floats:PackedFloat32Array = PackedFloat32Array()
    floats.resize(vertex_count * 3)
    var uv_floats:PackedFloat32Array = PackedFloat32Array()
    uv_floats.resize(vertex_count * 2)
    var geometry:MaszynaTrianglesChunkGeometry = MaszynaTrianglesChunkGeometry.new()
    geometry.texture = "split"
    geometry.vertices = floats
    geometry.normals = floats
    geometry.uvs = uv_floats
    var pieces:Array[MaszynaTrianglesChunkGeometry] = geometry.split()
    assert_eq(pieces.size(), 2)
    var total:int = 0
    for piece:MaszynaTrianglesChunkGeometry in pieces:
        var piece_vertices:int = piece.vertices.size() / 3
        assert_eq(piece_vertices % 3, 0, "a piece cut a triangle")
        assert_true(piece_vertices <= MaszynaTrianglesChunkGeometry.MAX_VERTICES)
        assert_eq(piece.texture, "split")
        total += piece_vertices
    assert_eq(total, vertex_count)
