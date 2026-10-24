extends MaszynaGutTest


## trainset_move(): the ends and the coupling of a coupler alone (coupling::coupler, MOVER.h:161)
const MOVE_DISTANCE: float = 10.0
## A vehicle with buffers and couplers that collide
const FIXTURE_PATH: String = "res://tests/fixtures/test_vehicle.fiz"
## A vehicle rolling at this speed [m/s] onto a parked one, this far apart [m] - beyond the parked
## one's scan range - meets it within COLLISION_TIME [s] of simulation, advanced in frames of
## COLLISION_FRAME [s]
const COLLISION_SPEED: float = 5.0
const COLLISION_GAP: float = 80.0
const COLLISION_TIME: float = 30.0
const COLLISION_FRAME: float = 0.1
## The fixture vehicle's axles this far apart [m] (its pivot spacing; fAxleDist, DynObj.cpp:2499)
const AXLE_DISTANCE: float = 10.0
## A track this long [m], crossed by moves this long - longer than it, shorter than the axle
## distance - so many of them taking both axles from the first track onto the last
const SHORT_TRACK: float = 1.0
const SHORT_TRACK_MOVE: float = 5.0
const SHORT_TRACK_MOVES: int = 8
## How long [s] a vehicle rolls in one process_movement() call to change its primary axle
const ROLL_TIME: float = 0.1

var created_tracks: Array[RID] = []
var created_vehicle_rids: Array[RID] = []
var created_controllers: Array[VehicleController] = []


func after_each() -> void:
    for vehicle_rid: RID in created_vehicle_rids:
        VehicleServer.vehicle_free(vehicle_rid)
    created_vehicle_rids.clear()

    created_controllers.clear()

    for track_rid: RID in created_tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    created_tracks.clear()
    TrackServer.topology_rebuild()


func test_vehicle_set_track_initializes_normal_track_state() -> void:
    var track_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        track_rid,
        15.0,
        TrackServer.DIRECTION_NORMAL
    )

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(10.0, TrackServer.rail_height, 0.0)
    )


func test_vehicle_set_track_follows_connected_tracks_for_initial_offsets() -> void:
    for index:int in range(3):
        _register_track(_curve(Vector3(index * 10, 0, 0), Vector3((index + 1) * 10, 0, 0)))
    TrackServer.topology_rebuild()
    var vehicle_rid:RID = _create_vehicle()
    for direction:TrackServer.Direction in [TrackServer.DIRECTION_NORMAL, TrackServer.DIRECTION_REVERSED]:
        RailVehicleServer.vehicle_set_track(vehicle_rid, created_tracks[2], -15.0, direction)
        _assert_vector_eq(RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
            Vector3(5.0, TrackServer.rail_height, 0.0))
        RailVehicleServer.vehicle_set_track(vehicle_rid, created_tracks[0], 25.0, direction)
        _assert_vector_eq(RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
            Vector3(25.0, TrackServer.rail_height, 0.0))


func test_vehicle_set_track_initializes_switch_state_from_active_branch() -> void:
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_DIVERGING)
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        switch_rid,
        3.0,
        TrackServer.DIRECTION_NORMAL
    )

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(2.1213, TrackServer.rail_height, 2.1213),
        "switch initialization should sample the active diverging branch"
    )


func test_vehicle_get_transform_applies_common_and_diverging_orientation() -> void:
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    var vehicle_rid: RID = _create_vehicle()

    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_COMMON)
    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        switch_rid,
        4.0,
        TrackServer.DIRECTION_NORMAL
    )
    var common_forward: Vector3 = -RailVehicleServer.vehicle_get_transform(vehicle_rid).basis.z.normalized()

    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_DIVERGING)
    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        switch_rid,
        4.0,
        TrackServer.DIRECTION_NORMAL
    )
    var diverging_forward: Vector3 = -RailVehicleServer.vehicle_get_transform(vehicle_rid).basis.z.normalized()

    _assert_vector_eq(common_forward, Vector3.RIGHT, "common branch should face along curve1")
    assert_true(diverging_forward.distance_to(Vector3.RIGHT) > 0.1, "diverging branch should not reuse curve1 orientation")


func test_vehicle_move_crosses_connected_tracks() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    var second_rid: RID = _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        created_tracks[0],
        8.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_move(vehicle_rid, 5.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(13.0, TrackServer.rail_height, 0.0)
    )


func test_vehicle_move_stops_at_ambiguous_shared_node() -> void:
    var source_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        source_rid,
        8.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_move(vehicle_rid, 5.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(10.0, TrackServer.rail_height, 0.0)
    )


func test_process_movement_advances_bound_controller_vehicle() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var controller: VehicleController = _create_controller(5.0)
    # the vehicle the controller already belongs to - a second handle would step it twice
    var vehicle_rid: RID = controller.get_rid()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        created_tracks[0],
        8.0,
        TrackServer.DIRECTION_REVERSED
    )

    RailVehicleServer.vehicle_process_movement(vehicle_rid, 1.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(3.0, TrackServer.rail_height, 0.0)
    )


func test_process_movement_moves_vehicle_toward_its_own_front() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var controller: VehicleController = _create_controller(5.0)
    # the vehicle the controller already belongs to - a second handle would step it twice
    var vehicle_rid: RID = controller.get_rid()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        created_tracks[0],
        4.0,
        TrackServer.DIRECTION_NORMAL
    )
    var forward: Vector3 = -RailVehicleServer.vehicle_get_transform(vehicle_rid).basis.z.normalized()

    RailVehicleServer.vehicle_process_movement(vehicle_rid, 1.0)

    var moved_by: Vector3 = (
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin
        - Vector3(4.0, TrackServer.rail_height, 0.0)
    )
    assert_true(
        moved_by.normalized().distance_to(forward) < 0.01,
        "positive mover velocity should move the vehicle toward its own front, got %s vs front %s" % [moved_by, forward]
    )


func test_trainset_move_moves_every_vehicle_the_same_way() -> void:
    _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(100.0, 0.0, 0.0)), null, TrackServer.TRACK_NORMAL)
    TrackServer.topology_rebuild()
    var first: VehicleController = _create_controller()
    var second: VehicleController = _create_controller()
    # the second stands the other way round: front to front
    RailVehicleServer.vehicle_set_track(first.get_rid(), created_tracks[0], 40.0, TrackServer.DIRECTION_NORMAL)
    RailVehicleServer.vehicle_set_track(second.get_rid(), created_tracks[0], 60.0, TrackServer.DIRECTION_REVERSED)
    first.couple(second, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLER_END_FRONT,
            RailVehicleController.COUPLING_FLAG_COUPLER)
    var forward: Vector3 = -RailVehicleServer.vehicle_get_transform(first.get_rid()).basis.z.normalized()
    var first_start: Vector3 = RailVehicleServer.vehicle_get_transform(first.get_rid()).origin
    var second_start: Vector3 = RailVehicleServer.vehicle_get_transform(second.get_rid()).origin

    RailVehicleServer.trainset_move(first.get_rid(), MOVE_DISTANCE)

    _assert_vector_eq(RailVehicleServer.vehicle_get_transform(first.get_rid()).origin,
            first_start + forward * MOVE_DISTANCE, "the vehicle moved")
    _assert_vector_eq(RailVehicleServer.vehicle_get_transform(second.get_rid()).origin,
            second_start + forward * MOVE_DISTANCE, "the reversed vehicle moved with it")


func test_process_movement_without_bound_controller_is_noop() -> void:
    var track_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        track_rid,
        3.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_process_movement(vehicle_rid, 1.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(3.0, TrackServer.rail_height, 0.0)
    )


func test_process_movement_with_invalid_controller_reference_is_noop() -> void:
    var track_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var controller: VehicleController = _create_controller(5.0)
    # the vehicle the controller already belongs to - a second handle would step it twice
    var vehicle_rid: RID = controller.get_rid()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        track_rid,
        3.0,
        TrackServer.DIRECTION_REVERSED
    )

    # what makes the reference invalid is the server losing it, not a local variable being
    # dropped - the vehicle then has nothing to ask for a velocity and stays where it is
    VehicleServer.vehicle_bind_controller(vehicle_rid, RID())
    await wait_idle_frames(1)

    RailVehicleServer.vehicle_process_movement(vehicle_rid, 1.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(3.0, TrackServer.rail_height, 0.0)
    )


func test_removed_track_makes_transform_and_movement_noop() -> void:
    var track_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.topology_rebuild()
    var controller: VehicleController = _create_controller(5.0)
    # the vehicle the controller already belongs to - a second handle would step it twice
    var vehicle_rid: RID = controller.get_rid()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        track_rid,
        3.0,
        TrackServer.DIRECTION_REVERSED
    )

    TrackServer.track_free(track_rid)

    RailVehicleServer.vehicle_move(vehicle_rid, 5.0)
    RailVehicleServer.vehicle_process_movement(vehicle_rid, 1.0)

    assert_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid),
        Transform3D.IDENTITY,
        "removed track should make transform queries safe and inert"
    )


func test_vehicle_reverse_on_switch_keeps_occupied_branch_after_switch_change() -> void:
    _register_track(
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        created_tracks[0],
        8.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_move(vehicle_rid, 5.0)
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_COMMON)

    RailVehicleServer.vehicle_move(vehicle_rid, -1.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        Vector3(1.4142, TrackServer.rail_height, 1.4142),
        "reverse movement should keep sampling the occupied diverging branch"
    )


func test_vehicle_move_forces_switch_diverging_when_entering_from_diverging_branch() -> void:
    var start_rid: RID = _register_track(
        _curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var distance_on_switch: float = 2.8579
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        start_rid,
        12.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_move(vehicle_rid, 5.0)

    assert_eq(TrackServer.switch_get_active_track(TrackServer.track_get_switch(switch_rid)), TrackServer.TRACK_DIVERGING)
    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        _track_position(
            switch_rid,
            TrackServer.track_get_length(switch_rid, TrackServer.TRACK_DIVERGING) - distance_on_switch
        ),
        "diverging branch entry should force diverging switch route"
    )


func test_vehicle_move_large_offset_forces_switch_when_entering_from_diverging_branch() -> void:
    var start_rid: RID = _register_track(
        _curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    var next_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(-10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        start_rid,
        12.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_move(vehicle_rid, 25.0)

    var expected_next_offset: float = 25.0 \
        - (TrackServer.track_get_length(start_rid) - 12.0) \
        - TrackServer.track_get_length(switch_rid, TrackServer.TRACK_DIVERGING)
    assert_eq(TrackServer.switch_get_active_track(TrackServer.track_get_switch(switch_rid)), TrackServer.TRACK_DIVERGING)
    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        _track_position(next_rid, expected_next_offset),
        "large diverging-side entry should force diverging without incremental simulation"
    )


func test_vehicle_move_from_common_point_uses_active_switch_branch() -> void:
    var start_rid: RID = _register_track(
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_rid), TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var vehicle_rid: RID = _create_vehicle()

    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        start_rid,
        8.0,
        TrackServer.DIRECTION_REVERSED
    )
    RailVehicleServer.vehicle_move(vehicle_rid, 15.0)

    _assert_vector_eq(
        RailVehicleServer.vehicle_get_transform(vehicle_rid).origin,
        _track_position(switch_rid, 13.0),
        "common-point entry should follow the active diverging branch"
    )


# A parked vehicle does not scan for neighbours again while nothing near it changes; one rolling
# up from beyond its scan range has to reach it all the same - it wakes its physics and pushes it
# (Mover.cpp:4490-4503)
func test_a_parked_vehicle_is_pushed_by_one_rolling_onto_it() -> void:
    _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(200.0, 0.0, 0.0)), null, TrackServer.TRACK_NORMAL)
    TrackServer.topology_rebuild()
    var rolling: VehicleController = _create_fiz_vehicle(COLLISION_SPEED)
    var parked: VehicleController = _create_fiz_vehicle(0.0)
    RailVehicleServer.vehicle_set_track(rolling.get_rid(), created_tracks[0], 100.0, TrackServer.DIRECTION_NORMAL)
    # parked ahead of the rolling one's front, whichever way along the track that is
    var forward: Vector3 = -RailVehicleServer.vehicle_get_transform(rolling.get_rid()).basis.z.normalized()
    RailVehicleServer.vehicle_set_track(parked.get_rid(), created_tracks[0],
            100.0 + signf(forward.x) * COLLISION_GAP, TrackServer.DIRECTION_NORMAL)
    var parked_start: Vector3 = RailVehicleServer.vehicle_get_transform(parked.get_rid()).origin

    var simulated: float = 0.0
    while simulated < COLLISION_TIME:
        SimulationServer.simulation_advance(COLLISION_FRAME)
        simulated += COLLISION_FRAME

    var pushed_by: Vector3 = RailVehicleServer.vehicle_get_transform(parked.get_rid()).origin - parked_start
    assert_gt(pushed_by.dot(forward), 0.0, "the parked vehicle is pushed the way the other one rolled")


func test_an_axle_counts_on_every_track_it_crosses() -> void:
    var first: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(50.0, 0.0, 0.0)))
    var short: RID = _register_track(_curve(Vector3(50.0, 0.0, 0.0), Vector3(50.0 + SHORT_TRACK, 0.0, 0.0)))
    _register_track(_curve(Vector3(50.0 + SHORT_TRACK, 0.0, 0.0), Vector3(100.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    var section: RID = TrackServer.isolated_create()
    TrackServer.isolated_add_track(section, short)
    var counts: Dictionary[StringName, int] = {&"entered": 0, &"left": 0, &"occupied": 0, &"freed": 0}
    var count: Callable = func(_isolated: RID, _vehicle: RID, what: StringName) -> void: counts[what] += 1
    var listeners: Dictionary[Signal, Callable] = {
        TrackServer.isolated_axle_entered: count.bind(&"entered"),
        TrackServer.isolated_axle_left: count.bind(&"left"),
        TrackServer.isolated_occupied: count.bind(&"occupied"),
        TrackServer.isolated_freed: count.bind(&"freed"),
    }
    for listened: Signal in listeners:
        listened.connect(listeners[listened])
    var vehicle: RID = _create_spaced_fiz_vehicle(0.0).get_rid()
    RailVehicleServer.vehicle_set_track(vehicle, first, 20.0, TrackServer.DIRECTION_REVERSED)

    # each axle crosses the short track within one move, onto the last track
    for move: int in SHORT_TRACK_MOVES:
        RailVehicleServer.vehicle_move(vehicle, SHORT_TRACK_MOVE)

    for listened: Signal in listeners:
        listened.disconnect(listeners[listened])
    # a section shorter than the axle distance is free between the axles, as the original's is
    assert_eq(counts, {&"entered": 2, &"left": 2, &"occupied": 2, &"freed": 2} as Dictionary[StringName, int],
            "two axles, each on and off the short track")
    assert_false(TrackServer.isolated_is_occupied(section))
    TrackServer.isolated_free(section)


func test_an_axle_occupies_a_section_before_its_vehicle_is_on_the_track() -> void:
    var first: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(50.0, 0.0, 0.0)))
    var second: RID = _register_track(_curve(Vector3(50.0, 0.0, 0.0), Vector3(100.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    var section: RID = TrackServer.isolated_create()
    TrackServer.isolated_add_track(section, second)
    var vehicle: RID = _create_spaced_fiz_vehicle(0.0).get_rid()

    # the front axle at 42 m on the first track, the rear one at 52 m on the second
    RailVehicleServer.vehicle_set_track(vehicle, first, 47.0, TrackServer.DIRECTION_REVERSED)

    assert_true(TrackServer.isolated_is_occupied(section), "the rear axle stands in the section")
    assert_eq(TrackServer.track_get_vehicles(first), [vehicle] as Array[RID], "listed by its primary, front axle")
    assert_false(TrackServer.track_is_occupied(second), "not listed on the rear axle's track")
    var front: Vector3 = RailVehicleServer.vehicle_get_bogie_transform(vehicle, RailVehicleWheels.BOGIE_FRONT).origin
    var rear: Vector3 = RailVehicleServer.vehicle_get_bogie_transform(vehicle, RailVehicleWheels.BOGIE_REAR).origin
    _assert_vector_eq(front, Vector3(47.0 - 0.5 * AXLE_DISTANCE, TrackServer.rail_height, 0.0), "the front axle")
    _assert_vector_eq(rear, Vector3(47.0 + 0.5 * AXLE_DISTANCE, TrackServer.rail_height, 0.0), "the rear axle")
    _assert_vector_eq(RailVehicleServer.vehicle_get_transform(vehicle).origin, (front + rear) * 0.5,
            "the body midway between the axles")
    RailVehicleServer.vehicle_detach(vehicle)
    assert_false(TrackServer.isolated_is_occupied(section), "a removed vehicle leaves the section")
    TrackServer.isolated_free(section)


func test_the_leading_axle_forces_a_switch_before_the_centre_reaches_it() -> void:
    var start_rid: RID = _register_track(_curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)))
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    var switch: RID = TrackServer.track_get_switch(switch_rid)
    TrackServer.switch_set_active_track(switch, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var vehicle: RID = _create_spaced_fiz_vehicle(0.0).get_rid()
    # the centre 6 m along the start track, the leading (rear) axle 11 m along it
    RailVehicleServer.vehicle_set_track(vehicle, start_rid, 6.0, TrackServer.DIRECTION_REVERSED)

    RailVehicleServer.vehicle_move(vehicle, 4.0)

    assert_lt(10.0, TrackServer.track_get_length(start_rid, TrackServer.TRACK_COMMON), "the centre is still before it")
    assert_eq(TrackServer.switch_get_active_track(switch), TrackServer.TRACK_DIVERGING,
            "the leading axle entering from the diverging branch forced it")


func test_the_primary_axle_is_the_second_in_the_direction_of_travel() -> void:
    var track: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(200.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    var vehicle: RID = _create_spaced_fiz_vehicle(COLLISION_SPEED).get_rid()
    RailVehicleServer.vehicle_set_track(vehicle, track, 100.0, TrackServer.DIRECTION_NORMAL)
    var front_along: float = float(RailVehicleServer.vehicle_get_track_position(vehicle)["along"])

    RailVehicleServer.vehicle_process_movement(vehicle, ROLL_TIME)

    # rolling forwards the rear axle leads the events: the position is now the rear axle's
    var along: float = float(RailVehicleServer.vehicle_get_track_position(vehicle)["along"])
    assert_almost_eq(absf(along - front_along), AXLE_DISTANCE, COLLISION_SPEED * ROLL_TIME + 0.01,
            "the position moved to the other axle")


func test_a_vehicle_stopped_across_a_joint_reports_both_tracks() -> void:
    var first: RID = _register_track(_curve(Vector3(0.0, 0.0, 0.0), Vector3(50.0, 0.0, 0.0)))
    var second: RID = _register_track(_curve(Vector3(50.0, 0.0, 0.0), Vector3(100.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    var vehicle: RID = _create_spaced_fiz_vehicle(0.0).get_rid()
    RailVehicleServer.vehicle_set_track(vehicle, first, 47.0, TrackServer.DIRECTION_REVERSED)
    var reported: Array[RID] = []
    var stopped: Callable = func(track: RID, stopped_vehicle: RID) -> void:
        if stopped_vehicle == vehicle:
            reported.append(track)
    TrackServer.track_vehicle_stopped.connect(stopped)

    SimulationServer.simulation_advance(COLLISION_FRAME)

    TrackServer.track_vehicle_stopped.disconnect(stopped)
    assert_eq(reported, [first, second] as Array[RID], "the primary axle's track, then the other one's")


## The fixture vehicle with its axles AXLE_DISTANCE apart, rolling at `velocity` (m/s)
func _create_spaced_fiz_vehicle(velocity: float) -> VehicleController:
    var controller: VehicleController = _create_fiz_vehicle(velocity)
    var wheels: RailVehicleWheels = VehicleServer.vehicle_component_get(
            controller.get_rid(), VehicleComponentType.COMPONENT_WHEELS) as RailVehicleWheels
    wheels.bogie_pivot_spacing = AXLE_DISTANCE
    return controller


func _create_vehicle() -> RID:
    var vehicle_rid: RID = VehicleServer.vehicle_create()
    RailVehicleServer.vehicle_attach(vehicle_rid)
    created_vehicle_rids.append(vehicle_rid)
    return vehicle_rid


## `velocity` (m/s) is the mover's own velocity, given as the initial velocity in km/h.
func _create_controller(velocity: float = 0.0) -> VehicleController:
    # the initial velocity is read while the Mover is initialised, so it goes in before the build
    var controller: VehicleController = build_vehicle(
            "mock_train_%d" % created_controllers.size(), null, velocity * 3.6)
    created_controllers.append(controller)
    return controller


## A vehicle of the fixture FIZ - buffers and couplers that collide - rolling at `velocity` (m/s)
func _create_fiz_vehicle(velocity: float) -> VehicleController:
    var controller: VehicleController = build_vehicle("fiz_train_%d" % created_controllers.size(),
            FizVehicleBuilder.build_description_at(FIXTURE_PATH), velocity * 3.6)
    created_controllers.append(controller)
    return controller


func _register_track(
    curve1: TrackCurve,
    curve2: TrackCurve = null,
    type: TrackServer.TrackType = TrackServer.TRACK_NORMAL
) -> RID:
    var track_rid: RID = TrackServer.track_create()
    created_tracks.append(track_rid)
    TrackServer.track_update_curves(track_rid, curve1, curve2)
    TrackServer.track_update(track_rid, type, "", 1.435)
    return track_rid


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


func _track_position(track_rid: RID, offset: float) -> Vector3:
    var vehicle_rid: RID = VehicleServer.vehicle_create()
    RailVehicleServer.vehicle_attach(vehicle_rid)
    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        track_rid,
        offset,
        TrackServer.DIRECTION_NORMAL
    )
    var transform: Transform3D = RailVehicleServer.vehicle_get_transform(vehicle_rid)
    VehicleServer.vehicle_free(vehicle_rid)
    return transform.origin


func _assert_vector_eq(actual: Vector3, expected: Vector3, message: String = "") -> void:
    assert_true(actual.distance_to(expected) <= 0.02, "%s expected %s, got %s" % [message, expected, actual])
