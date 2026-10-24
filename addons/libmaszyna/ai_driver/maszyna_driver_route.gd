@tool
extends RefCounted
class_name MaszynaDriverRoute

## The original driver's speed table (TController::TableTraceRoute(), TableUpdate(),
## TableUpdateEvent(), TSpeedPos, Driver.cpp:190-1727): what lies on the tracks ahead of the
## trainset - the track limits, the switches, the end of the line, and the passive events the
## scenery speaks to drivers with (a `putvalues`/`getvalues` of SetVelocity, ShuntVelocity,
## OutsideStation, ...; MaszynaScenarioEventServer.event_is_passive()) - and what it makes of them: the next
## speed and how far it is (VelNext, ActualProximityDist), the speed at the next signal
## (VelSignalNext), a limit of the speed wanted, and the orders the driver gives itself when it sees
## a signal (SetVelocity, ShuntVelocity) or reaches a memory's command.
##
## As the original, the table is kept between updates: traced once, moved as the trainset drives,
## traced on at its end as the reach needs, traced again from a switch thrown ahead, and the events
## the front reaches take effect once (TableCheck(), TableTraceRoute()). The passenger stops
## (`PassengerStopPoint:`) are driven by the timetable (TableUpdateStopPoint()). Behind the trainset
## it looks only for a signal to turn back to (backward_scan()). Not ported yet: the section and road
## speeds, stopping at an automatic block signal (spStopOnSBL), the crossings - see TODO.md, "Drivers".

## How far ahead it reads [m]: at least MIN_RANGE; moving, MOVING_RANGE past the braking distance;
## standing, STANDING_DRIVER_DISTANCES of its distance to keep (Driver.cpp:4984-4990, fDriverDist 50)
const MIN_RANGE:float = 750.0
## The vehicles ahead are looked for at least this far [m] (scan_obstacles(), Driver.cpp:6663)
const OBSTACLE_RANGE:float = 1000.0
## A vehicle ahead nearer than this [m] is measured between the couplers, not along the track between
## the primary axles (scan_obstacles(), Driver.cpp:6672)
const OBSTACLE_COUPLER_RANGE:float = 100.0
const MOVING_RANGE:float = 400.0
const STANDING_RANGE:float = 1500.0
## Moving faster than this [km/h] (EU07_AI_MOVEMENT)
const MOVEMENT_SPEED:float = 1.0
## determine_braking_distance() (Driver.cpp:6597-6618): fDriverBraking of a train, the speed floor,
## a heavy train, the G setting's reaction
const DRIVER_BRAKING:float = 0.06
const BRAKING_SPEED_FLOOR:float = 2.0
const BRAKING_SPEED_OFFSET:float = 40.0
const HEAVY_MASS:float = 1000000.0
const HEAVY_FACTOR:float = 2.0
const DECELERATION_FACTOR:float = 25.92
const G_REACTION_FACTOR:float = 2.0
## A braking threshold past this [m/s2] takes the braking distance from the deceleration instead
const THRESHOLD_BRAKING:float = 0.05
## determine_proximity_ranges() (Driver.cpp:6684-6812) [m]: shunting 5/10 plus a vehicle each, at
## most 25/50; train 5/10 plus a vehicle each within 10-15/15-40, a goods train 10 more, standing
## 50; coupling up 2/5 and, once coupling, right up to it; uncoupling 1/10; anything else 5/10
const SHUNT_MIN_BASE:float = 5.0
const SHUNT_MIN_MAX:float = 25.0
const SHUNT_MAX_BASE:float = 10.0
const SHUNT_MAX_MAX:float = 50.0
const TRAIN_MIN_BASE:float = 5.0
const TRAIN_MIN_LOW:float = 10.0
const TRAIN_MIN_HIGH:float = 15.0
const TRAIN_MAX_BASE:float = 10.0
const TRAIN_MAX_LOW:float = 15.0
const TRAIN_MAX_HIGH:float = 40.0
const CARGO_PROXIMITY:float = 10.0
## A vehicle braking by an EIM controller keeps this much further when shunting (Driver.cpp:6717-6721),
## and a trainset of more than one vehicle under rain or snow this much further (Driver.cpp:6722-6727,
## 6759-6764) [m]
const MODERN_PROXIMITY:float = 5.0
const WEATHER_PROXIMITY:float = 5.0
## A late train's speed margins (moveLate, Driver.cpp:6772-6777) [km/h]
const LATE_VELOCITY_MINUS:float = 1.0
const LATE_VELOCITY_PLUS:float = 5.0
const STANDING_MAX_PROXIMITY:float = 50.0
const STANDING_SPEED:float = 0.1
const CONNECT_MIN_PROXIMITY:float = 2.0
const CONNECT_MAX_PROXIMITY:float = 5.0
const COUPLING_MIN_PROXIMITY:float = -1.0
const COUPLING_MAX_PROXIMITY:float = 0.0
const DISCONNECT_MIN_PROXIMITY:float = 1.0
const DISCONNECT_MAX_PROXIMITY:float = 10.0
const OTHER_MIN_PROXIMITY:float = 5.0
const OTHER_MAX_PROXIMITY:float = 10.0
## ... and the speed [km/h] run over a limit before braking (fVelPlus) and under it before adding
## power (fVelMinus): shunting 2 and a tenth of the shunting speed, at most 3; a train 5% of the
## speed wanted, 2-5 over and 1-5 under; coupling up 2/1, once coupling and uncoupling 1/0.5;
## anything else 2/5
const SHUNT_VELOCITY_PLUS:float = 2.0
const SHUNT_VELOCITY_MINUS_SHARE:float = 0.1
const SHUNT_VELOCITY_MINUS_MAX:float = 3.0
const TRAIN_VELOCITY_SHARE:float = 0.05
const TRAIN_VELOCITY_PLUS_LOW:float = 2.0
const TRAIN_VELOCITY_MINUS_LOW:float = 1.0
const TRAIN_VELOCITY_HIGH:float = 5.0
const CONNECT_VELOCITY_PLUS:float = 2.0
const CONNECT_VELOCITY_MINUS:float = 1.0
const CLOSE_VELOCITY_PLUS:float = 1.0
const CLOSE_VELOCITY_MINUS:float = 0.5
const OTHER_VELOCITY_PLUS:float = 2.0
const OTHER_VELOCITY_MINUS:float = 5.0
## TableUpdate() (Driver.cpp:940-960): the target's acceleration is eased towards the preferred one
## while further than this share of the braking distance
const EASING_BRAKING_SHARE:float = 1.2
## Standing, an event behind asks for this [m/s2] (Driver.cpp:988)
const BEHIND_ACCELERATION:float = -2.0
## -1 is no limit (min_speed())
const NO_LIMIT:float = -1.0
## No current limit read, or no switch under the trainset (VelLimitLastDist, SwitchClearDist,
## Driver.cpp:7247-7248)
const NO_DISTANCE:float = -1.0
## A current limit unbroken through the whole reading lasts at least this far [m]
## (EU07_AI_SPEEDLIMITEXTENDSBEYONDSCANRANGE, Driver.h:27)
const LIMIT_BEYOND_RANGE:float = 10000.0
## A speed the driver takes as an order to go (TableUpdateEvent(), Driver.cpp:1680)
const GO_SPEED:float = 1.0
## A human driver drops a signal passed further than the trainset's length plus the margin, and at
## least the distance [m] (TableUpdateEvent(), Driver.cpp:1551)
const HUMAN_PASSED_SIGNAL_MARGIN:float = 100.0
const HUMAN_PASSED_SIGNAL_DISTANCE:float = 250.0
## A signal speaks to a driver only while it drives - shunting, as a train, banking - or waits for
## orders; an order with anything else in it hears nothing (check_route_ahead(), Driver.cpp:8321-8335)
const TAKES_SPEED:int = (MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT
        | MaszynaAIDriver.Order.OBEY_TRAIN | MaszynaAIDriver.Order.BANK)
## Sends a memory's command only nearly standing (check_route_ahead(), Driver.cpp:8340)
const COMMAND_SPEED:float = 0.1
## TableUpdateStopPoint() (Driver.cpp:866, 1093-1380): a passenger stop is reached within this [m] of
## the front of the trainset, passed without stopping within PASSING_SHARE of it; another station's
## stop nearer than REWIND_BRAKE_SHARE of the braking distance plus REWIND_DISTANCE [m] is where the
## timetable goes on from
const PASSENGER_STOP_MAX_DISTANCE:float = 400.0
const PASSING_SHARE:float = 0.5
const REWIND_BRAKE_SHARE:float = 1.15
const REWIND_DISTANCE:float = 300.0
## The next station is shown as current this far [m] past a stop left, or past one passed at
## speed, plus the trainset's length (fLastStopExpDist, Driver.cpp:1131, 1281)
const NEXT_STATION_AFTER_DEPARTURE:float = 50.0
const NEXT_STATION_AFTER_PASSING:float = 250.0
## No next station waiting to be shown (fLastStopExpDist = -1, Driver.cpp:6487)
const NEXT_STATION_SHOWN:float = -1.0
## No signal read yet (d_to_next_sem, Driver.cpp:877)
const NO_SIGNAL_DISTANCE:float = 10000.0
## Standing still [km/h] (Driver.cpp:921, 1080)
const STOPPED_SPEED:float = 0.01
## An odd first number of a stop keeps the train there until the way is clear (Driver.cpp:1317)
const HOLD_PARITY:int = 2
## The event lists of a track, by the way it is driven (CheckTrackEvent(), Driver.cpp:459-470)
const EVENTS_TOWARD_END:int = MaszynaScenarioEventServer.TRACK_EVENT2
const EVENTS_TOWARD_START:int = MaszynaScenarioEventServer.TRACK_EVENT1
## How far behind the trainset a signal to turn back to is looked for [m] (check_route_behind( 1000 ),
## Driver.cpp:7298 - "legacy scan range value")
const BACKWARD_RANGE:float = 1000.0
## What a signal behind the trainset asks of it (BackwardScan(), TCommandType, Driver.cpp:5407-5579)
enum BackwardCommand { NONE, SET_VELOCITY, SHUNT_VELOCITY, COMMAND }
## The orders a driver looks behind in - shunting, coupling up, or none at all (Driver.cpp:5413)
const SHUNTING_ORDERS:int = MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT \
        | MaszynaAIDriver.Order.CONNECT
## A memory's text that is a speed, not a command for a standing driver (TMemCell::IsVelocity())
const VELOCITY_COMMANDS:Array[String] = ["SetVelocity", "ShuntVelocity", "OutsideStation", "SetProximityVelocity"]

## What an entry means (TSpeedPosFlag, Driver.h:127-149)
enum Kind { TRACK, SWITCH, LINE_END, SEMAPHORE, SHUNT_SEMAPHORE, OUTSIDE_STATION, COMMAND, STOP_POINT, OTHER }
## What a passenger stop asks of the driver's orders, in the order asked (TableUpdateStopPoint()):
## HOLD and GO set whether it waits for the way to be clear (moveStopHere); OBEY_TRAIN drives on as
## a train; TURN_THEN_TRAIN and TURN_THEN_SHUNT turn a push-pull train by its cab, then drive on;
## NEXT_ORDER takes the next order; GUARD_SIGNAL - it left a stop, the guard's message is due
## (moveGuardSignal, Driver.cpp:1313-1316); LOAD_EXCHANGE - it arrived at a platform, on the side of
## `exchange_platform`, and its passengers get off and on (Driver.cpp:1233-1241)
## START_HORN / NO_START_HORN: the horn before moving off due, or not, after this stop
## (moveStartHorn, Driver.cpp:1240, 1375)
enum StopOrder {
    HOLD, GO, OBEY_TRAIN, TURN_THEN_TRAIN, TURN_THEN_SHUNT, NEXT_ORDER, GUARD_SIGNAL, LOAD_EXCHANGE, START_HORN,
    NO_START_HORN,
}
## The platform's digit of a passenger stop's second number - its last one (`% 10`,
## Driver.cpp:1236): 1 on the left of the way the train drives, 2 on the right, 3 both; any other
## is no platform, and nothing is exchanged (TDynamicObject::LoadExchange(), DynObj.cpp:2828)
const PLATFORM_DIGITS:int = 10
const PLATFORM_SIDES:Dictionary[int, RailVehicleLoad.PlatformSide] = {
    1: RailVehicleLoad.PLATFORM_SIDE_LEFT,
    2: RailVehicleLoad.PLATFORM_SIDE_RIGHT,
    3: RailVehicleLoad.PLATFORM_SIDE_BOTH,
}
## What a passenger stop is on this reading: an entry to take as it is, one to skip, or one that let
## the train go (cm_Ready)
enum StopResult { USE, SKIP, READY }

## One entry of the table (TSpeedPos)
class Entry:
    var kind:Kind
    ## from the front of the trainset [m], negative once passed
    var distance:float
    ## the speed from here on, NO_LIMIT for none (fVelNext)
    var velocity:float
    var length:float
    var event:RID
    ## the command the event carries, read live from its memory (input_command())
    var command:String
    ## the station of a passenger stop
    var station:String
    var value1:float
    var value2:float
    ## where the event stands, sent with its command
    var position:Vector3
    ## where it stands along the table [m], and the track it is on
    var along:float
    var segment:TrackRouteSegment
    ## the event's action, its command read again on every update (TSpeedPos::Update())
    var action:MaszynaVehicleCommandAction
    ## the front has reached it, or it was behind the front when traced
    var passed:bool

## VelNext, ActualProximityDist
var velocity_next:float = NO_LIMIT
var proximity_distance:float = MIN_RANGE
## How far ahead it read on the last update [m]
var reach:float = MIN_RANGE
## VelSignalNext, VelSignalLast
var signal_velocity_next:float = 0.0
var signal_velocity_last:float = NO_LIMIT
## The limit TableUpdate() puts on the speed wanted (fVelDes)
var velocity_limit:float = NO_LIMIT
## VelLimitLastDist: the speed wanted on the last update, and how far [m] the current limit under
## it lasts - NO_DISTANCE while none holds
var velocity_limit_last:float = NO_LIMIT
var velocity_limit_last_distance:float = NO_DISTANCE
## fMinProximityDist, fMaxProximityDist, fBrakeDist
var min_proximity:float = OTHER_MIN_PROXIMITY
var max_proximity:float = OTHER_MAX_PROXIMITY
var brake_distance:float = 0.0
## fVelPlus, fVelMinus [km/h]
var velocity_plus:float = OTHER_VELOCITY_PLUS
var velocity_minus:float = OTHER_VELOCITY_MINUS
## The speed allowed after this update (VelSignal) and the orders it gives itself: [command,
## value1, value2, position]
var signal_velocity:float = 0.0
var commands:Array[Array] = []
## The nearest vehicle ahead (Obstacle), null for none, and its speed [km/h]
var obstacle:RailVehicleNeighbour = null
var obstacle_speed:float = 0.0
## Standing at its passenger stop (IsAtPassengerStop)
var at_passenger_stop:bool = false
## Standing at its passenger stop before the departure time (the else of TableUpdateStopPoint(),
## Driver.cpp:1341-1342), read afresh on every update
var waiting_for_departure:bool = false
## What its passenger stop asked of the orders on this update
var stop_orders:Array[StopOrder] = []
## The side of the platform LOAD_EXCHANGE asks about
var exchange_platform:RailVehicleLoad.PlatformSide = RailVehicleLoad.PLATFORM_SIDE_BOTH
## The passenger stops done with, by event, until it has left them (TSpeedPos::iFlags = 0)
var _stops_done:Dictionary[RID, bool] = {}
## How far a passenger stop was brought forward for the train's length and the platform, by event
## (TSpeedPos::fMoved)
var _stops_moved:Dictionary[RID, float] = {}
## How far [m] the train still has to drive along the route before the next station is shown as
## current (fLastStopExpDist, counted on the Mover's DistCounter there), NEXT_STATION_SHOWN for none
var _next_station_distance:float = NEXT_STATION_SHOWN
## The next station's stop was read on this update (IsScheduledPassengerStopVisible)
var _scheduled_stop_visible:bool = false
## The way the driver drove when the tracks were last read (iTableDirection), 0 to read them afresh
var _direction:int = 0
## The memories' commands it sent, by event, not to send them again until they change
## (StopCommandSent(), MemCell.cpp:196-205)
var _sent:Dictionary[RID, String] = {}
## The table (sSpeedTable): the tracks traced from the front vehicle, entered at `distance` along
## the table, and the entries on them, nearest first; along the table counts from where the front
## vehicle's middle stood when the table was first traced [m]
var _segments:Array[TrackRouteSegment] = []
var _table:Array[Entry] = []
## The vehicle the table is traced from, and where its middle is along the table [m]
var _front:RID = RID()
var _front_along:float = 0.0


## One reading of the tracks ahead (TableCheck(), TableUpdate(), check_route_ahead()) for the driver
## of `vehicle`: `allowed` is the speed allowed now (VelSignal), `speed` the trainset's along the way
## it drives [km/h], `acceleration` the one preferred (AccPreferred) [m/s2]; `timetable` how far it
## got, at `hours` of the day; `shunt_velocity` and `velocity_desired` the driver's [km/h], `coupling`
## whether it is coupling up now (moveConnect), `human_driving` whether a player drives it
## (AIControllFlag false). The speed allowed afterwards is `signal_velocity`, the orders it gives
## itself `commands` and `stop_orders`.
func update(
    vehicle:RID, order:int, stop_here:bool, allowed:float, speed:float, acceleration:float, velocity_max:float,
    trainset:MaszynaDriverTrainset, timetable:MaszynaDriverTimetable, hours:float,
    shunt_velocity:float, velocity_desired:float, coupling:bool, braking:MaszynaDriverBraking,
    human_driving:bool
) -> void:
    commands.clear()
    stop_orders.clear()
    at_passenger_stop = false
    waiting_for_departure = false
    # read the other way, or afresh: what was ahead is not passed and no stop is done (TableClear()),
    # and the stop of a signal passed does not keep it from reversing (TableCheck(), Driver.cpp:510-526)
    if not trainset.direction == _direction:
        _direction = trainset.direction
        _clear_table()
        _stops_done.clear()
        _stops_moved.clear()
        if signal_velocity_last == 0.0:
            signal_velocity_last = NO_LIMIT
    _scheduled_stop_visible = false
    # IsCargoTrain (Driver.cpp:2303): keeps further from a stop, leaves a passenger stop at once
    var trainset_type:RailVehicleServer.TrainsetType = RailVehicleServer.trainset_get_type(vehicle)
    var cargo:bool = trainset_type == RailVehicleServer.TRAINSET_TYPE_CARGO
    # moveLate: late on leaving the last station (UpdateDelayFlag(), Driver.cpp:5683-5692)
    _determine_distances(vehicle, order, speed, trainset, shunt_velocity, velocity_desired, coupling, cargo,
            braking.acceleration_threshold, timetable.latency < 0.0)
    reach = maxf(MIN_RANGE, MOVING_RANGE + brake_distance if absf(speed) > MOVEMENT_SPEED else STANDING_RANGE)
    var front_along:float = _front_along
    _update_table(reach, trainset)
    # the next station shown once the train has driven clear of the stop left (UpdateNextStop(),
    # Driver.cpp:6481, run for a train only, Driver.cpp:7154); the way driven is the front's along
    # the table - a table traced afresh starts from 0 and counts nothing
    if _next_station_distance >= 0.0:
        _next_station_distance = maxf(0.0, _next_station_distance - maxf(0.0, _front_along - front_along))
        if order == MaszynaAIDriver.Order.OBEY_TRAIN and _next_station_distance == 0.0:
            _next_station_distance = NEXT_STATION_SHOWN
            timetable.show_next_station(hours)
    # the passenger stops left behind are forgotten
    var read:Dictionary[RID, bool] = {}
    for entry:Entry in _table:
        if entry.kind == Kind.STOP_POINT:
            read[entry.event] = true
    for event:RID in _stops_done.keys():
        if not read.has(event):
            _stops_done.erase(event)
    for event:RID in _stops_moved.keys():
        if not read.has(event):
            _stops_moved.erase(event)
    var signal_distance:float = NO_SIGNAL_DISTANCE
    var obey_train:bool = order & MaszynaAIDriver.Order.OBEY_TRAIN
    # the events the front has reached; one dropped from the table unreached - a switch ahead
    # thrown - is not passed (FINDINGS.md, 2026-09-29)
    for entry:Entry in _table:
        if entry.event.is_valid() and not entry.passed and entry.distance <= 0.0:
            entry.passed = true

    velocity_next = NO_LIMIT
    proximity_distance = reach
    velocity_limit = NO_LIMIT
    velocity_limit_last = velocity_desired
    velocity_limit_last_distance = NO_DISTANCE
    # SwitchClearDist, and whether the current limit is unbroken through the reading
    # (speedlimitiscontinuous, Driver.cpp:862)
    var switch_clear_distance:float = NO_DISTANCE
    var limit_continuous:bool = true
    var best_acceleration:float = acceleration
    var signal_found:bool = false
    var go:String = ""
    var command_entry:Entry = null
    var let_go:Array[Entry] = []
    for entry:Entry in _table:
        if entry.kind == Kind.STOP_POINT:
            var result:StopResult = _update_stop_point(
                    entry, order, absf(speed), cargo, trainset, timetable, hours, signal_distance)
            if result == StopResult.READY and go.is_empty():
                go = "Ready"
            if not result == StopResult.USE:
                continue
        var velocity:float = entry.velocity
        var distance:float = entry.distance
        # a limit goes on through the switches and the passenger stops within it (Driver.cpp:1016)
        var breaks_limit:bool = not (entry.kind == Kind.SWITCH or entry.kind == Kind.STOP_POINT)
        if entry.kind == Kind.SWITCH:
            # the trainset is on a switch until it has left it (Driver.cpp:880-883)
            switch_clear_distance = distance + entry.length + trainset.length
        # a signal the front has reached gives the speed in force, read again on every update while
        # it is in the table - taken once on passing, a Tm at stop held the player when it opened
        # (TableUpdateEvent(), Driver.cpp:1545-1558, FINDINGS.md 2026-10-06); a player's is dropped
        # once passed far enough
        if entry.event.is_valid() and distance <= 0.0 and _is_proper_semaphore(entry.kind, obey_train):
            if human_driving and distance < -maxf(trainset.length + HUMAN_PASSED_SIGNAL_MARGIN,
                    HUMAN_PASSED_SIGNAL_DISTANCE):
                allowed = NO_LIMIT
                let_go.append(entry)
                continue
            signal_velocity_last = velocity
        if entry.event.is_valid():
            if distance > 0.0:
                if _is_proper_semaphore(entry.kind, obey_train):
                    signal_distance = minf(distance, signal_distance)
                if _is_proper_semaphore(entry.kind, obey_train) and not signal_found:
                    # the nearest signal (TableUpdateEvent(), Driver.cpp:1585-1602)
                    signal_found = true
                    signal_velocity_next = entry.velocity
                    if velocity < 0.0:
                        velocity = velocity_max
                        allowed = velocity_max
                match entry.kind:
                    Kind.OUTSIDE_STATION:
                        # a train goes on past it; shunting ends there
                        velocity = NO_LIMIT if obey_train else 0.0
                    Kind.SHUNT_SEMAPHORE:
                        if obey_train and velocity == 0.0:
                            velocity = NO_LIMIT
                        elif not velocity == 0.0 and go.is_empty():
                            go = "ShuntVelocity"
                            if allowed == 0.0:
                                allowed = velocity
                    Kind.SEMAPHORE:
                        if (velocity < 0.0 or velocity >= GO_SPEED) and go.is_empty():
                            go = "SetVelocity"
                            if allowed == 0.0:
                                allowed = NO_LIMIT
                    Kind.COMMAND:
                        # a memory's command for a standing driver, sent once (cm_Command, Driver.cpp:1708-1723)
                        if go.is_empty() and not _sent.get(entry.event, "") == _signature(entry) \
                                and (stop_here or distance <= max_proximity):
                            go = "Command"
                            command_entry = entry
                    Kind.OTHER:
                        continue
            elif entry.kind == Kind.OTHER or entry.kind == Kind.COMMAND:
                continue
            elif entry.kind == Kind.SHUNT_SEMAPHORE and obey_train and velocity == 0.0:
                # a train ignores a Tm at stop passed as much as one ahead, and lets go of it
                # (TableUpdateEvent(), Driver.cpp:1618-1626)
                let_go.append(entry)
                continue
            elif entry.kind == Kind.SHUNT_SEMAPHORE and not velocity == 0.0 and go.is_empty():
                # passed, a Tm letting it go gives its speed and is let go of - the player stands at
                # the dwarf, often a metre past it (TableUpdateEvent(), Driver.cpp:1649-1665)
                go = "ShuntVelocity"
                allowed = velocity
                let_go.append(entry)
                continue
            elif (entry.kind == Kind.SEMAPHORE or (entry.kind == Kind.OUTSIDE_STATION and obey_train)) \
                    and (velocity < 0.0 or velocity >= GO_SPEED) and go.is_empty():
                # passed showing the way on, it gives its speed and is let go of: its fall to stop
                # behind the train holds nothing - kept, it braked the train hard every time a
                # signal behind it closed (TableUpdateEvent(), Driver.cpp:1662-1679)
                go = "SetVelocity"
                allowed = NO_LIMIT
                let_go.append(entry)
                continue
        # a point without a limit breaks the current one (Driver.cpp:1022-1029)
        if entry.velocity < 0.0 and breaks_limit:
            limit_continuous = false
        var line_end:bool = entry.kind == Kind.LINE_END
        if velocity < 0.0 and not line_end:
            continue
        var wanted:float = acceleration
        if distance > 0.0:
            if velocity >= 0.0:
                wanted = (velocity * velocity - speed * speed) / (DECELERATION_FACTOR * distance)
                if speed < velocity or velocity == 0.0:
                    # plenty of room to brake: keep the preferred acceleration, easing into braking
                    var easing:float = EASING_BRAKING_SHARE * brake_distance \
                            * braking.distance_multiplier(velocity, absf(speed), trainset, trainset_type)
                    if easing > 0.0:
                        # std::lerp gives its end exactly (Driver.cpp:919), lerpf() may miss it by a
                        # rounding: a far stop then read above the preferred acceleration and lost to
                        # the speed after it (FINDINGS.md 2026-09-29)
                        var share:float = clampf((distance - easing) / easing, 0.0, 1.0)
                        wanted = acceleration if share == 1.0 else lerpf(wanted, acceleration, share)
                if distance < min_proximity:
                    velocity_limit = MaszynaDriverSpeed.min_speed(velocity_limit, velocity)
        elif entry.event.is_valid():
            # an event behind holds only a stop (Driver.cpp:984-990)
            wanted = acceleration if velocity > 0.0 else (0.0 if absf(speed) < COMMAND_SPEED else BEHIND_ACCELERATION)
        else:
            # a track the trainset is still on: its limit holds until it has left it
            if velocity >= GO_SPEED and distance + entry.length < -trainset.length and not line_end:
                continue
            velocity_limit = MaszynaDriverSpeed.min_speed(velocity_limit, velocity)
            # the current limit lasts until the trainset has left it (Driver.cpp:945-951)
            if velocity >= 0.0 and velocity < velocity_limit_last:
                velocity_limit_last_distance = distance + entry.length + trainset.length
            elif velocity_limit_last_distance > 0.0 and breaks_limit:
                limit_continuous = false
            if not line_end:
                continue
        if line_end:
            # the end of the line is a stop of its own (Driver.cpp:991-1001)
            var stopping:float = -speed * speed / (DECELERATION_FACTOR * maxf(distance + entry.length, COMMAND_SPEED))
            if stopping < wanted:
                wanted = stopping
                velocity = 0.0
                distance += entry.length
                if distance < min_proximity:
                    velocity_limit = MaszynaDriverSpeed.min_speed(velocity_limit, 0.0)
        if wanted <= best_acceleration and (velocity < velocity_next or velocity_next < 0.0):
            best_acceleration = wanted
            velocity_next = velocity
            proximity_distance = distance
        elif wanted > 0.0 and wanted <= best_acceleration and velocity >= 0.0 and velocity_next < 0.0:
            best_acceleration = wanted
            velocity_next = velocity
            proximity_distance = distance
        # a point behind, or one right after the current limit, is part of it; an event has no
        # length (Driver.cpp:1005-1019)
        if velocity >= 0.0 and velocity < velocity_limit_last \
                and (distance < 0.0 or velocity_limit_last_distance > 0.0):
            velocity_limit_last_distance = distance + entry.length + trainset.length
        elif breaks_limit:
            limit_continuous = false
        if velocity_next == 0.0:
            break
    for entry:Entry in let_go:
        _table.erase(entry)
    # no signal ahead any more: the last one's speed is forgotten on the line (Driver.cpp:1030-1034)
    if obey_train and not signal_found:
        signal_velocity_last = NO_LIMIT
    # a limit unbroken through the reading lasts beyond it; a signal's lasts until the trainset has
    # left the switches behind it (Driver.cpp:1050-1057)
    if velocity_limit_last_distance > 0.0 and limit_continuous:
        velocity_limit_last_distance = LIMIT_BEYOND_RANGE
    if signal_velocity_last >= 0.0 and switch_clear_distance >= 0.0:
        velocity_limit_last_distance = maxf(velocity_limit_last_distance, switch_clear_distance)
    # standing at its passenger stop, it holds there (Driver.cpp:1080-1082)
    if at_passenger_stop and absf(speed) < STOPPED_SPEED:
        velocity_limit = 0.0
    else:
        velocity_limit = MaszynaDriverSpeed.min_speed(velocity_limit, signal_velocity_last)
    # a stop let it go: on at once, unless something ahead holds it (check_route_ahead(), cm_Ready)
    if go == "Ready" and not velocity_next == 0.0 and timetable.stop_closer:
        allowed = NO_LIMIT
    signal_velocity = allowed
    # the orders it gives itself (check_route_ahead(), Driver.cpp:8302-8356)
    match go:
        "SetVelocity":
            if absf(allowed) >= GO_SPEED and not order & ~TAKES_SPEED:
                commands.append(["SetVelocity", allowed, velocity_next, Vector3.ZERO])
        "ShuntVelocity":
            # not while it couples, uncouples or turns: a shunting speed there cancels the uncoupling
            # (vehicle_count reset) - the eszelon drove on with all its wagons
            if not order & ~TAKES_SPEED:
                commands.append(["ShuntVelocity", allowed, velocity_next, Vector3.ZERO])
        "Command":
            if absf(speed) < COMMAND_SPEED:
                commands.append([command_entry.command, command_entry.value1, command_entry.value2, command_entry.position])
                _sent[command_entry.event] = _signature(command_entry)
    # the vehicles ahead, from the front of the trainset the way it drives (scan_obstacles(),
    # Driver.cpp:6638-6680)
    obstacle = null
    obstacle_speed = 0.0
    if trainset.vehicles:
        obstacle = RailVehicleServer.vehicle_find_vehicle(
                trainset.vehicles[0],
                RailVehicleController.COUPLER_END_FRONT if trainset.front_direction > 0 else RailVehicleController.COUPLER_END_REAR,
                maxf(OBSTACLE_RANGE, reach))
    if obstacle:
        obstacle_speed = VehicleServer.vehicle_get_speed(obstacle.vehicle_rid)
        # at short distances the range is measured between the couplers (Driver.cpp:6672-6675)
        if obstacle.distance < OBSTACLE_COUPLER_RANGE:
            obstacle.distance = RailVehicleServer.vehicle_get_coupler_distance(
                    trainset.vehicles[0], obstacle.vehicle_rid)


## The table brought up to date (TableCheck(), TableTraceRoute(), Driver.cpp:430-779): where the
## front is along it, traced again from a switch ahead thrown since, the tracks the whole trainset
## has left dropped, traced on to cover `reach` [m], and every entry's distance from the front and
## its command read again. Traced afresh from the front vehicle when it changed or the table does not
## hold the track it stands on.
func _update_table(reach:float, trainset:MaszynaDriverTrainset) -> void:
    var here:Array[TrackRouteSegment] = []
    if trainset.vehicles:
        here = RailVehicleServer.vehicle_trace_route(trainset.vehicles[0], trainset.front_direction, 0.0)
    if not here:
        _clear_table()
        return
    var front:RID = trainset.vehicles[0]
    # the placement is the vehicle's middle; the distances count from the trainset's front
    var front_offset:float = VehicleServer.vehicle_get_dimensions(front).z / 2.0
    var found:bool = false
    var along:float = 0.0
    if front == _front:
        # the track it stands on, the entry nearest where it was should the route cross it twice
        for segment:TrackRouteSegment in _segments:
            if segment.track_rid == here[0].track_rid and segment.toward_end == here[0].toward_end:
                var candidate:float = segment.distance - here[0].distance
                if not found or absf(candidate - _front_along) < absf(along - _front_along):
                    along = candidate
                found = true
    if not found:
        # traced afresh: what is behind the front already is not passed now
        _clear_table()
        _front = front
        _append(here, front_offset)
    _front_along = along
    var front_along:float = along + front_offset
    for index:int in _segments.size():
        var segment:TrackRouteSegment = _segments[index]
        if segment.branch_from_setting and segment.distance > front_along \
                and not TrackServer.switch_get_active_track(TrackServer.track_get_switch(segment.track_rid)) \
                    == segment.branch:
            _forget_segments(_segments.slice(index))
            break
    var left:Array[TrackRouteSegment] = []
    for segment:TrackRouteSegment in _segments.slice(0, _segments.size() - 1):
        if segment.distance + segment.length >= front_along - trainset.length:
            break
        left.append(segment)
    _forget_segments(left)
    # the last track traced again with what follows: it may turn out to end the line
    var last:TrackRouteSegment = _segments.back()
    if not last.line_end and last.distance + last.length < front_along + reach:
        var last_only:Array[TrackRouteSegment] = [last]
        _forget_segments(last_only)
        _append(TrackServer.track_trace_route(last.track_rid, last.branch, last.branch_from_setting, last.toward_end,
                last.distance, front_along + reach), front_along)
    for entry:Entry in _table:
        entry.distance = entry.along - front_along
        if entry.action:
            _read_command(entry)
        else:
            # a trackvel event changes a track's limit (TSpeedPos::Update(), Driver.cpp:295-301)
            entry.velocity = TrackServer.track_get_velocity(entry.segment.track_rid)


## `segments`, traced on from the table's end, added with what stands on them: a track where the
## limit changes, a switch or the end of the line, and the passive events (TableTraceRoute(),
## Driver.cpp:589-779); an event behind `front_along` [m] already counts as passed
func _append(segments:Array[TrackRouteSegment], front_along:float) -> void:
    var last_velocity:float = _segments.back().velocity if _segments else NO_LIMIT - 1.0
    var added:Array[Entry] = []
    for segment:TrackRouteSegment in segments:
        _segments.append(segment)
        # the events first, as the track is entered
        var slot:int = EVENTS_TOWARD_END if segment.toward_end else EVENTS_TOWARD_START
        for event:RID in MaszynaScenarioEventServer.track_get_events(segment.track_rid, slot):
            var action:MaszynaVehicleCommandAction = ScenarioEventServer.event_get_action(event) as MaszynaVehicleCommandAction
            if not MaszynaScenarioEventServer.event_is_passive(event) or not action:
                continue
            var entry:Entry = Entry.new()
            entry.event = event
            entry.action = action
            entry.segment = segment
            entry.position = action.position
            # where its position meets the track (GetDistanceToEvent())
            var curve:Curve3D = TrackServer.track_get_domain_curve(segment.track_rid)
            var offset:float = curve.get_closest_offset(action.position) if curve else 0.0
            entry.along = segment.distance + (offset if segment.toward_end else segment.length - offset)
            entry.passed = entry.along <= front_along
            _read_command(entry)
            added.append(entry)
        if segment.track_switch or segment.velocity == 0.0 or not segment.velocity == last_velocity or segment.line_end:
            var entry:Entry = Entry.new()
            entry.kind = Kind.LINE_END if segment.line_end else (Kind.SWITCH if segment.track_switch else Kind.TRACK)
            entry.segment = segment
            entry.along = segment.distance
            entry.velocity = segment.velocity
            entry.length = segment.length
            added.append(entry)
        last_velocity = segment.velocity
    added.sort_custom(func(a:Entry, b:Entry) -> bool: return a.along < b.along)
    _table.append_array(added)


## BackwardScan() with BackwardTraceRoute() (Driver.cpp:5313-5579): the first signal's memory read on
## the tracks behind the trainset, from its front, the way it would drive turned back, as far as
## BACKWARD_RANGE, the end of the line or a track of no speed - and whether it asks the driver to turn
## back: a shunting driver (or one waiting for orders) to a shunting signal letting it go while the
## way ahead is closed, a train's driver to a signal letting it go. A signal not yet behind the rear
## (the original's dot product of the way and the memory, Driver.cpp:5453-5462) asks nothing.
func backward_scan(order:int, trainset:MaszynaDriverTrainset) -> BackwardCommand:
    if order & ~SHUNTING_ORDERS or not trainset.vehicles:
        return BackwardCommand.NONE
    var segments:Array[TrackRouteSegment] = RailVehicleServer.vehicle_trace_route(
            trainset.vehicles[0], -trainset.front_direction, BACKWARD_RANGE)
    # the front vehicle's middle is where the trace starts; the rear lies a trainset further on
    var rear_along:float = trainset.length - VehicleServer.vehicle_get_dimensions(trainset.vehicles[0]).z / 2.0
    for segment:TrackRouteSegment in segments:
        if segment.velocity == 0.0:
            return BackwardCommand.NONE
        var slot:int = EVENTS_TOWARD_END if segment.toward_end else EVENTS_TOWARD_START
        for event:RID in MaszynaScenarioEventServer.track_get_events(segment.track_rid, slot):
            var action:MaszynaVehicleCommandAction = ScenarioEventServer.event_get_action(event) as MaszynaVehicleCommandAction
            if not MaszynaScenarioEventServer.event_is_passive(event) or not action:
                continue
            # only a memory read (getvalues) - a putvalues says nothing behind (Driver.cpp:5441-5444)
            if not action.source.is_valid():
                return BackwardCommand.NONE
            var curve:Curve3D = TrackServer.track_get_domain_curve(segment.track_rid)
            var offset:float = curve.get_closest_offset(action.position) if curve else 0.0
            if segment.distance + (offset if segment.toward_end else segment.length - offset) < rear_along:
                return BackwardCommand.NONE
            var command:String = MaszynaScenarioEventServer.memory_get_text(action.source)
            var velocity:float = MaszynaScenarioEventServer.memory_get_value1(action.source)
            # a stop signal pulls a shunting driver up to it (move, Driver.cpp:5468-5473)
            var pull_up:bool = command == "SetVelocity" and bool(order & (SHUNTING_ORDERS if velocity == 0.0
                    else MaszynaAIDriver.Order.CONNECT))
            # a train signal letting it go: both of the original's branches answer the same
            # (Driver.cpp:5476-5513)
            if command == "SetVelocity" and not pull_up:
                return BackwardCommand.SET_VELOCITY if velocity > 0.0 else BackwardCommand.NONE
            # Driver.cpp:5516-5576 (it sees the shunting signals waiting for orders too): not while it
            # can go on ahead, nor to a signal at stop
            if pull_up or command == "ShuntVelocity":
                return (BackwardCommand.SHUNT_VELOCITY if velocity_next == 0.0 and velocity > 0.0
                        else BackwardCommand.NONE)
            return BackwardCommand.COMMAND if command and not command in VELOCITY_COMMANDS else BackwardCommand.NONE
        if segment.line_end:
            break
    return BackwardCommand.NONE


## FirstSemaphorDist: how far ahead [m] the nearest signal is, NO_SIGNAL_DISTANCE for none
func get_first_semaphore_distance() -> float:
    for entry:Entry in _table:
        if not entry.passed and (entry.kind == Kind.SEMAPHORE or entry.kind == Kind.SHUNT_SEMAPHORE):
            return entry.distance
    return NO_SIGNAL_DISTANCE


## The table emptied, to be traced afresh from the front on the next update (TableClear())
func _clear_table() -> void:
    _front = RID()
    _segments.clear()
    _table.clear()


## `segments` taken out of the table, with what stands on them
func _forget_segments(segments:Array[TrackRouteSegment]) -> void:
    if not segments:
        return
    var kept:Array[TrackRouteSegment] = []
    for segment:TrackRouteSegment in _segments:
        if not segments.has(segment):
            kept.append(segment)
    _segments = kept
    var kept_entries:Array[Entry] = []
    for entry:Entry in _table:
        if not segments.has(entry.segment):
            kept_entries.append(entry)
    _table = kept_entries


## An event's command read from its memory, or its action, and what it means to the table
## (TSpeedPos::CommandCheck(), Driver.cpp:201-286)
func _read_command(entry:Entry) -> void:
    var action:MaszynaVehicleCommandAction = entry.action
    if action.source.is_valid():
        entry.command = MaszynaScenarioEventServer.memory_get_text(action.source)
        entry.value1 = MaszynaScenarioEventServer.memory_get_value1(action.source)
        entry.value2 = MaszynaScenarioEventServer.memory_get_value2(action.source)
    else:
        entry.command = action.command
        entry.value1 = action.value1
        entry.value2 = action.value2
    match entry.command:
        "ShuntVelocity":
            entry.kind = Kind.SHUNT_SEMAPHORE
            entry.velocity = entry.value1
        "SetVelocity":
            entry.kind = Kind.SEMAPHORE
            entry.velocity = entry.value1
        "OutsideStation":
            entry.kind = Kind.OUTSIDE_STATION
            entry.velocity = NO_LIMIT
        "SetProximityVelocity", "RoadVelocity", "SectionVelocity", "CabSignal", "Emergency_brake":
            entry.kind = Kind.OTHER
            entry.velocity = NO_LIMIT
        _:
            if entry.command.begins_with(MaszynaEventFactory.PASSENGER_STOP_POINT):
                # a stop, until the timetable says otherwise (TSpeedPos::Set(), Driver.cpp:242-246)
                entry.kind = Kind.STOP_POINT
                entry.station = entry.command.trim_prefix(MaszynaEventFactory.PASSENGER_STOP_POINT)
                entry.velocity = 0.0
            else:
                # any other text is a command for a standing driver: it stops there for it
                entry.kind = Kind.COMMAND
                entry.velocity = 0.0


## What was read of the tracks is forgotten, read afresh on the next update - a player drove the
## vehicle and may have driven it against the signals (TakeControl(), Driver.cpp:5707-5708)
func forget() -> void:
    _direction = 0


## A passenger stop on this reading (TableUpdateStopPoint(), Driver.cpp:1093-1380): another station's
## is skipped or the timetable goes on from it; the next station's is passed at speed where the train
## does not stop, else brought forward for the train and the platform, stopped at, and left at the
## departure time - or turned at, or the end of the timetable. `speed` is the vehicle's [km/h].
func _update_stop_point(
    entry:Entry, order:int, speed:float, cargo:bool, trainset:MaszynaDriverTrainset,
    timetable:MaszynaDriverTimetable, hours:float, signal_distance:float
) -> StopResult:
    var driving:int = (MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT
            | MaszynaAIDriver.Order.OBEY_TRAIN | MaszynaAIDriver.Order.BANK)
    if not order & driving or _stops_done.has(entry.event):
        return StopResult.SKIP
    if not MaszynaStations.key(entry.station) == MaszynaStations.key(timetable.next_stop):
        if not _scheduled_stop_visible and entry.distance > 0.0 \
                and entry.distance < REWIND_BRAKE_SHARE * brake_distance + REWIND_DISTANCE:
            timetable.rewind(entry.station)
        return StopResult.SKIP
    if not timetable.stop_point:
        # coupling up or turning: it drives past
        _stops_done[entry.event] = true
        return StopResult.SKIP
    _scheduled_stop_visible = true
    if not timetable.is_stop():
        # passed at speed, taken as reached a little before it
        entry.velocity = NO_LIMIT
        if entry.distance < PASSENGER_STOP_MAX_DISTANCE * PASSING_SHARE:
            timetable.arrive(hours)
            timetable.advance()
            _next_station_distance = NEXT_STATION_AFTER_PASSING + trainset.length
            _stops_done[entry.event] = true
            return StopResult.SKIP
        return StopResult.USE
    if not _stops_moved.has(entry.event):
        # the first number: negative, where the front stops before it; positive, where the middle
        # stops; the second: the platform's length, or negative - only for trains shorter than that
        var place:float = entry.value1
        var platform:float = entry.value2
        if platform < 0.0 and trainset.length >= -platform:
            _stops_done[entry.event] = true
            return StopResult.SKIP
        var moved:float = -place if place < 0.0 else place - min_proximity - trainset.length / 2.0
        _stops_moved[entry.event] = maxf(0.0, minf(moved, absf(platform) - min_proximity - trainset.length))
    var shift:float = _stops_moved[entry.event]
    entry.distance -= shift
    at_passenger_stop = entry.distance <= PASSENGER_STOP_MAX_DISTANCE and (
            entry.distance + trainset.length + shift - min_proximity / 2.0
                    <= maxf(absf(entry.value2), 2.0 * max_proximity + trainset.length)
            if timetable.stop_closer else entry.distance < signal_distance)
    if speed > MOVEMENT_SPEED:
        return StopResult.USE
    if not at_passenger_stop:
        # standing short of it: let it draw up closer
        entry.velocity = NO_LIMIT
        return StopResult.USE
    var arrived:bool = timetable.arrive(hours)
    # further stations ahead: no horn before leaving this one (Driver.cpp:1236-1241)
    if arrived and not timetable.is_last_station():
        stop_orders.append(StopOrder.NO_START_HORN)
    var platform:int = floori(absf(entry.value2)) % PLATFORM_DIGITS
    if arrived and PLATFORM_SIDES.has(platform):
        exchange_platform = PLATFORM_SIDES[platform]
        stop_orders.append(StopOrder.LOAD_EXCHANGE)
    if arrived and timetable.turns_here():
        # `@`: a push-pull train turns by its cab and stays, a locomotive goes on to its next order
        if trainset.push_pull:
            stop_orders.append(StopOrder.HOLD)
            stop_orders.append(StopOrder.TURN_THEN_SHUNT if timetable.is_last_station() else StopOrder.TURN_THEN_TRAIN)
        else:
            timetable.pass_stops()
            stop_orders.append(StopOrder.GO)
        stop_orders.append(StopOrder.NEXT_ORDER)
        timetable.stop_short()
        _stops_done[entry.event] = true
        return StopResult.SKIP
    if order & (MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT):
        stop_orders.append(StopOrder.OBEY_TRAIN)
    if timetable.is_last_station():
        # the end of the timetable: its next order, and it stays until told to go
        timetable.finish()
        timetable.stop_short()
        if not trainset.push_pull:
            timetable.pass_stops()
        stop_orders.append(StopOrder.NEXT_ORDER)
        stop_orders.append(StopOrder.HOLD)
        stop_orders.append(StopOrder.START_HORN)
        _stops_done[entry.event] = true
        return StopResult.SKIP
    if cargo or timetable.is_time_to_go(hours):
        at_passenger_stop = false
        timetable.advance()
        _next_station_distance = NEXT_STATION_AFTER_DEPARTURE + trainset.length
        stop_orders.append(StopOrder.HOLD if floori(absf(entry.value1)) % HOLD_PARITY else StopOrder.GO)
        stop_orders.append(StopOrder.GUARD_SIGNAL)
        timetable.draw_up_close()
        _stops_done[entry.event] = true
        return StopResult.READY
    # waiting for the departure time
    waiting_for_departure = true
    return StopResult.USE


## A memory's command, as far as sending it again goes
static func _signature(entry:Entry) -> String:
    return "%s %s %s" % [entry.command, entry.value1, entry.value2]


## IsProperSemaphor() (Driver.cpp:373-388)
func _is_proper_semaphore(kind:Kind, obey_train:bool) -> bool:
    if obey_train:
        return kind == Kind.SEMAPHORE
    return kind == Kind.SEMAPHORE or kind == Kind.SHUNT_SEMAPHORE or kind == Kind.OUTSIDE_STATION


## determine_braking_distance(), determine_proximity_ranges() (Driver.cpp:6597-6812), with the
## margins of an EIM controller, of the weather and of a late train
func _determine_distances(
    vehicle:RID, order:int, speed:float, trainset:MaszynaDriverTrainset, shunt_velocity:float,
    velocity_desired:float, coupling:bool, cargo:bool, acceleration_threshold:float, late:bool
) -> void:
    var velocity_ceiling:float = maxf(BRAKING_SPEED_FLOOR, ceilf(absf(speed)))
    brake_distance = DRIVER_BRAKING * velocity_ceiling * (BRAKING_SPEED_OFFSET + velocity_ceiling)
    if trainset.mass > HEAVY_MASS:
        brake_distance *= HEAVY_FACTOR
    if -acceleration_threshold > THRESHOLD_BRAKING:
        brake_distance = velocity_ceiling * velocity_ceiling / DECELERATION_FACTOR / -acceleration_threshold
    # the G setting brakes later: its reaction on top
    var brake:MaszynaMoverRailVehicleBrake = RailVehicleServer.vehicle_component_get(
            vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as MaszynaMoverRailVehicleBrake
    if brake and brake.get_delay_setting() == MaszynaDriverBraking.DELAY_SETTING_G:
        brake_distance += G_REACTION_FACTOR * velocity_ceiling
    var vehicles:float = trainset.vehicles.size()
    # Global.FrictionWeatherFactor < 1: rain or snow, a trainset of more than one vehicle keeps further
    var weather_margin:float = WEATHER_PROXIMITY \
            if SimulationServer.simulation_get_weather_friction_factor() < 1.0 and vehicles > 1 else 0.0
    match order:
        MaszynaAIDriver.Order.CONNECT:
            if coupling:
                # stood close without a collision: right up to it
                _set_ranges(COUPLING_MIN_PROXIMITY, COUPLING_MAX_PROXIMITY, CLOSE_VELOCITY_PLUS, CLOSE_VELOCITY_MINUS)
            else:
                _set_ranges(CONNECT_MIN_PROXIMITY, CONNECT_MAX_PROXIMITY, CONNECT_VELOCITY_PLUS, CONNECT_VELOCITY_MINUS)
        MaszynaAIDriver.Order.DISCONNECT:
            _set_ranges(DISCONNECT_MIN_PROXIMITY, DISCONNECT_MAX_PROXIMITY, CLOSE_VELOCITY_PLUS, CLOSE_VELOCITY_MINUS)
        MaszynaAIDriver.Order.SHUNT:
            # modern vehicles might brake slower at low speeds (EIMCtrlType > 0, Driver.cpp:6717-6721)
            var configuration:MaszynaMoverEngineConfiguration = MaszynaDriverTraction.engine_configuration(VehicleServer.vehicle_component_get(
                    RailVehicleServer.vehicle_find_powered(vehicle), VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine)
            var modern_margin:float = MODERN_PROXIMITY \
                    if configuration and not configuration.cntrl_eim_control_type == MaszynaMoverEngineConfiguration.EIM_CONTROL_TYPE_0 else 0.0
            _set_ranges(minf(SHUNT_MIN_BASE + vehicles, SHUNT_MIN_MAX) + modern_margin + weather_margin,
                    minf(SHUNT_MAX_BASE + vehicles, SHUNT_MAX_MAX) + modern_margin + weather_margin,
                    SHUNT_VELOCITY_PLUS, minf(SHUNT_VELOCITY_MINUS_SHARE * shunt_velocity, SHUNT_VELOCITY_MINUS_MAX))
        MaszynaAIDriver.Order.LOOSE_SHUNT:
            _set_ranges(COUPLING_MIN_PROXIMITY, COUPLING_MAX_PROXIMITY, SHUNT_VELOCITY_PLUS, CLOSE_VELOCITY_MINUS)
        MaszynaAIDriver.Order.OBEY_TRAIN:
            var extra:float = (CARGO_PROXIMITY if cargo else 0.0) + weather_margin
            # a late train presses on (moveLate, Driver.cpp:6772-6777)
            _set_ranges(clampf(TRAIN_MIN_BASE + vehicles, TRAIN_MIN_LOW, TRAIN_MIN_HIGH) + extra,
                    clampf(TRAIN_MAX_BASE + vehicles, TRAIN_MAX_LOW, TRAIN_MAX_HIGH) + extra,
                    LATE_VELOCITY_PLUS if late else clampf(ceilf(TRAIN_VELOCITY_SHARE * velocity_desired), TRAIN_VELOCITY_PLUS_LOW, TRAIN_VELOCITY_HIGH),
                    LATE_VELOCITY_MINUS if late else clampf(roundf(TRAIN_VELOCITY_SHARE * velocity_desired), TRAIN_VELOCITY_MINUS_LOW, TRAIN_VELOCITY_HIGH))
            if absf(speed) < STANDING_SPEED:
                # stood too far: it does not draw up the last metres
                max_proximity = STANDING_MAX_PROXIMITY
        MaszynaAIDriver.Order.BANK:
            # the original leaves them as they were (Driver.cpp:6803-6806)
            pass
        _:
            _set_ranges(OTHER_MIN_PROXIMITY, OTHER_MAX_PROXIMITY, OTHER_VELOCITY_PLUS, OTHER_VELOCITY_MINUS)


func _set_ranges(minimum:float, maximum:float, plus:float, minus:float) -> void:
    min_proximity = minimum
    max_proximity = maximum
    velocity_plus = plus
    velocity_minus = minus

