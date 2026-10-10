extends MaszynaGutTest

## The rail profile a track is drawn with (models/tory/railprofile_default.txt) comes from the
## fixtures, not the game directory
const FIXTURES_GAME_DIR: String = "res://tests/fixtures"

var created_tracks: Array[TrackNormal3D] = []
var created_track_rids: Array[RID] = []
var _previous_game_dir: String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)
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
    assert_true(TrackServer.switch_is_right(track_rid))


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
    assert_false(TrackServer.switch_is_right(track_rid))


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
    assert_true(TrackServer.switch_is_right(track_rid))


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
    assert_false(TrackServer.switch_is_right(track_rid))


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
    assert_true(TrackServer.switch_is_right(track_rid))


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
    assert_false(TrackServer.switch_is_right(track_rid))


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
    assert_false(TrackServer.switch_is_right(track_rid))


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
        TrackRenderingServer._get_unique_endpoint_connection(
            source_rid,
            TrackServer.CURVE1_P2
        ),
        null
    )

    var previous_connection: TrackEndpointRef = TrackRenderingServer._get_unique_endpoint_connection(
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
    track.material1 = "rail_screw_used1"
    track.material2 = "1435mm/tpbps-new2"
    track._process_dirty(0.0)

    var state: Variant = TrackRenderingServer._tracks[track._track_render_rid]

    assert_not_null(state.primary_rail_mesh)
    assert_null(state.secondary_rail_mesh)
    assert_not_null(state.trackbed_mesh)


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

    assert_almost_eq(TrackRenderingServer._get_mesh_curve_bake_interval(straight), 10.0, 0.001)
    assert_almost_eq(TrackRenderingServer._get_mesh_curve_bake_interval(curved), 10.0, 0.001)
    assert_almost_eq(TrackRenderingServer._get_mesh_curve_bake_interval(radius_curve), 4.0, 0.001)


func test_track_render_lookup_is_removed_with_render_track() -> void:
    var track_rid: RID = TrackServer.track_create()
    created_track_rids.append(track_rid)
    var track_render_rid: RID = TrackRenderingServer.create_track(track_rid)

    assert_eq(TrackRenderingServer._get_track_render_rid_by_track_rid(track_rid), track_render_rid)

    TrackRenderingServer.free_track(track_render_rid)

    assert_false(TrackRenderingServer._get_track_render_rid_by_track_rid(track_rid).is_valid())


func test_switch_blade_layout_uses_curve1_left_and_curve2_right_for_right_switch() -> void:
    var layout: Dictionary = TrackRenderingServer._get_switch_blade_layout(true, -0.05, 0.0, TrackServer.switch_max_offset)

    assert_true(layout["primary_blade_mirrored"], "right switch should place curve1 blade on the left rail")
    assert_false(layout["secondary_blade_mirrored"], "right switch should place curve2 blade on the right rail")
    assert_almost_eq(layout["primary_blade_offset"], 0.0, 0.001)
    assert_almost_eq(layout["secondary_blade_offset"], -0.15, 0.001)


func test_switch_blade_layout_uses_curve1_right_and_curve2_left_for_left_switch() -> void:
    var layout: Dictionary = TrackRenderingServer._get_switch_blade_layout(false, -0.05, 0.0, TrackServer.switch_max_offset)

    assert_false(layout["primary_blade_mirrored"], "left switch should place curve1 blade on the right rail")
    assert_true(layout["secondary_blade_mirrored"], "left switch should place curve2 blade on the left rail")
    assert_almost_eq(layout["primary_blade_offset"], 0.0, 0.001)
    assert_almost_eq(layout["secondary_blade_offset"], 0.15, 0.001)


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
