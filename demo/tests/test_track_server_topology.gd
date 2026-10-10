extends MaszynaGutTest

var created_tracks: Array[RID] = []
var topology_change_count: int = 0


func after_each() -> void:
    for track_rid: RID in created_tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    created_tracks.clear()
    TrackServer.topology_rebuild()


func test_connects_normal_track_endpoints() -> void:
    var first_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var second_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()

    var first_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_rid,
        TrackServer.CURVE1_P2
    )
    var second_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        second_rid,
        TrackServer.CURVE1_P1
    )

    assert_eq(first_connections.size(), 1)
    assert_eq(first_connections[0].track_rid, second_rid)
    assert_eq(first_connections[0].endpoint_index, TrackServer.CURVE1_P1)
    assert_eq(second_connections.size(), 1)
    assert_eq(second_connections[0].track_rid, first_rid)
    assert_eq(second_connections[0].endpoint_index, TrackServer.CURVE1_P2)


func test_switch_branch_neighbors_with_common_start() -> void:
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    var previous_rid: RID = _register_track(_curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)))
    var main_next_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    var diverging_next_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 10.0), Vector3(20.0, 0.0, 20.0)))
    TrackServer.topology_rebuild()

    var main_neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(switch_rid, TrackServer.TRACK_COMMON)
    var diverging_neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(switch_rid, TrackServer.TRACK_DIVERGING)

    assert_eq(TrackServer.track_get_common_endpoint_index(switch_rid), TrackServer.POINT_P1)
    assert_eq(main_neighbors.previous_track_rid, previous_rid)
    assert_eq(main_neighbors.previous_endpoint_index, TrackServer.CURVE1_P2)
    assert_eq(main_neighbors.next_track_rid, main_next_rid)
    assert_eq(main_neighbors.next_endpoint_index, TrackServer.CURVE1_P1)
    assert_eq(diverging_neighbors.previous_track_rid, previous_rid)
    assert_eq(diverging_neighbors.previous_endpoint_index, TrackServer.CURVE1_P2)
    assert_eq(diverging_neighbors.next_track_rid, diverging_next_rid)
    assert_eq(diverging_neighbors.next_endpoint_index, TrackServer.CURVE1_P1)


func test_switch_branch_neighbors_with_common_end() -> void:
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(20.0, 0.0, 10.0), Vector3(10.0, 0.0, 0.0)),
        TrackServer.TRACK_SWITCH
    )
    var main_previous_rid: RID = _register_track(_curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)))
    var diverging_previous_rid: RID = _register_track(_curve(Vector3(30.0, 0.0, 20.0), Vector3(20.0, 0.0, 10.0)))
    var next_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(30.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()

    var main_neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(switch_rid, TrackServer.TRACK_COMMON)
    var diverging_neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(switch_rid, TrackServer.TRACK_DIVERGING)

    assert_eq(TrackServer.track_get_common_endpoint_index(switch_rid), TrackServer.POINT_P2)
    assert_eq(main_neighbors.previous_track_rid, main_previous_rid)
    assert_eq(main_neighbors.previous_endpoint_index, TrackServer.CURVE1_P2)
    assert_eq(main_neighbors.next_track_rid, next_rid)
    assert_eq(main_neighbors.next_endpoint_index, TrackServer.CURVE1_P1)
    assert_eq(diverging_neighbors.previous_track_rid, diverging_previous_rid)
    assert_eq(diverging_neighbors.previous_endpoint_index, TrackServer.CURVE1_P2)
    assert_eq(diverging_neighbors.next_track_rid, next_rid)
    assert_eq(diverging_neighbors.next_endpoint_index, TrackServer.CURVE1_P1)


func test_switch_metadata_getters_return_precomputed_values() -> void:
    var common_curve: TrackCurve = _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0))
    var diverging_curve: TrackCurve = _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0))
    var switch_rid: RID = _register_track(
        common_curve,
        diverging_curve,
        TrackServer.TRACK_SWITCH
    )

    var common_endpoints: PackedInt32Array = TrackServer.switch_get_common_endpoints(switch_rid)

    assert_eq(common_endpoints, PackedInt32Array([TrackServer.CURVE1_P1, TrackServer.CURVE2_P1]))
    assert_eq(
        TrackServer.switch_get_branch_start_endpoint(switch_rid, TrackServer.TRACK_COMMON),
        TrackServer.CURVE1_P1
    )
    assert_eq(
        TrackServer.switch_get_branch_end_endpoint(switch_rid, TrackServer.TRACK_DIVERGING),
        TrackServer.CURVE2_P2
    )
    assert_eq(
        TrackServer.switch_get_endpoint_branch(switch_rid, TrackServer.CURVE2_P2),
        TrackServer.TRACK_DIVERGING
    )
    assert_eq(TrackServer.track_get_curve(switch_rid, TrackServer.TRACK_COMMON), common_curve)
    assert_eq(TrackServer.track_get_curve(switch_rid, TrackServer.TRACK_DIVERGING), diverging_curve)
    assert_eq(
        TrackServer.track_get_domain_curve(
            switch_rid,
            TrackServer.TRACK_DIVERGING
        ).get_baked_length(),
        TrackServer.track_get_length(switch_rid, TrackServer.TRACK_DIVERGING)
    )
    assert_true(TrackServer.switch_get_blade_boundary_offset(switch_rid, TrackServer.TRACK_COMMON) > 0.0)


func test_switch_common_node_does_not_report_own_branch_as_connection() -> void:
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.topology_rebuild()

    assert_eq(
        TrackServer.track_get_endpoint_connections(switch_rid, TrackServer.CURVE1_P1),
        []
    )


func test_track_get_endpoint_connections_returns_empty_for_unconnected_endpoint() -> void:
    var track_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()

    assert_eq(
        TrackServer.track_get_endpoint_connections(track_rid, TrackServer.CURVE1_P2),
        []
    )


func test_track_get_endpoint_connections_returns_multiple_connections_for_shared_node() -> void:
    var source_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var first_neighbor_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    var second_neighbor_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 10.0)))
    TrackServer.topology_rebuild()

    var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        source_rid,
        TrackServer.CURVE1_P2
    )

    assert_eq(connections.size(), 2)
    assert_eq(connections[0].track_rid, first_neighbor_rid)
    assert_eq(connections[0].endpoint_index, TrackServer.CURVE1_P1)
    assert_eq(connections[1].track_rid, second_neighbor_rid)
    assert_eq(connections[1].endpoint_index, TrackServer.CURVE1_P1)


func test_track_get_endpoint_connections_returns_copies() -> void:
    var first_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var second_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()

    var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_rid,
        TrackServer.CURVE1_P2
    )
    connections[0].track_rid = RID()
    connections[0].endpoint_index = TrackServer.CURVE2_P2

    var fresh_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_rid,
        TrackServer.CURVE1_P2
    )

    assert_eq(fresh_connections.size(), 1)
    assert_eq(fresh_connections[0].track_rid, second_rid)
    assert_eq(fresh_connections[0].endpoint_index, TrackServer.CURVE1_P1)


func test_rebuild_topology_preserves_connections() -> void:
    var first_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var second_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))

    TrackServer.topology_rebuild()

    var first_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_rid,
        TrackServer.CURVE1_P2
    )
    var second_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        second_rid,
        TrackServer.CURVE1_P1
    )

    assert_eq(first_connections.size(), 1)
    assert_eq(first_connections[0].track_rid, second_rid)
    assert_eq(second_connections.size(), 1)
    assert_eq(second_connections[0].track_rid, first_rid)


func test_register_track_marks_topology_changed() -> void:
    TrackServer.topology_rebuild()
    topology_change_count = 0
    TrackServer.topology_changed.connect(_on_topology_changed)

    var track_rid: RID = TrackServer.track_create()
    created_tracks.append(track_rid)

    TrackServer.topology_changed.disconnect(_on_topology_changed)
    assert_true(TrackServer.is_topology_changed)
    assert_eq(topology_change_count, 1)


func test_track_free_marks_topology_changed() -> void:
    var track_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    topology_change_count = 0
    TrackServer.topology_changed.connect(_on_topology_changed)

    TrackServer.track_free(track_rid)
    created_tracks.erase(track_rid)

    TrackServer.topology_changed.disconnect(_on_topology_changed)
    assert_true(TrackServer.is_topology_changed)
    assert_eq(topology_change_count, 1)


func test_connected_endpoint_change_marks_topology_changed() -> void:
    var first_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var second_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    topology_change_count = 0
    TrackServer.topology_changed.connect(_on_topology_changed)

    TrackServer.track_update_curves(
        first_rid,
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(11.0, 0.0, 0.0)),
        null
    )
    TrackServer.track_update(first_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    TrackServer.topology_changed.disconnect(_on_topology_changed)
    var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_rid,
        TrackServer.CURVE1_P2
    )
    assert_eq(connections.size(), 1)
    assert_eq(connections[0].track_rid, second_rid)
    assert_true(TrackServer.is_topology_changed)
    assert_eq(topology_change_count, 1)


func test_unconnected_endpoint_change_does_not_mark_topology_changed() -> void:
    var first_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var second_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    topology_change_count = 0
    TrackServer.topology_changed.connect(_on_topology_changed)

    TrackServer.track_update_curves(
        first_rid,
        _curve(Vector3(-1.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null
    )
    TrackServer.track_update(first_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    TrackServer.topology_changed.disconnect(_on_topology_changed)
    var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_rid,
        TrackServer.CURVE1_P2
    )
    assert_eq(connections.size(), 1)
    assert_eq(connections[0].track_rid, second_rid)
    assert_false(TrackServer.is_topology_changed)
    assert_eq(topology_change_count, 0)


func test_disconnected_graphs_stay_independent() -> void:
    var first_a_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var second_a_rid: RID = _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    var first_b_rid: RID = _register_track(_curve(Vector3(100.0, 0.0, 0.0), Vector3(110.0, 0.0, 0.0)))
    var second_b_rid: RID = _register_track(_curve(Vector3(110.0, 0.0, 0.0), Vector3(120.0, 0.0, 0.0)))

    TrackServer.topology_rebuild()

    var first_a_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_a_rid,
        TrackServer.CURVE1_P2
    )
    var first_b_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_b_rid,
        TrackServer.CURVE1_P2
    )

    assert_eq(first_a_connections.size(), 1)
    assert_eq(first_a_connections[0].track_rid, second_a_rid)
    assert_eq(first_b_connections.size(), 1)
    assert_eq(first_b_connections[0].track_rid, second_b_rid)


func test_incompatible_track_types_do_not_share_topology() -> void:
    var track_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    var road_rid: RID = _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_ROAD
    )

    TrackServer.topology_rebuild()

    assert_eq(TrackServer.track_get_endpoint_connections(track_rid, TrackServer.CURVE1_P2), [])
    assert_eq(TrackServer.track_get_endpoint_connections(road_rid, TrackServer.CURVE1_P1), [])


func test_road_connects_to_cross() -> void:
    var road_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_ROAD
    )
    var cross_rid: RID = _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        _curve(Vector3(15.0, 0.0, -5.0), Vector3(15.0, 0.0, 5.0)),
        TrackServer.TRACK_CROSS
    )

    TrackServer.topology_rebuild()

    var road_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        road_rid,
        TrackServer.CURVE1_P2
    )
    var cross_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        cross_rid,
        TrackServer.CURVE1_P1
    )

    assert_eq(road_connections.size(), 1)
    assert_eq(road_connections[0].track_rid, cross_rid)
    assert_eq(cross_connections.size(), 1)
    assert_eq(cross_connections[0].track_rid, road_rid)


func test_river_connects_to_tributary() -> void:
    var river_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_RIVER
    )
    var tributary_rid: RID = _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_TRIBUTARY
    )

    TrackServer.topology_rebuild()

    var river_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        river_rid,
        TrackServer.CURVE1_P2
    )
    var tributary_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        tributary_rid,
        TrackServer.CURVE1_P1
    )

    assert_eq(river_connections.size(), 1)
    assert_eq(river_connections[0].track_rid, tributary_rid)
    assert_eq(tributary_connections.size(), 1)
    assert_eq(tributary_connections[0].track_rid, river_rid)


func test_switches_report_topology_connections_to_each_other() -> void:
    var first_switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    var second_switch_rid: RID = _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )

    TrackServer.topology_rebuild()

    var first_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        first_switch_rid,
        TrackServer.CURVE1_P2
    )
    var second_connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
        second_switch_rid,
        TrackServer.CURVE1_P1
    )

    assert_eq(first_connections.size(), 2)
    assert_eq(first_connections[0].track_rid, second_switch_rid)
    assert_eq(first_connections[1].track_rid, second_switch_rid)
    assert_eq(second_connections.size(), 1)
    assert_eq(second_connections[0].track_rid, first_switch_rid)


func test_get_tracks_in_aabb_returns_intersecting_tracks_once() -> void:
    var track_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(600.0, 0.0, 0.0)))

    var result: Array[RID] = TrackServer.tracks_find_in_aabb(Rect2(Vector2(-1.0, -1.0), Vector2(602.0, 2.0)))

    assert_eq(result, [track_rid])


func test_get_tracks_in_aabb_updates_track_position() -> void:
    var track_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))

    TrackServer.track_update_curves(
        track_rid,
        _curve(Vector3(1000.0, 0.0, 0.0), Vector3(1010.0, 0.0, 0.0)),
        null
    )
    TrackServer.track_update(track_rid, TrackServer.TRACK_NORMAL, "", 1.435)

    assert_eq(TrackServer.tracks_find_in_aabb(Rect2(Vector2(-10.0, -10.0), Vector2(30.0, 20.0))), [])
    assert_eq(TrackServer.tracks_find_in_aabb(Rect2(Vector2(990.0, -10.0), Vector2(30.0, 20.0))), [track_rid])


func test_get_tracks_in_aabb_removes_unregistered_track() -> void:
    var track_rid: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))

    TrackServer.track_free(track_rid)
    created_tracks.erase(track_rid)

    assert_eq(TrackServer.tracks_find_in_aabb(Rect2(Vector2(-10.0, -10.0), Vector2(30.0, 20.0))), [])


func test_connected_track_topology_builds_one_graph() -> void:
    _register_connected_track_topology()

    TrackServer.topology_rebuild()

    assert_eq(TrackServer.topology_get_summary().graphs.size(), 1)


func test_connected_track_topology_graph_has_all_tracks() -> void:
    _register_connected_track_topology()

    TrackServer.topology_rebuild()

    var summary: Dictionary = TrackServer.topology_get_summary()
    assert_eq(summary.graphs[0].num_tracks, 6)


func test_connected_track_topology_graph_total_length() -> void:
    _register_connected_track_topology()

    TrackServer.topology_rebuild()

    var summary: Dictionary = TrackServer.topology_get_summary()
    assert_almost_eq(summary.graphs[0].total_length, 1082.0, 0.5)


func test_connected_track_topology_has_no_orphaned_tracks() -> void:
    _register_connected_track_topology()

    TrackServer.topology_rebuild()

    assert_eq(TrackServer.topology_get_summary().orphaned_tracks_count, 0)


func _register_track(
    curve1: TrackCurve,
    curve2: TrackCurve = null,
    type: int = TrackServer.TRACK_NORMAL,
) -> RID:
    var track_rid: RID = TrackServer.track_create()
    created_tracks.append(track_rid)
    TrackServer.track_update_curves(track_rid, curve1, curve2)
    TrackServer.track_update(track_rid, type, "", 1.435)
    return track_rid


func _curve(p1: Vector3, p2: Vector3) -> TrackCurve:
    var curve: TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    return curve


func _demo_curve(p1: Vector3, p2: Vector3, c1: Vector3 = Vector3.ZERO, c2: Vector3 = Vector3.ZERO) -> TrackCurve:
    var curve: TrackCurve = _curve(p1, p2)
    curve.c1 = c1
    curve.c2 = c2
    return curve


func _register_connected_track_topology() -> void:
    _register_track(_demo_curve(Vector3(-25.8, 0.0, -136.0), Vector3(-25.8, 0.0, 136.0)))
    _register_track(_demo_curve(Vector3(-19.19, 0.0, -100.0), Vector3(-19.19, 0.0, 70.0)))
    _register_track(_demo_curve(Vector3(-25.8, 0.0, 236.0), Vector3(-25.8, 0.0, 536.0)))
    _register_track(
        _demo_curve(Vector3(-25.8, 0.0, 236.0), Vector3(-25.8, 0.0, 136.0)),
        _demo_curve(
            Vector3(-25.8, 0.0, 236.0),
            Vector3(-19.19, 0.0, 130.0),
            Vector3(-0.305, 0.0, -21.41),
            Vector3(0.305, 0.0, 20.0)
        ),
        TrackServer.TRACK_SWITCH
    )
    _register_track(
        _demo_curve(Vector3(-19.19, 0.0, 130.0), Vector3(-19.19, 0.0, 70.0)),
        _demo_curve(
            Vector3(-19.19, 0.0, 130.0),
            Vector3(-12.53, 0.0, 80.0),
            Vector3(-0.513, 0.0, -8.065),
            Vector3(-0.823, 0.0, 6.27)
        ),
        TrackServer.TRACK_SWITCH
    )
    _register_track(_demo_curve(Vector3(-12.53, 0.0, -100.0), Vector3(-12.53, 0.0, 80.0), Vector3.ZERO, Vector3(1.61, 0.0, -3.35)))


func _on_topology_changed() -> void:
    topology_change_count += 1
