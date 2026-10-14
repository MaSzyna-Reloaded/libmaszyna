extends MaszynaGutTest

## A world (MaszynaWorld) composed of its sectors: each sector's tracks moved from its corner to its
## place, registered with TrackServer and joined across the sectors' edge, and freed with the world.

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


## A world of two sectors, a track each, meeting on their edge (E = the origin's)
func _world() -> MaszynaWorld:
    var world:MaszynaWorld = MaszynaWorld.new()
    world.autoload = false
    world.world_name = "sectors"
    world.origin = Vector2(ORIGIN_CELL) * MaszynaWorld.SECTOR_SIZE
    # in its sector's coordinates X runs west from the corner, so the edge with the western
    # neighbour is X 0 here and X -SECTOR_SIZE there
    world.sectors.append(_sector(ORIGIN_CELL, "east",
            Vector3(-TRACK_LENGTH, 0.0, TRACK_Z), Vector3(0.0, 0.0, TRACK_Z)))
    world.sectors.append(_sector(WEST_CELL, "west",
            Vector3(-MaszynaWorld.SECTOR_SIZE, 0.0, TRACK_Z),
            Vector3(-MaszynaWorld.SECTOR_SIZE + TRACK_LENGTH, 0.0, TRACK_Z)))
    return world


func _sector(cell:Vector2i, track_name:String, p1:Vector3, p2:Vector3) -> PackedScene:
    var sector:MaszynaWorldSector = MaszynaWorldSector.new()
    sector.id = "PL92-E%04d-N%04d" % [cell.x, cell.y]
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
    tracks.owner = sector
    var scene:PackedScene = PackedScene.new()
    scene.pack(sector)
    sector.free()
    return scene
