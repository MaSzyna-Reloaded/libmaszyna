extends MaszynaGutTest

## A world (MaszynaWorld) composed of its sectors: each sector's tracks moved from its corner to its
## place, registered with TrackServer and joined across the sectors' edge, and freed with the world;
## what is drawn of the loaded sectors handed over by the cells of the streaming grid
## (MaszynaWorldSectorProvider); the sectors in reach of the camera asked of the world's source and
## loaded once it has them (MaszynaWorldSectors).

## The sector the world's origin is the corner of, and its western neighbour
const ORIGIN_CELL:Vector2i = Vector2i(437, 700)
const WEST_CELL:Vector2i = Vector2i(436, 700)
const TRACK_WIDTH:float = 1.435
## How far off a moved point may be [m]
const TOLERANCE:float = 0.001
const TRACK_Z:float = 10.0
const TRACK_LENGTH:float = 100.0
## SceneryStreamingServer::CHUNK_SIZE_M (not bound)
const STREAMING_CHUNK:float = 1000.0
const SECTORS_DIR:String = "user://gut/world_sectors"
## Where the origin's sector's two meshes stand in it, and the cells of the grid that puts them in
const NEAR_MESH:Vector3 = Vector3(-100.0, 0.0, 100.0)
const FAR_MESH:Vector3 = Vector3(-1010.0, 0.0, 500.0)
const NEAR_CELL:Vector2i = Vector2i(-1, 0)
const FAR_CELL:Vector2i = Vector2i(-2, 0)
const MESH_RANGE:float = 300.0
## Where the origin's sector's decoration stands in it, and its wire's two ends
const MODEL_POSITION:Vector3 = Vector3(-200.0, 0.0, 300.0)
const WIRE_FROM:Vector3 = Vector3(-300.0, 8.0, 200.0)
const WIRE_TO:Vector3 = Vector3(-310.0, 8.0, 260.0)
## A sector of the far side of the world, a block away from the origin's
const FAR_CELL_OF_GRID:Vector2i = Vector2i(437 - 300, 700)


## A source whose sectors are in SECTORS_DIR, available at once or when made so (make_available())
class TestSource extends MaszynaWorldSectorSource:
    var immediate:bool = true
    var requested:PackedStringArray = []
    var released:PackedStringArray = []

    func sector_request(id:String) -> void:
        requested.append(id)
        if immediate:
            sector_available.emit(id)

    func sector_release(id:String) -> void:
        released.append(id)

    func sector_get_path(id:String, file:MaszynaWorld.SectorFile) -> String:
        return SECTORS_DIR.path_join(id + MaszynaWorld.SECTOR_FILE_SUFFIXES[file])

    func make_available(id:String) -> void:
        sector_available.emit(id)


func test_sectors_tracks_are_moved_to_their_place_and_joined() -> void:
    var world:MaszynaWorld = _world()
    add_child(world)
    await world.load()

    var east:RID = TrackServer.track_get_rid_by_name("east")
    var west:RID = TrackServer.track_get_rid_by_name("west")
    assert_true(east.is_valid() and west.is_valid())
    # the origin's sector: its corner is the world's origin
    assert_almost_eq(TrackServer.track_get_endpoints(east)[1], Vector3(0.0, 0.0, TRACK_Z), Vector3.ONE * TOLERANCE)
    # the western sector's corner lies a sector's side west (+X) of it
    assert_almost_eq(TrackServer.track_get_endpoints(west)[0], Vector3(0.0, 0.0, TRACK_Z), Vector3.ONE * TOLERANCE)
    assert_almost_eq(
            TrackServer.track_get_endpoints(west)[1], Vector3(TRACK_LENGTH, 0.0, TRACK_Z), Vector3.ONE * TOLERANCE)
    var connections:Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(east, TrackServer.CURVE1_P2)
    assert_eq(connections.size(), 1)
    assert_eq(connections[0].track_rid, west)

    world.free()
    assert_false(TrackServer.track_exists(east))
    assert_false(TrackServer.track_exists(west))


func test_sectors_roads_are_announced_built_and_before_they_are_freed() -> void:
    DirAccess.make_dir_recursive_absolute(SECTORS_DIR)
    var camera:Camera3D = Camera3D.new()
    add_child_autoqfree(camera)
    camera.global_position = Vector3(-500.0, 0.0, 500.0)
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var world:MaszynaWorld = MaszynaWorld.new()
    world.autoload = false
    world.world_name = "roads"
    world.origin = Vector2(ORIGIN_CELL) * MaszynaWorld.SECTOR_SIZE
    var sector:MaszynaWorldSector = _sector(
            ORIGIN_CELL, "road", Vector3(-TRACK_LENGTH, 0.0, TRACK_Z), Vector3(0.0, 0.0, TRACK_Z))
    var roads:MaszynaTracksNode = sector.get_child(0)
    roads.type = TrackServer.GROUP_ROAD
    roads.tracks[0].type = TrackServer.TRACK_ROAD
    var source:TestSource = TestSource.new()
    var path:String = source.sector_get_path(sector.id, MaszynaWorld.SectorFile.ROADS)
    assert_eq(ResourceSaver.save(world.add_sector(sector), path), OK)
    world.sector_ids = PackedStringArray([sector.id])
    sector.free()
    world.sector_source = source
    var built:Array[RID] = []
    var freeing:Array[RID] = []
    var standing:Array[bool] = []
    world.sector_roads_built.connect(func(_id:String, track_rids:Array[RID]) -> void: built.append_array(track_rids))
    world.sector_roads_freeing.connect(func(_id:String, track_rids:Array[RID]) -> void:
            freeing.append_array(track_rids)
            standing.append(TrackServer.track_exists(track_rids[0])))
    add_child(world)
    await world.load()

    assert_eq(built.size(), 1, "the sector's road is announced")
    assert_eq(TrackServer.track_get_group(built[0]), TrackServer.GROUP_ROAD)
    world.free()
    assert_eq(freeing, built, "the road is announced going with its sector")
    assert_eq(standing, [true] as Array[bool], "while it still stands")
    assert_false(TrackServer.track_exists(built[0]))
    SceneryStreamingServer.streaming_set_camera(0)
    DirAccess.remove_absolute(path)


func test_world_is_named_and_loads_nothing_without_sectors() -> void:
    var world:MaszynaWorld = MaszynaWorld.new()
    world.autoload = false
    world.world_name = "empty"
    add_child(world)
    await world.load()
    assert_eq(world.get_scenery_name(), "empty")
    assert_eq(world.get_vehicles().size(), 0)
    world.free()


func test_mesh_is_streamed_where_its_transform_places_it_and_freed() -> void:
    var mesh:ArrayMesh = ArrayMesh.new()
    var arrays:Array = []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([Vector3.ZERO, Vector3(0.0, 0.0, 1.0), Vector3(1.0, 0.0, 0.0)])
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    # the sector's corner a chunk and a half east (+X) and north (+Z) of the origin
    var corner:Vector3 = Vector3(1.5, 0.0, 1.5) * STREAMING_CHUNK
    var chunk:Vector2i = Vector2i(1, 1)
    var before:int = SceneryStreamingServer.chunk_get_rids(chunk).size()
    var placed:RID = SceneryMeshRenderingServer.mesh_create(
            mesh, Transform3D(Basis.IDENTITY, corner), 0.0, get_viewport().world_3d.scenario)
    assert_true(placed.is_valid())
    assert_eq(SceneryStreamingServer.chunk_get_rids(chunk).size(), before + 1)
    SceneryMeshRenderingServer.mesh_free(placed)
    assert_eq(SceneryStreamingServer.chunk_get_rids(chunk).size(), before)


func test_provider_hands_over_what_of_a_loaded_sector_stands_in_a_cell() -> void:
    DirAccess.make_dir_recursive_absolute(SECTORS_DIR)
    var sector:MaszynaWorldSector = MaszynaWorldSector.new()
    sector.id = _sector_id(ORIGIN_CELL)
    var layer:MaszynaMeshesNode = MaszynaMeshesNode.new()
    layer.range_end = MESH_RANGE
    layer.meshes.append(_mesh(NEAR_MESH))
    layer.meshes.append(_mesh(FAR_MESH))
    var decorations:MaszynaModelsNode = MaszynaModelsNode.new()
    decorations.models.append(_model("", MODEL_POSITION))
    var wires:MaszynaLinesNode = MaszynaLinesNode.new()
    var wire:MaszynaLinesData = MaszynaLinesData.new()
    wire.points = PackedVector3Array([WIRE_FROM, WIRE_TO])
    wires.lines.append(wire)
    for node:Node in [layer, decorations, wires]:
        sector.add_child(node)
        node.owner = sector
    var scene:PackedScene = PackedScene.new()
    scene.pack(sector)
    sector.free()
    var source:TestSource = TestSource.new()
    var path:String = source.sector_get_path(_sector_id(ORIGIN_CELL), MaszynaWorld.SectorFile.DRAWN)
    assert_eq(ResourceSaver.save(scene, path), OK)
    var provider:MaszynaWorldSectorProvider = MaszynaWorldSectorProvider.new(
            Vector2(ORIGIN_CELL) * MaszynaWorld.SECTOR_SIZE, PackedStringArray([_sector_id(ORIGIN_CELL)]), source)

    # the origin's sector lies west (-X) and north (+Z) of the origin, across 2x2 cells
    assert_eq(provider.get_chunk_cells().size(), 4)
    assert_true(NEAR_CELL in provider.get_chunk_cells())
    assert_false(Vector2i(0, 0) in provider.get_chunk_cells())
    assert_eq(provider.chunk_load(NEAR_CELL, SceneryStreamingProvider.CONTENT_MESHES).size(), 0, "read before it was loaded")

    provider.sector_set_loaded(_sector_id(ORIGIN_CELL), true)
    var near:Array = provider.chunk_load(NEAR_CELL, SceneryStreamingProvider.CONTENT_MESHES)
    assert_eq(near.size(), 1)
    var placement:MaszynaMeshPlacement = near[0]
    assert_almost_eq(placement.mesh.get_aabb().get_center(), NEAR_MESH, Vector3.ONE * TOLERANCE)
    assert_eq(placement.transform.origin, Vector3.ZERO)
    assert_eq(placement.range_end, MESH_RANGE)
    assert_eq(provider.chunk_load(FAR_CELL, SceneryStreamingProvider.CONTENT_MESHES).size(), 1)
    var models:Array = provider.chunk_load(NEAR_CELL, SceneryStreamingProvider.CONTENT_MODELS)
    assert_eq(models.size(), 1)
    assert_almost_eq((models[0] as SceneryModelPlacement).transform.origin, MODEL_POSITION, Vector3.ONE * TOLERANCE)
    var lines:Array = provider.chunk_load(NEAR_CELL, SceneryStreamingProvider.CONTENT_LINES)
    assert_eq(lines.size(), 1)
    assert_eq((lines[0] as MaszynaLinesGeometry).get_node_count(), 1)
    DirAccess.remove_absolute(path)


func test_a_sector_keeps_only_what_is_drawn_once_added_to_the_world() -> void:
    var world:MaszynaWorld = MaszynaWorld.new()
    world.origin = Vector2(ORIGIN_CELL) * MaszynaWorld.SECTOR_SIZE
    var sector:MaszynaWorldSector = _sector(ORIGIN_CELL, "east", Vector3(-TRACK_LENGTH, 0.0, TRACK_Z), Vector3(0.0, 0.0, TRACK_Z))
    var models:MaszynaModelsNode = MaszynaModelsNode.new()
    models.models.append(_model("sig_1", MODEL_POSITION))
    models.models.append(_model("", MODEL_POSITION))
    sector.add_child(models)
    var wires:MaszynaLinesNode = MaszynaLinesNode.new()
    wires.lines.append(MaszynaLinesData.new())
    sector.add_child(wires)

    world.add_sector(sector)
    assert_eq(world.scenery.tracks.size(), 1)
    assert_eq(world.scenery.models.size(), 1, "the signal is the simulation's")
    assert_eq(world.scenery.models[0].name, "sig_1")
    assert_eq(world.scenery.lines.size(), 0, "the wires are only drawn")
    assert_false(sector.get_children().any(func(node:Node) -> bool: return node is MaszynaTracksNode))
    assert_eq(models.models.size(), 1, "the decoration is drawn with the sector")
    assert_eq(wires.lines.size(), 1)
    sector.free()
    world.free()


func test_a_sector_is_in_the_block_of_its_column_and_row() -> void:
    assert_eq(MaszynaWorld.block_of("PL92-E0437-N0700"), "PL92-B8-E0054-N0087")
    assert_eq(MaszynaWorld.block_of("PL92-E0432-N0703"), "PL92-B8-E0054-N0087")
    assert_eq(MaszynaWorld.block_of("PL92-E0431-N0704"), "PL92-B8-E0053-N0088")
    assert_eq(MaszynaWorld.sector_path("w", "PL92-E0437-N0700", MaszynaWorld.SectorFile.TERRAIN),
            "res://worlds/w/blocks/PL92-B8-E0054-N0087/sectors/PL92-E0437-N0700.terrain.res")


func test_the_sectors_in_reach_are_asked_for_and_loaded_once_available() -> void:
    var camera:Camera3D = Camera3D.new()
    add_child_autoqfree(camera)
    camera.global_position = Vector3(-500.0, 0.0, 500.0)
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var source:TestSource = TestSource.new()
    source.immediate = false
    var sectors:MaszynaWorldSectors = MaszynaWorldSectors.new()
    # arrays, not packed ones: the lambdas take a packed array by value
    var loaded:Array[String] = []
    var unloaded:Array[String] = []
    sectors.sector_loaded.connect(func(id:String) -> void: loaded.append(id))
    sectors.sector_unloaded.connect(func(id:String) -> void: unloaded.append(id))
    var near:String = _sector_id(ORIGIN_CELL)
    var far:String = _sector_id(FAR_CELL_OF_GRID)
    sectors.start(Vector2(ORIGIN_CELL) * MaszynaWorld.SECTOR_SIZE, PackedStringArray([near, far]), source)

    assert_eq(source.requested, PackedStringArray([near]), "only the sector in reach is asked for")
    assert_eq(loaded.size(), 0, "loaded before its source had it")
    source.make_available(near)
    assert_eq(loaded, [near] as Array[String])
    assert_true(sectors.is_sector_loaded(near))

    # the far sector's square is 300 sectors west (+X) of the origin's
    camera.global_position = Vector3(300.0 * MaszynaWorld.SECTOR_SIZE - 500.0, 0.0, 500.0)
    sectors.update()
    assert_eq(source.released, PackedStringArray([near]))
    assert_eq(unloaded, [near] as Array[String])
    assert_true(far in source.requested)
    sectors.stop()
    SceneryStreamingServer.streaming_set_camera(0)


func test_mesh_placement_is_drawn_where_it_stands_and_freed() -> void:
    var mesh:ArrayMesh = _mesh(Vector3.ZERO)
    mesh.surface_set_material(0, StandardMaterial3D.new())
    var corner:Vector3 = Vector3(1.5, 0.0, 1.5) * STREAMING_CHUNK
    var chunk:Vector2i = Vector2i(1, 1)
    var before:int = SceneryStreamingServer.chunk_get_rids(chunk).size()
    var placed:RID = MaszynaMeshPlacement.create_mesh(
            MaszynaMeshPlacement.new(mesh, Transform3D(Basis.IDENTITY, corner), MESH_RANGE), get_viewport().world_3d.scenario)
    assert_true(placed.is_valid())
    assert_eq(SceneryStreamingServer.chunk_get_rids(chunk).size(), before + 1)
    SceneryMeshRenderingServer.mesh_free(placed)
    assert_eq(SceneryStreamingServer.chunk_get_rids(chunk).size(), before)


## A world of two sectors, a track each, meeting on their edge (E = the origin's)
func _world() -> MaszynaWorld:
    var world:MaszynaWorld = MaszynaWorld.new()
    world.autoload = false
    world.world_name = "sectors"
    world.origin = Vector2(ORIGIN_CELL) * MaszynaWorld.SECTOR_SIZE
    # in its sector's coordinates X runs west from the corner, so the edge with the western
    # neighbour is X 0 here and X -SECTOR_SIZE there
    for sector:MaszynaWorldSector in [
            _sector(ORIGIN_CELL, "east", Vector3(-TRACK_LENGTH, 0.0, TRACK_Z), Vector3(0.0, 0.0, TRACK_Z)),
            _sector(WEST_CELL, "west", Vector3(-MaszynaWorld.SECTOR_SIZE, 0.0, TRACK_Z),
                    Vector3(-MaszynaWorld.SECTOR_SIZE + TRACK_LENGTH, 0.0, TRACK_Z))]:
        world.add_sector(sector)
        sector.free()
    return world


## A model of the game, named `model_name` ("" for none), standing at `position`
func _model(model_name:String, position:Vector3) -> MaszynaModelData:
    var model:MaszynaModelData = MaszynaModelData.new()
    model.name = model_name
    model.data_path = "models/test"
    model.model_filename = "test"
    model.position = position
    return model


func _sector_id(cell:Vector2i) -> String:
    return "PL92-E%04d-N%04d" % [cell.x, cell.y]


## A triangle whose bounds have their centre at `centre`
func _mesh(centre:Vector3) -> ArrayMesh:
    var mesh:ArrayMesh = ArrayMesh.new()
    var arrays:Array = []
    arrays.resize(Mesh.ARRAY_MAX)
    arrays[Mesh.ARRAY_VERTEX] = PackedVector3Array([
            centre + Vector3(-1.0, 0.0, -1.0), centre + Vector3(1.0, 0.0, -1.0), centre + Vector3(0.0, 0.0, 1.0)])
    mesh.add_surface_from_arrays(Mesh.PRIMITIVE_TRIANGLES, arrays)
    return mesh


func _sector(cell:Vector2i, track_name:String, p1:Vector3, p2:Vector3) -> MaszynaWorldSector:
    var sector:MaszynaWorldSector = MaszynaWorldSector.new()
    sector.id = _sector_id(cell)
    var tracks:MaszynaTracksNode = MaszynaTracksNode.new()
    var track:MaszynaTrackData = MaszynaTrackData.new()
    track.type = TrackServer.TRACK_NORMAL
    track.track_name = track_name
    track.width = TRACK_WIDTH
    track.curve = TrackCurve.new()
    track.curve.p1 = p1
    track.curve.p2 = p2
    tracks.tracks.append(track)
    sector.add_child(tracks)
    return sector
