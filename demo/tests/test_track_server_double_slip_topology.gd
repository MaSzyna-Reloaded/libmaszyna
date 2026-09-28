extends MaszynaGutTest

## Regression fixture for a double slip (rozjazd krzyzowy): four "switch" tracks plus the four
## short "normal" connectors between them. The coordinates are the real ones from a scenery
## (tarniowo, switch group cze_z1205a..d), moved to a local origin - what matters is that the
## two branch ends of one switch sit only ~0.2 m apart, which used to be inside TrackServer's
## endpoint tolerance and collapsed the whole group into a single topology node.

var created_tracks: Array[RID] = []

## The two branch ends of every switch in the group, in fixture coordinates.
const SWITCH_ENDS: Dictionary[String, Array] = {
    "a": [Vector3(-0.653, 0.0, -0.606), Vector3(-0.719, 0.0, -0.799)],
    "b": [Vector3(-0.598, 0.0, -0.424), Vector3(-0.545, 0.0, -0.227)],
    "c": [Vector3(-3.891, 0.0, 0.573), Vector3(-3.826, 0.0, 0.766)],
    "d": [Vector3(-3.947, 0.0, 0.390), Vector3(-3.999, 0.0, 0.194)],
}
const SWITCH_POINTS: Dictionary[String, Vector3] = {
    "a": Vector3(7.616, 0.0, -3.617),
    "b": Vector3(7.953, 0.0, -2.503),
    "c": Vector3(-12.160, 0.003, 3.584),
    "d": Vector3(-12.497, 0.002, 2.470),
}
## Each connector joins one branch end of one switch to one branch end of another.
const CONNECTORS: Array[Array] = [
    ["a", 0, "c", 0],
    ["a", 1, "d", 1],
    ["d", 0, "b", 0],
    ["b", 1, "c", 1],
]

var switches: Dictionary[String, RID] = {}
var connectors: Array[RID] = []


func before_each() -> void:
    switches.clear()
    connectors.clear()
    for name: String in SWITCH_POINTS:
        switches[name] = _register_track(
            _curve(SWITCH_POINTS[name], SWITCH_ENDS[name][0]),
            _curve(SWITCH_POINTS[name], SWITCH_ENDS[name][1]),
            TrackServer.TRACK_SWITCH
        )
    for connector: Array in CONNECTORS:
        connectors.append(_register_track(
            _curve(SWITCH_ENDS[connector[0]][connector[1]], SWITCH_ENDS[connector[2]][connector[3]])
        ))
    TrackServer.topology_rebuild()


func after_each() -> void:
    for track_rid: RID in created_tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    created_tracks.clear()
    TrackServer.topology_rebuild()


## Guards the fixture itself: if a future edit spreads these points apart, the test stops
## reproducing the geometry that broke.
func test_fixture_branch_ends_are_closer_than_a_quarter_metre() -> void:
    for name: String in SWITCH_ENDS:
        var gap: float = SWITCH_ENDS[name][0].distance_to(SWITCH_ENDS[name][1])
        assert_lt(gap, 0.25, "branch ends of switch %s" % name)
        assert_gt(gap, 0.05, "branch ends of switch %s" % name)


func test_each_branch_end_has_exactly_one_connection() -> void:
    var branch_end_endpoints: PackedInt32Array = [
        TrackServer.CURVE1_P2,
        TrackServer.CURVE2_P2,
    ]
    for name: String in switches:
        for endpoint_index: TrackServer.EndpointIndex in branch_end_endpoints:
            var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
                switches[name],
                endpoint_index
            )
            assert_eq(connections.size(), 1, "switch %s endpoint %d" % [name, endpoint_index])


func test_branch_ends_connect_to_the_expected_connector() -> void:
    var branch_end_endpoints: PackedInt32Array = [
        TrackServer.CURVE1_P2,
        TrackServer.CURVE2_P2,
    ]
    for connector_index: int in range(CONNECTORS.size()):
        var connector: Array = CONNECTORS[connector_index]
        var connector_rid: RID = connectors[connector_index]
        for side: int in [0, 2]:
            var connections: Array[TrackEndpointRef] = TrackServer.track_get_endpoint_connections(
                switches[connector[side]],
                branch_end_endpoints[connector[side + 1]]
            )
            assert_eq(connections.size(), 1)
            assert_eq(connections[0].track_rid, connector_rid)


func test_only_the_switch_point_is_a_common_endpoint() -> void:
    for name: String in switches:
        var common_endpoints: PackedInt32Array = TrackServer.switch_get_common_endpoints(switches[name])
        assert_eq(common_endpoints.size(), 2, "switch %s" % name)
        assert_true(common_endpoints.has(TrackServer.CURVE1_P1), "switch %s" % name)
        assert_true(common_endpoints.has(TrackServer.CURVE2_P1), "switch %s" % name)


func test_both_branches_report_their_connector_as_a_neighbor() -> void:
    for connector_index: int in range(CONNECTORS.size()):
        var connector: Array = CONNECTORS[connector_index]
        var connector_rid: RID = connectors[connector_index]
        for side: int in [0, 2]:
            var branch: TrackServer.SwitchTrack = connector[side + 1] as TrackServer.SwitchTrack
            var neighbors: TrackBranchNeighbors = TrackServer.switch_track_get_neighbors(
                switches[connector[side]],
                branch
            )
            assert_eq(neighbors.next_track_rid, connector_rid, "switch %s branch %d" % [connector[side], branch])


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
