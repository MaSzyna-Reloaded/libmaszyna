extends MaszynaGutTest

## The rail profile a track is drawn with (models/tory/railprofile_default.txt) comes from the
## fixtures, not the game directory
const FIXTURES_GAME_DIR: String = "res://tests/fixtures"
## Frames given to the streaming to build a road cell in the camera's range
const STREAMING_FRAMES: int = 60

var created_tracks: Array[TrackNormal3D] = []
var created_track_rids: Array[RID] = []
var _previous_game_dir: String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)
    for track: TrackNormal3D in created_tracks:
        remove_child(track)
        track.queue_free()
    created_tracks.clear()
    for track_rid: RID in created_track_rids:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    created_track_rids.clear()
    TrackServer.topology_rebuild()
    await wait_idle_frames(2)


func test_switch_is_right_returns_true_for_right_switch() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "right_switch",
        _curve(
            Vector3(0.0, 0.0, 0.0),
            Vector3(0.0, 0.0, -20.0)
        ),
        _curve(
            Vector3(0.0, 0.0, 0.0),
            Vector3(6.0, 0.0, -20.0),
            Vector3(0.0, 0.0, -16.667),
            Vector3(0.0, 0.0, 16.667)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "right switch should register in TrackServer")
    assert_true(TrackServer.track_is_switch_right(track_rid))


func test_switch_is_right_returns_false_for_left_switch() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "left_switch",
        _curve(
            Vector3(0.0, 0.0, 0.0),
            Vector3(0.0, 0.0, -20.0)
        ),
        _curve(
            Vector3(0.0, 0.0, 0.0),
            Vector3(-6.0, 0.0, -20.0),
            Vector3(0.0, 0.0, -16.667),
            Vector3(0.0, 0.0, 16.667)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "left switch should register in TrackServer")
    assert_false(TrackServer.track_is_switch_right(track_rid))


func test_switch_is_right_returns_true_for_smoothed_demo3d_right_switch() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "smoothed_right_switch",
        _curve(
            Vector3(-19.19, 0.0, 130.0),
            Vector3(-19.19, 0.0, 70.0)
        ),
        _curve(
            Vector3(-19.19, 0.0, 130.0),
            Vector3(-12.53, 0.0, 80.0),
            Vector3(0.0, 0.0, -16.667),
            Vector3(0.0, 0.0, 16.667)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "smoothed right switch should register in TrackServer")
    assert_true(TrackServer.track_is_switch_right(track_rid))


func test_switch_is_right_returns_false_for_smoothed_demo3d_left_switch() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "smoothed_left_switch",
        _curve(
            Vector3(19.19, 0.0, 130.0),
            Vector3(19.19, 0.0, 70.0)
        ),
        _curve(
            Vector3(19.19, 0.0, 130.0),
            Vector3(12.53, 0.0, 80.0),
            Vector3(0.0, 0.0, -16.667),
            Vector3(0.0, 0.0, 16.667)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "smoothed left switch should register in TrackServer")
    assert_false(TrackServer.track_is_switch_right(track_rid))


func test_switch_is_right_returns_true_for_right_switch_with_common_end() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "right_switch_common_end",
        _curve(
            Vector3(0.0, 0.0, -20.0),
            Vector3(0.0, 0.0, 0.0)
        ),
        _curve(
            Vector3(6.0, 0.0, -20.0),
            Vector3(0.0, 0.0, 0.0)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "right switch with common end should register in TrackServer")
    assert_true(TrackServer.track_is_switch_right(track_rid))


func test_switch_is_right_returns_false_for_left_switch_with_common_end() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "left_switch_common_end",
        _curve(
            Vector3(0.0, 0.0, -20.0),
            Vector3(0.0, 0.0, 0.0)
        ),
        _curve(
            Vector3(-6.0, 0.0, -20.0),
            Vector3(0.0, 0.0, 0.0)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "left switch with common end should register in TrackServer")
    assert_false(TrackServer.track_is_switch_right(track_rid))


func test_switch_is_right_returns_false_for_demo3d_left_switch() -> void:
    var track: TrackSwitch3D = await _create_switch_track(
        "demo3d_left_switch",
        _curve(
            Vector3(-25.8, 0.0, 136.0),
            Vector3(-25.8, 0.0, 100.0)
        ),
        _curve(
            Vector3(-25.8, 0.0, 136.0),
            Vector3(-32.5, 0.0, 100.0),
            Vector3(0.0, 0.0, -11.205),
            Vector3(0.0, 0.0, 10.675)
        )
    )
    var track_rid: RID = TrackServer.track_get_rid_by_name(track.track_name)

    assert_true(track_rid.is_valid(), "demo3d left switch should register in TrackServer")
    assert_false(TrackServer.track_is_switch_right(track_rid))


func test_get_unique_endpoint_connection_returns_null_for_ambiguous_connections() -> void:
    var previous_rid: RID = TrackServer.track_create()
    created_track_rids.append(previous_rid)
    TrackServer.track_update_curves(
        previous_rid,
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null
    )
    TrackServer.track_update(previous_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    var source_rid: RID = TrackServer.track_create()
    created_track_rids.append(source_rid)
    TrackServer.track_update_curves(
        source_rid,
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null
    )
    TrackServer.track_update(source_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    var first_neighbor_rid: RID = TrackServer.track_create()
    created_track_rids.append(first_neighbor_rid)
    TrackServer.track_update_curves(
        first_neighbor_rid,
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null
    )
    TrackServer.track_update(first_neighbor_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    var second_neighbor_rid: RID = TrackServer.track_create()
    created_track_rids.append(second_neighbor_rid)
    TrackServer.track_update_curves(
        second_neighbor_rid,
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 10.0)),
        null
    )
    TrackServer.track_update(second_neighbor_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    TrackServer.topology_rebuild()

    assert_eq(
        MaszynaTrackRenderingBackend.unique_endpoint_connection(
            source_rid,
            TrackServer.CURVE1_P2
        ),
        null
    )

    var previous_connection: TrackEndpointRef = MaszynaTrackRenderingBackend.unique_endpoint_connection(
        previous_rid,
        TrackServer.CURVE1_P2
    )

    assert_not_null(previous_connection)
    assert_eq(previous_connection.track_rid, source_rid)
    assert_eq(previous_connection.endpoint_index, TrackServer.CURVE1_P1)


func test_normal_track_does_not_build_secondary_rail_from_material2() -> void:
    var track: TrackNormal3D = await _create_normal_track(
        "normal_track_with_ballast_material2",
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0))
    )
    _describe(track, _scn_look(TrackServer.TRACK_NORMAL, "rail_screw_used1", "1435mm/tpbps-new2", 4.0, 0.0, 0.0, 0.0))

    var meshes: Dictionary = _backend().track_get_meshes(track._track_render_rid)

    assert_not_null(meshes.get(MaszynaTrackRenderingBackend.SLOT_PRIMARY_RAIL))
    assert_null(meshes.get(MaszynaTrackRenderingBackend.SLOT_SECONDARY_RAIL))
    assert_not_null(meshes.get(MaszynaTrackRenderingBackend.SLOT_TRACKBED))


## A road along +X: its surface as wide as the road at its points' level, its shoulders flat for
## tex_width then down by tex_height over tex_slope (Track.cpp:3069-3141)
func test_road_has_its_surface_and_shoulders() -> void:
    var track: TrackNormal3D = await _create_road_track("road_with_shoulders", 6.0, "road_asphalt", "road_side", 0.2, 0.5, 0.5)
    var meshes: Dictionary = _backend().track_get_meshes(track._track_render_rid)

    var surface: AABB = (meshes[MaszynaTrackRenderingBackend.SLOT_PRIMARY_RAIL] as Mesh).get_aabb()
    assert_almost_eq(surface.size.z, 6.0, 0.001, "the surface is as wide as the road")
    assert_almost_eq(surface.size.y, 0.0, 0.001, "a flat road's surface lies at its points' level")
    var sides: AABB = (meshes[MaszynaTrackRenderingBackend.SLOT_TRACKBED] as Mesh).get_aabb()
    assert_almost_eq(sides.size.z, 6.0 + 2.0 * (0.5 + 0.5), 0.001, "a shoulder on each side")
    assert_almost_eq(sides.position.y, -0.2, 0.001, "the shoulders go down by tex_height")
    assert_null(meshes.get(MaszynaTrackRenderingBackend.SLOT_SECONDARY_RAIL), "a road has no rails")


## A negative tex_height is a curb that high with a sidewalk behind it, tex_slope wide on the right
## of the road's direction and tex_width on its left (Track.cpp:3142-3254)
func test_road_with_a_negative_texture_height_has_curbs_and_sidewalks() -> void:
    var track: TrackNormal3D = await _create_road_track("road_with_sidewalks", 6.0, "road_asphalt", "road_sidewalk", -0.15, 2.0, 1.5)
    var meshes: Dictionary = _backend().track_get_meshes(track._track_render_rid)

    var sides: AABB = (meshes[MaszynaTrackRenderingBackend.SLOT_TRACKBED] as Mesh).get_aabb()
    assert_almost_eq(sides.position.y, 0.0, 0.001, "the curb starts at the road's level")
    assert_almost_eq(sides.end.y, 0.15, 0.001, "the sidewalk is as high as the curb")
    assert_almost_eq(sides.end.z, 3.0 + 1.5, 0.001, "tex_slope wide on the right")
    assert_almost_eq(sides.position.z, -3.0 - 2.0, 0.001, "tex_width wide on the left")


func test_road_without_a_side_material_has_no_sides() -> void:
    var track: TrackNormal3D = await _create_road_track("road_without_sides", 4.0, "road_asphalt", "", 0.2, 0.5, 0.5)
    var meshes: Dictionary = _backend().track_get_meshes(track._track_render_rid)

    assert_not_null(meshes.get(MaszynaTrackRenderingBackend.SLOT_PRIMARY_RAIL))
    assert_null(meshes.get(MaszynaTrackRenderingBackend.SLOT_TRACKBED))


## A plain track makes the instances it draws - its rails and its trackbed - and no blades
func test_plain_track_makes_only_the_instances_it_draws() -> void:
    var track: TrackNormal3D = await _create_normal_track(
        "plain_track_instances",
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0))
    )
    _describe(track, _scn_look(TrackServer.TRACK_NORMAL, "rail_screw_used1", "1435mm/tpbps-new2", 4.0, 0.0, 0.0, 0.0))
    var instances: Dictionary = _backend().track_get_instances(track._track_render_rid)

    assert_true(instances.get(MaszynaTrackRenderingBackend.SLOT_PRIMARY_RAIL, RID()).is_valid())
    assert_true(instances.get(MaszynaTrackRenderingBackend.SLOT_TRACKBED, RID()).is_valid())
    assert_false(instances.get(MaszynaTrackRenderingBackend.SLOT_SECONDARY_RAIL, RID()).is_valid(), "no second path")
    assert_false(instances.get(MaszynaTrackRenderingBackend.SLOT_PRIMARY_BLADE, RID()).is_valid(), "no blades")
    assert_false(instances.get(MaszynaTrackRenderingBackend.SLOT_SECONDARY_BLADE, RID()).is_valid(), "no blades")


## Streamed roads near each other are drawn by their cell: one instance, one mesh with a surface per
## material, and no instance of the roads' own - built as the streaming camera comes within its range
func test_streamed_roads_are_one_mesh_of_their_cell() -> void:
    var camera: Camera3D = Camera3D.new()
    add_child_autoqfree(camera)
    SceneryStreamingServer.streaming_set_camera(camera.get_instance_id())
    var backend: MaszynaTrackRenderingBackend = _backend()
    var render_rids: Array[RID] = []
    for i in 2:
        var track_rid: RID = TrackServer.track_create()
        created_track_rids.append(track_rid)
        var render_rid: RID = TrackRenderingServer.track_create(track_rid)
        render_rids.append(render_rid)
        TrackRenderingServer.track_set_scenario(render_rid, get_tree().root.world_3d.scenario)
        TrackServer.track_update_curves(track_rid, _curve(Vector3(10.0 * i, 0.0, 5.0), Vector3(10.0 * i + 10.0, 0.0, 5.0)), null)
        TrackServer.track_update(track_rid, TrackServer.TRACK_ROAD, "streamed_road_%d" % i, 6.0)
        TrackRenderingServer.track_set_description(
                render_rid, _scn_look(TrackServer.TRACK_ROAD, "road_asphalt", "road_side", 6.0, 0.2, 0.5, 0.5))
        TrackRenderingServer.track_stream(render_rid)
    var cell_rid: RID = backend.track_get_road_cell(render_rids[0])

    assert_true(cell_rid.is_valid(), "a streamed road goes into a cell")
    assert_eq(backend.track_get_road_cell(render_rids[1]), cell_rid, "roads of one square share their cell")
    await wait_streaming(STREAMING_FRAMES)
    assert_true(backend.road_cell_get_instance(cell_rid).is_valid())
    var mesh: ArrayMesh = backend.road_cell_get_mesh(cell_rid)
    assert_eq(mesh.get_surface_count(), 2, "the surface and the shoulders, both roads in each")
    assert_almost_eq(mesh.get_aabb().size.x, 20.0, 0.001, "both roads in the mesh")
    assert_true(backend.track_get_instances(render_rids[0]).is_empty(), "no instance of its own")

    for render_rid: RID in render_rids:
        TrackRenderingServer.track_free(render_rid)
    assert_false(backend.road_cell_exists(cell_rid), "the cell goes with its last road")
    SceneryStreamingServer.streaming_set_camera(0)


## A track of no .scn is drawn with the look its features map to (maszyna_track_looks.tres)
func test_a_described_road_is_drawn_with_its_features_look() -> void:
    var track: TrackNormal3D = await _create_normal_track(
        "described_road", _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var road: TrackData = TrackData.new()
    road.kind = TrackServer.TRACK_ROAD
    road.width = 6.0
    road.surface = TrackData.SURFACE_ASPHALT
    road.centre_line = true
    road.shoulder = true
    TrackRenderingServer.track_set_description(track._track_render_rid, road)
    var look: Dictionary = _backend().track_get_look(track._track_render_rid)

    assert_eq(look["material1"], "asphalt1_dashedwhite", "an asphalt road with a centre line")
    assert_eq(look["material2"], "asphalt1_side1", "its shoulder")
    assert_almost_eq(look["tex_length"], 6.0, 0.001, "a road's texture as long as it is wide")


func test_mesh_curve_sampling_matches_maszyna_geometry_fidelity() -> void:
    var straight: TrackCurve = _curve(
        Vector3.ZERO,
        Vector3(100.0, 0.0, 0.0)
    )
    var curved: TrackCurve = _curve(
        Vector3.ZERO,
        Vector3(100.0, 0.0, 0.0),
        Vector3(30.0, 0.0, 0.0),
        Vector3(-30.0, 0.0, 0.0)
    )
    var radius_curve: TrackCurve = _curve(
        Vector3.ZERO,
        Vector3(100.0, 0.0, 0.0)
    )
    radius_curve.radius = 200.0

    assert_almost_eq(MaszynaTrackRenderingBackend.mesh_curve_bake_interval(straight), 10.0, 0.001)
    assert_almost_eq(MaszynaTrackRenderingBackend.mesh_curve_bake_interval(curved), 10.0, 0.001)
    assert_almost_eq(MaszynaTrackRenderingBackend.mesh_curve_bake_interval(radius_curve), 4.0, 0.001)


func test_track_render_lookup_is_removed_with_render_track() -> void:
    var track_rid: RID = TrackServer.track_create()
    created_track_rids.append(track_rid)
    var track_render_rid: RID = TrackRenderingServer.track_create(track_rid)

    assert_eq(TrackRenderingServer.track_get_render_rid(track_rid), track_render_rid)

    TrackRenderingServer.track_free(track_render_rid)

    assert_false(TrackRenderingServer.track_get_render_rid(track_rid).is_valid())


func test_switch_blade_layout_uses_curve1_left_and_curve2_right_for_right_switch() -> void:
    var layout: Dictionary = MaszynaTrackRenderingBackend.switch_blade_layout(true, -0.05, 0.0, TrackServer.switch_max_offset)

    assert_true(layout["primary_blade_mirrored"], "right switch should place curve1 blade on the left rail")
    assert_false(layout["secondary_blade_mirrored"], "right switch should place curve2 blade on the right rail")
    assert_almost_eq(layout["primary_blade_offset"], 0.0, 0.001)
    assert_almost_eq(layout["secondary_blade_offset"], -0.15, 0.001)


func test_switch_blade_layout_uses_curve1_right_and_curve2_left_for_left_switch() -> void:
    var layout: Dictionary = MaszynaTrackRenderingBackend.switch_blade_layout(false, -0.05, 0.0, TrackServer.switch_max_offset)

    assert_false(layout["primary_blade_mirrored"], "left switch should place curve1 blade on the right rail")
    assert_true(layout["secondary_blade_mirrored"], "left switch should place curve2 blade on the left rail")
    assert_almost_eq(layout["primary_blade_offset"], 0.0, 0.001)
    assert_almost_eq(layout["secondary_blade_offset"], 0.15, 0.001)


## MaSzyna's track rendering backend, as TrackRenderingServer has it
func _backend() -> MaszynaTrackRenderingBackend:
    return instance_from_id(TrackRenderingServer.backend_get(&"maszyna")) as MaszynaTrackRenderingBackend


func _create_normal_track(
    track_name: String,
    curve1: TrackCurve
) -> TrackNormal3D:
    var track: TrackNormal3D = TrackNormal3D.new()
    track.track_name = track_name
    track.curve = curve1
    add_child(track)
    created_tracks.append(track)
    await wait_idle_frames(2)
    return track


func _create_road_track(track_name: String, width: float, material1: String, material2: String,
        tex_height: float, tex_width: float, tex_slope: float) -> TrackNormal3D:
    var track: TrackNormal3D = TrackNormal3D.new()
    track.type = TrackServer.TRACK_ROAD
    track.track_name = track_name
    track.width = width
    track.curve = _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0))
    add_child(track)
    created_tracks.append(track)
    await wait_idle_frames(2)
    track._process_dirty(0.0)
    _describe(track, _scn_look(TrackServer.TRACK_ROAD, material1, material2, width, tex_height, tex_width, tex_slope))
    return track


## A .scn track's look: its textures and section, as MaszynaTrackRenderingBackend draws a .scn
func _scn_look(kind: TrackServer.TrackType, material1: String, material2: String, tex_length: float,
        tex_height: float, tex_width: float, tex_slope: float) -> MaszynaTrackData:
    var look: MaszynaTrackData = MaszynaTrackData.new()
    look.kind = kind
    look.material1 = material1
    look.material2 = material2
    look.tex_length = tex_length
    look.tex_height = tex_height
    look.tex_width = tex_width
    look.tex_slope = tex_slope
    return look


## The node's track drawn with [param look] (a node describes only what the track is)
func _describe(track: TrackNormal3D, look: MaszynaTrackData) -> void:
    TrackRenderingServer.track_set_description(track._track_render_rid, look)
    TrackRenderingServer.track_rebuild(track._track_render_rid)


func _create_switch_track(
    track_name: String,
    curve1: TrackCurve,
    curve2: TrackCurve
) -> TrackSwitch3D:
    var track: TrackSwitch3D = TrackSwitch3D.new()
    track.track_name = track_name
    track.curve = curve1
    track.diverging_curve = curve2
    add_child(track)
    created_tracks.append(track)
    await wait_idle_frames(2)
    return track


func _curve(
    p1: Vector3,
    p2: Vector3,
    c1: Vector3 = Vector3.ZERO,
    c2: Vector3 = Vector3.ZERO
) -> TrackCurve:
    var curve: TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    curve.c1 = c1
    curve.c2 = c2
    return curve
