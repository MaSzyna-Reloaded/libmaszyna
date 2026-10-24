extends MaszynaGutTest

## MaszynaDriverRoute's table kept between updates, as the original's (TableCheck(),
## TableTraceRoute()): what was traced stays until passed, a switch thrown ahead is traced again
## from, and a train ignores a shunting signal at stop, passed or ahead - read from tracks built
## here.

const Order = MaszynaAIDriver.Order
const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const LINE_VELOCITY:float = 100.0
const RESTRICTED_VELOCITY:float = 40.0
const DIVERGING_VELOCITY:float = 20.0
const SHUNT_SPEED:float = 25.0
## Standing the driver reads 1500 m ahead, moving at this speed [km/h] far less (MOVING_RANGE plus the
## braking distance)
const MOVING_SPEED:float = 50.0
## Where the limit starts on either side of the vehicle's middle [m]: inside the standing reading,
## beyond the moving one
const LIMIT_DISTANCE:float = 1200.0
const LINE_LENGTH:float = 4000.0
## The layout with a switch: the track the vehicle stands on, the switch's length and how far its
## diverging branch ends to the side [m]
const APPROACH_LENGTH:float = 200.0
const SWITCH_LENGTH:float = 30.0
const DIVERGING_OFFSET:float = 10.0
## How far [m] from the vehicle's middle the Tm on either side stand: inside the moving reading
const SHUNT_SIGNAL_DISTANCE:float = 200.0
## How far [m] from the vehicle's middle a signal stands that a player has left far behind: beyond
## MaszynaDriverRoute.HUMAN_PASSED_SIGNAL_DISTANCE, inside the standing reading
const FAR_BEHIND_DISTANCE:float = 400.0
## Markowo Górne's approach: its entry signal at stop this far ahead [m], and the line's speed
## given 88 m after it (markowo_grn_tor2_wjazd_speedinfo); the speeds the train comes at [km/h]
const STOP_SIGNAL_DISTANCE:float = 400.0
const LINE_SPEED_DISTANCE:float = 488.0
const LINE_SPEED:float = 120.0
const APPROACH_SPEED_MIN:float = 5.0
const APPROACH_SPEED_MAX:float = 40.0
const APPROACH_SPEED_STEP:float = 0.01

var _tracks:Array[RID] = []
var _events:Array[RID] = []
## The event system the test's signals are events of, the original's - a track takes only an event of
## one (MaszynaScenarioEventServer.track_add_event())
var _event_system:RID
## Freed before the nodes they are driven by (autofree), as test_rail_vehicle_track_movement.gd does
var _vehicles:Array[RailVehicle3D] = []


func before_each() -> void:
    _event_system = ScenarioEventServer.system_create()
    ScenarioEventServer.system_attach_implementation(_event_system, MaszynaScenarioEventServer.implementation_get())


func after_each() -> void:
    for vehicle:RailVehicle3D in _vehicles:
        remove_child(vehicle)
        vehicle.queue_free()
    _vehicles.clear()
    for event:RID in _events:
        ScenarioEventServer.event_free(event)
    _events.clear()
    ScenarioEventServer.system_free(_event_system)
    for track:RID in _tracks:
        TrackServer.track_free(track)
    _tracks.clear()
    TrackServer.topology_rebuild()


func test_a_limit_traced_standing_stays_when_the_reach_shrinks_moving_off() -> void:
    _track(Vector3(-LINE_LENGTH, 0.0, 0.0), Vector3(-LIMIT_DISTANCE, 0.0, 0.0), null, RESTRICTED_VELOCITY, "")
    _track(Vector3(-LIMIT_DISTANCE, 0.0, 0.0), Vector3(LIMIT_DISTANCE, 0.0, 0.0), null, LINE_VELOCITY, "line")
    _track(Vector3(LIMIT_DISTANCE, 0.0, 0.0), Vector3(LINE_LENGTH, 0.0, 0.0), null, RESTRICTED_VELOCITY, "")
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("line", LIMIT_DISTANCE)
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, 1)
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()

    _update(route, vehicle, trainset, 0.0)
    assert_eq(route.velocity_next, RESTRICTED_VELOCITY, "standing, the limit is read")
    _update(route, vehicle, trainset, MOVING_SPEED)

    assert_lt(route.reach, LIMIT_DISTANCE, "moving, the reach no longer covers it")
    assert_eq(route.velocity_next, RESTRICTED_VELOCITY, "but what was traced stays until passed")


func test_a_switch_thrown_ahead_is_traced_again_from() -> void:
    _track(Vector3(-APPROACH_LENGTH, 0.0, 0.0), Vector3.ZERO, null, LINE_VELOCITY, "approach")
    var switch_track:RID = _track(Vector3.ZERO, Vector3(SWITCH_LENGTH, 0.0, 0.0),
            _curve(Vector3.ZERO, Vector3(SWITCH_LENGTH, 0.0, DIVERGING_OFFSET)), LINE_VELOCITY, "", TrackServer.TRACK_SWITCH)
    _track(Vector3(SWITCH_LENGTH, 0.0, 0.0), Vector3(LINE_LENGTH, 0.0, 0.0), null, LINE_VELOCITY, "")
    _track(Vector3(SWITCH_LENGTH, 0.0, DIVERGING_OFFSET), Vector3(LINE_LENGTH, 0.0, LINE_LENGTH), null,
            DIVERGING_VELOCITY, "")
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_track), TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("approach", APPROACH_LENGTH / 2.0)
    # the way that leads over the switch
    var toward_switch:int = 1
    for segment:TrackRouteSegment in RailVehicleServer.vehicle_trace_route(vehicle, -1, APPROACH_LENGTH):
        if segment.track_rid == switch_track:
            toward_switch = -1
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, toward_switch)
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()

    _update(route, vehicle, trainset, 0.0)
    assert_eq(route.velocity_next, LINE_VELOCITY, "set straight, the line's speed")
    TrackServer.switch_set_active_track(TrackServer.track_get_switch(switch_track), TrackServer.TRACK_DIVERGING)
    _update(route, vehicle, trainset, 0.0)

    assert_eq(route.velocity_next, DIVERGING_VELOCITY, "thrown, the diverging track's limit is read")


## FINDINGS.md 2026-09-29: a Tm at stop a train had passed still held it, and it braked to a stop a
## braking distance past it
func test_a_train_ignores_shunting_signals_at_stop_passed_or_ahead() -> void:
    var line:RID = _track(Vector3.ZERO, Vector3(LINE_LENGTH, 0.0, 0.0), null, LINE_VELOCITY, "line")
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("line", LINE_LENGTH / 2.0)
    var middle:Vector3 = RailVehicleServer.vehicle_get_transform(vehicle).origin
    # one Tm at the vehicle's middle, behind its front whichever way it drives, one on either side
    # further on: one of them ahead
    for offset:float in [0.0, -SHUNT_SIGNAL_DISTANCE, SHUNT_SIGNAL_DISTANCE]:
        var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
        action.command = "ShuntVelocity"
        action.position = middle + Vector3(offset, 0.0, 0.0)
        var event:RID = ScenarioEventServer.event_create()
        ScenarioEventServer.system_add_event(_event_system, event)
        _events.append(event)
        ScenarioEventServer.event_attach_action(event, action)
        MaszynaScenarioEventServer.event_set_passive(event, true)
        MaszynaScenarioEventServer.track_add_event(line, MaszynaScenarioEventServer.TRACK_EVENT1, event)
        MaszynaScenarioEventServer.track_add_event(line, MaszynaScenarioEventServer.TRACK_EVENT2, event)
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, 1)
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()

    _update(route, vehicle, trainset, MOVING_SPEED, Order.OBEY_TRAIN)

    assert_eq(route.velocity_next, MaszynaDriverRoute.NO_LIMIT, "no Tm at stop stops the train")
    assert_eq(route.commands.size(), 0, "nor turns it to shunting")


## FINDINGS.md 2026-09-29: a signal passed at proceed stayed in the table, and when it closed behind
## the train the train braked hard - krzyzowa2's 3E/1-42 did it at every block, ran its wagons'
## air down and could not stop at Drawowo's entry signal
func test_a_signal_passed_at_proceed_does_not_hold_the_train_when_it_closes() -> void:
    var line:RID = _track(Vector3.ZERO, Vector3(LINE_LENGTH, 0.0, 0.0), null, LINE_VELOCITY, "line")
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("line", LINE_LENGTH / 2.0)
    # the signal at the vehicle's middle: behind its front, whichever way it drives
    var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
    action.command = "SetVelocity"
    action.value1 = RESTRICTED_VELOCITY
    action.position = RailVehicleServer.vehicle_get_transform(vehicle).origin
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.system_add_event(_event_system, event)
    _events.append(event)
    ScenarioEventServer.event_attach_action(event, action)
    MaszynaScenarioEventServer.event_set_passive(event, true)
    MaszynaScenarioEventServer.track_add_event(line, MaszynaScenarioEventServer.TRACK_EVENT1, event)
    MaszynaScenarioEventServer.track_add_event(line, MaszynaScenarioEventServer.TRACK_EVENT2, event)
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, 1)
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()
    _update(route, vehicle, trainset, MOVING_SPEED, Order.OBEY_TRAIN)

    action.value1 = 0.0
    _update(route, vehicle, trainset, MOVING_SPEED, Order.OBEY_TRAIN)

    assert_eq(route.velocity_next, MaszynaDriverRoute.NO_LIMIT, "the signal closed behind it holds nothing")


## FINDINGS.md 2026-10-06: Stary Jawor's eszelon stood at a dwarf (Tm) it had reached at stop and
## kept "STOP" when the dwarf showed Ms2 - the speed of a signal passed was taken once, on passing
func test_a_shunting_signal_reached_at_stop_lets_go_when_it_opens() -> void:
    var line:RID = _track(Vector3.ZERO, Vector3(LINE_LENGTH, 0.0, 0.0), null, LINE_VELOCITY, "line")
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("line", LINE_LENGTH / 2.0)
    # the Tm at the vehicle's middle: behind its front, whichever way it drives
    var action:MaszynaVehicleCommandAction = _signal(line, "ShuntVelocity",
            RailVehicleServer.vehicle_get_transform(vehicle).origin)
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, 1)
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()
    _update(route, vehicle, trainset, 0.0)
    assert_eq(route.velocity_limit, 0.0, "at stop, it holds the driver")

    action.value1 = SHUNT_SPEED
    _update(route, vehicle, trainset, 0.0)

    assert_eq(route.velocity_limit, SHUNT_SPEED, "open, its speed is the limit")
    assert_eq(route.signal_velocity, SHUNT_SPEED, "and the speed allowed")
    assert_eq(route.commands, [["ShuntVelocity", SHUNT_SPEED, route.velocity_next, Vector3.ZERO]],
            "and it tells the driver to go")


## Driver.cpp:1549-1553: a player passed a signal at stop and drove on - far enough behind it holds
## the player no more, while it still holds the computer
func test_a_signal_at_stop_far_behind_holds_a_player_no_more() -> void:
    var line:RID = _track(Vector3.ZERO, Vector3(LINE_LENGTH, 0.0, 0.0), null, LINE_VELOCITY, "line")
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("line", LINE_LENGTH / 2.0)
    var middle:Vector3 = RailVehicleServer.vehicle_get_transform(vehicle).origin
    # one on either side, beyond a player's distance: one of them behind
    for offset:float in [-FAR_BEHIND_DISTANCE, FAR_BEHIND_DISTANCE]:
        _signal(line, "SetVelocity", middle + Vector3(offset, 0.0, 0.0))
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, 1)
    var computer:MaszynaDriverRoute = MaszynaDriverRoute.new()
    var player:MaszynaDriverRoute = MaszynaDriverRoute.new()

    _update(computer, vehicle, trainset, 0.0)
    _update(player, vehicle, trainset, 0.0, Order.SHUNT, true)

    assert_eq(computer.signal_velocity_last, 0.0, "the computer keeps to it")
    assert_eq(player.signal_velocity_last, MaszynaDriverRoute.NO_LIMIT, "the player is let go of it")


## FINDINGS.md 2026-09-29: a stop far ahead lost to the line speed given after it on some updates
## and not on others - lerpf() at its end is not exactly its end, std::lerp is - and the driving aid
## flickered between "0 in 0.4 km" and nothing
func test_a_stop_ahead_is_not_lost_to_the_speed_given_after_it() -> void:
    var line:RID = _track(Vector3(-LINE_LENGTH, 0.0, 0.0), Vector3(LINE_LENGTH, 0.0, 0.0), null, LINE_VELOCITY, "line")
    TrackServer.topology_rebuild()
    var vehicle:RID = await _place("line", LINE_LENGTH)
    # the signal and the speed after it ahead, towards the line's end
    for aspect:Array in [[STOP_SIGNAL_DISTANCE, 0.0], [LINE_SPEED_DISTANCE, LINE_SPEED]]:
        var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
        action.command = "SetVelocity"
        action.value1 = aspect[1]
        action.position = Vector3(float(aspect[0]), 0.0, 0.0)
        var event:RID = ScenarioEventServer.event_create()
        ScenarioEventServer.system_add_event(_event_system, event)
        _events.append(event)
        ScenarioEventServer.event_attach_action(event, action)
        MaszynaScenarioEventServer.event_set_passive(event, true)
        MaszynaScenarioEventServer.track_add_event(line, MaszynaScenarioEventServer.TRACK_EVENT2, event)
    var toward_end:bool = RailVehicleServer.vehicle_trace_route(vehicle, 1, STOP_SIGNAL_DISTANCE)[0].toward_end
    var trainset:MaszynaDriverTrainset = _trainset(vehicle, 1 if toward_end else -1)
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()

    var lost:Array[float] = []
    var speed:float = APPROACH_SPEED_MIN
    while speed <= APPROACH_SPEED_MAX:
        _update(route, vehicle, trainset, speed, Order.OBEY_TRAIN)
        if not route.velocity_next == 0.0:
            lost.append(speed)
        speed += APPROACH_SPEED_STEP

    assert_eq(lost.size(), 0, "the stop ahead is the next speed at every speed, lost at %s" % [lost])


func _curve(from:Vector3, to:Vector3) -> TrackCurve:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = from
    curve.p2 = to
    return curve


func _track(from:Vector3, to:Vector3, diverging:TrackCurve, velocity:float, name:String,
        type:TrackServer.TrackType = TrackServer.TRACK_NORMAL) -> RID:
    var track:RID = TrackServer.track_create()
    _tracks.append(track)
    TrackServer.track_update_curves(track, _curve(from, to), diverging)
    TrackServer.track_update(track, type, name, 1.435)
    TrackServer.track_set_velocity(track, velocity)
    return track


## A standing SM42 on the named track
func _place(track_name:String, offset:float) -> RID:
    var physics_node:VehiclePhysicsNode = build_vehicle_node("RouteTableTest", SM42)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    add_child(vehicle)
    _vehicles.append(vehicle)
    vehicle.controller_path = vehicle.get_path_to(physics_node)
    await wait_idle_frames(2)
    return vehicle.get_rid()


func _trainset(vehicle:RID, direction:int) -> MaszynaDriverTrainset:
    var trainset:MaszynaDriverTrainset = MaszynaDriverTrainset.new()
    trainset.update(vehicle, direction, true)
    return trainset


## A signal's memory read on both ways of `track` at `position`, at stop; its action, to change
func _signal(track:RID, command:String, position:Vector3) -> MaszynaVehicleCommandAction:
    var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
    action.command = command
    action.position = position
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.system_add_event(_event_system, event)
    _events.append(event)
    ScenarioEventServer.event_attach_action(event, action)
    MaszynaScenarioEventServer.event_set_passive(event, true)
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENT1, event)
    MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENT2, event)
    return action


## One update of the route with `order` (shunting unless given), wanting LINE_VELOCITY, at `speed`
## [km/h], by the computer unless `human_driving`
func _update(route:MaszynaDriverRoute, vehicle:RID, trainset:MaszynaDriverTrainset, speed:float,
        order:Order = Order.SHUNT, human_driving:bool = false) -> void:
    route.update(vehicle, order, false, SHUNT_SPEED, speed, MaszynaDriverSpeed.EASY_ACCELERATION,
            MaszynaDriverSpeed.NO_LIMIT, trainset, MaszynaDriverTimetable.new(), 0.0, SHUNT_SPEED,
            LINE_VELOCITY, false, MaszynaDriverBraking.new(), human_driving)
