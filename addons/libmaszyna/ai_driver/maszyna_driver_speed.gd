@tool
extends RefCounted
class_name MaszynaDriverSpeed

## The speed and the acceleration the original's driver wants on every update
## (TController::pick_optimal_speed(), Driver.cpp:7297-7400): the trainset's limit, the timetable's,
## the shunting speed, the speed allowed by the orders, the track's limit; then the acceleration
## towards it (adjust_desired_speed_for_target_speed(), adjust_desired_speed_for_current_speed()).
##
## The tracks and the vehicles ahead come from the speed table (MaszynaDriverRoute), the
## braking characteristic from MaszynaDriverBraking; it stands while its train is dispatched
## at a stop (StationServer, the original's fStopTime).

## AccPreferred of a calm driver [m/s2] (EasyAcceleration, Driver.cpp:152)
const EASY_ACCELERATION:float = 0.85
## Below this [m/s2] the driver does not ask for power (EU07_AI_NOACCELERATION, Driver.h:23)
const NO_ACCELERATION:float = -0.05
## Standing, it holds itself back this much (Driver.cpp:7425)
const STANDING_ACCELERATION:float = -0.01
## Too fast where it should stand: brake firmly (Driver.cpp:7730)
const STOP_ACCELERATION:float = -0.85
## The acceleration asked for is smoothed, braking is not (Driver.cpp:7383-7387)
const SMOOTHING_NEW:float = 0.2
const SMOOTHING_KEPT:float = 0.8
## A train's acceleration asked for stays within this [m/s2] (Driver.cpp:7395-7398)
const MAX_ACCELERATION:float = 0.9
## -1 is no limit (min_speed(), utilities.h:284)
const NO_LIMIT:float = -1.0
## adjust_desired_speed_for_target_speed() (Driver.cpp:7440-7500): the speed that brakes a train to
## a stop over a distance, and the margins of stopping short of it [m]
const DECELERATION_FACTOR:float = 25.92
const STOP_MARGIN:float = 0.1
const STOP_MARGIN_SHARE:float = 0.5
## Coasting towards a far stop at up to these [km/h], further than COAST_DISTANCE [m] the higher
const COAST_SPEED_FAR:float = 10.0
const COAST_SPEED_NEAR:float = 5.0
const COAST_DISTANCE:float = 10.0
## Slowing down starts no nearer than this before a stop [m], coupling up closer
const SLOWDOWN_DISTANCE:float = 100.0
const SLOWDOWN_CONNECT:float = 25.0
## Shunting or coupling up this close to a vehicle ahead [m], a train coasts up to it at up to
## COAST_SPEED_NEAR (Driver.cpp:7621-7625)
const OBSTACLE_COAST_DISTANCE:float = 50.0
## adjust_desired_speed_for_obstacles() (Driver.cpp:7405-7527): a vehicle ahead is minded unless it
## runs this [km/h] faster; closer than OBSTACLE_NEAR [m] the speed follows its, at least
## OBSTACLE_NEAR_SPEED, braking with OBSTACLE_NEAR_ACCELERATION [m/s2] when faster
const OBSTACLE_FASTER_SPEED:float = 5.0
const OBSTACLE_NEAR:float = 250.0
const OBSTACLE_NEAR_SPEED:float = 20.0
const OBSTACLE_NEAR_ACCELERATION:float = -0.30
## The safe distance takes this much of the braking distance
const OBSTACLE_BRAKE_SHARE:float = 1.15
## A vehicle ahead slower than this [km/h] is driven up to at no more than OBSTACLE_FAR_MARGIN over
## its speed further than OBSTACLE_FAR [m], OBSTACLE_CLOSE_MARGIN at most OBSTACLE_CLOSE_SPEED nearer;
## coupling up to it, at up to OBSTACLE_CONNECT_MARGIN over, at most OBSTACLE_CLOSE_SPEED, without
## braking harder than OBSTACLE_CONNECT_ACCELERATION [m/s2]
const OBSTACLE_SLOW_SPEED:float = 10.0
const OBSTACLE_FAR:float = 100.0
const OBSTACLE_FAR_MARGIN:float = 20.0
const OBSTACLE_CLOSE_MARGIN:float = 4.0
const OBSTACLE_CLOSE_SPEED:float = 8.0
const OBSTACLE_CONNECT_MARGIN:float = 2.0
const OBSTACLE_CONNECT_ACCELERATION:float = 0.35
## A train stops this far short of a vehicle ahead [m] (fDriverDist, Driver.cpp:1884)
const DRIVER_DISTANCE:float = 50.0
## Both moving, it slows down within twice the distance kept plus OBSTACLE_SPEED_DISTANCE [m] per
## km/h, braking with OBSTACLE_BRAKE_ACCELERATION to OBSTACLE_SPEED_MARGIN under the other's speed
const OBSTACLE_SPEED_DISTANCE:float = 2.0
const OBSTACLE_BRAKE_ACCELERATION:float = -0.9
const OBSTACLE_SPEED_MARGIN:float = 5.0
## Coupling up, near a vehicle ahead it runs at most OBSTACLE_CONNECT_FAR_SPEED further than
## OBSTACLE_FAR, OBSTACLE_CONNECT_NEAR_SPEED nearer [km/h]
const OBSTACLE_CONNECT_FAR_SPEED:float = 20.0
const OBSTACLE_CONNECT_NEAR_SPEED:float = 4.0
## Downhill past this [m/s2] the brakes' reaction is reckoned with: the speed gained over this many
## seconds (Driver.cpp:7747-7750); a goods train's a0 over CARGO_LATE_A0 brakes up to CARGO_LATE_EXTRA
## more (Driver.cpp:7762-7765); over the speed allowed OVERSPEED_BRAKING per km/h, up to
## OVERSPEED_LIMIT km/h of it (Driver.cpp:7779)
const DOWNHILL_GRAVITY:float = 0.025
const DOWNHILL_REACTION:float = 30.0
const CARGO_LATE_A0:float = 0.2
const CARGO_LATE_EXTRA:float = 0.15
const OVERSPEED_BRAKING:float = 0.15
const OVERSPEED_LIMIT:float = 5.0
## Half the coupler's strength over the train's mass, of a train with wagons the more the faster up
## to 40 km/h, never under a heavy goods train's HeavyCargoTrainAcceleration (Driver.cpp:157,
## 7784-7788)
const COUPLER_SHARE:float = 0.5
const COUPLER_SPEED_SHARE:float = 0.025
const COUPLER_SPEED_MIN:float = 0.2
const HEAVY_CARGO_ACCELERATION:float = 0.10
## The final check (Driver.cpp:7829-7836): a0 of this share unless ready and the next speed not
## within NEAR_NEXT_VELOCITY [km/h]; the braking distance's multiplier taken this many times
const NOT_READY_BRAKE_SHARE:float = 0.8
const NEAR_NEXT_VELOCITY:float = 40.0
const DISTANCE_MULTIPLIER_SHARE:float = 1.2
## Close to a vehicle ahead or a stop, the driver reacts this often [s] (Driver.cpp:7501, 7684)
const HURRIED_REACTION_TIME:float = 0.1

## VelDesired [km/h]
var velocity_desired:float = 0.0
## Why the driver wants to stand (velocity_desired 0), for whoever shows it: the vehicle is not
## ready (iEngineActive), it has no order to drive, a signal ahead holds it (moveStopHere), the
## tracks and vehicles ahead do (a stop point, the end of the line, a vehicle, a speed of 0), or the
## train is dispatched at a stop (fStopTime < 0)
var stop_reason:DriverState.StopReason = DriverState.STOP_REASON_NONE
## AccDesired [m/s2]
var acceleration_desired:float = 0.0
## AccPreferred [m/s2]: a calm driver's, lower near a vehicle ahead
var acceleration_preferred:float = EASY_ACCELERATION
## VelNext [km/h] and ActualProximityDist [m]: the speed table's, the vehicle ahead taken in
var velocity_next:float = NO_LIMIT
var proximity_distance:float = 0.0
## How soon the driver looks again [s] (ReactionTime): its own, sooner close to a stop or a vehicle
var reaction_time:float = 0.0
## fAccDesiredAv
var _acceleration_average:float = 0.0


## `order` the current one; `dispatch_step` its train's dispatch at a stop; `velocity` the speed
## allowed (VelSignal), `timetable_velocity` the timetable's limit (TTVmax), `speed` the vehicle's
## along the way it drives (DirectionalVel()) [km/h]; `route` what it read of the tracks and the
## vehicles ahead; `reaction` the driver's own reaction time [s]
func pick(
    order:int, active:bool, stop_here:bool, dispatch_step:StationServer.DispatchStep, velocity:float,
    shunt_velocity:float, timetable_velocity:float, speed:float, trainset:MaszynaDriverTrainset,
    route:MaszynaDriverRoute, reaction:float, braking:MaszynaDriverBraking,
    trainset_type:RailVehicleServer.TrainsetType
) -> void:
    reaction_time = reaction
    velocity_desired = trainset.velocity_max
    stop_reason = DriverState.STOP_REASON_NONE
    acceleration_preferred = EASY_ACCELERATION
    acceleration_desired = acceleration_preferred
    velocity_next = route.velocity_next
    proximity_distance = route.proximity_distance
    if order & MaszynaAIDriver.Order.OBEY_TRAIN and timetable_velocity > 0.0:
        velocity_desired = min_speed(velocity_desired, timetable_velocity)
    # shunting is calm
    if not order & (MaszynaAIDriver.Order.OBEY_TRAIN | MaszynaAIDriver.Order.BANK):
        velocity_desired = min_speed(velocity_desired, shunt_velocity)
    if not active:
        velocity_desired = 0.0
        stop_reason = DriverState.STOP_REASON_NOT_READY
        velocity_next = 0.0
        acceleration_desired = minf(acceleration_desired, NO_ACCELERATION)
        return
    # told to put the vehicle away or to turn: stop first
    if order & (MaszynaAIDriver.Order.RELEASE_ENGINE | MaszynaAIDriver.Order.CHANGE_DIRECTION):
        velocity = 0.0
    # the speed table's limit - the tracks the trainset is on, a stop close ahead (TableUpdate() fVelDes)
    velocity_desired = min_speed(velocity_desired, route.velocity_limit)
    if route.obstacle:
        _adjust_for_obstacle(order, speed, route)
    # adjust_desired_speed_for_limits() (Driver.cpp:7516-7576)
    if velocity >= 0.0:
        velocity_desired = min_speed(velocity_desired, velocity)
    var driving:int = (MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.LOOSE_SHUNT
            | MaszynaAIDriver.Order.OBEY_TRAIN | MaszynaAIDriver.Order.BANK)
    # while its train is dispatched it does not drive (Driver.cpp:7454-7457)
    if not dispatch_step == StationServer.DISPATCH_STEP_NONE:
        velocity_desired = 0.0
        stop_reason = DriverState.STOP_REASON_DISPATCH
    elif (order & driving and stop_here and absf(speed) < MaszynaDriverTrainset.NO_MOVEMENT_SPEED
            and route.signal_velocity_next == 0.0) or order == MaszynaAIDriver.Order.WAIT_FOR_ORDERS:
        velocity_desired = 0.0
        stop_reason = DriverState.STOP_REASON_WAITING_FOR_ORDERS if order == MaszynaAIDriver.Order.WAIT_FOR_ORDERS \
                else DriverState.STOP_REASON_SIGNAL
    _adjust_for_target_speed(order, speed, route, braking, trainset, trainset_type)
    # adjust_desired_speed_for_current_speed() (Driver.cpp:7722-7760)
    if speed > velocity_desired:
        if velocity_desired == 0.0:
            acceleration_desired = minf(acceleration_desired, STOP_ACCELERATION)
        elif speed > velocity_desired + route.velocity_plus:
            acceleration_desired = minf(acceleration_desired, NO_ACCELERATION + braking.acceleration_threshold)
        else:
            acceleration_desired = minf(acceleration_desired, maxf(0.0, acceleration_preferred))
    _adjust_for_slope_and_couplers(speed, trainset, route, braking, trainset_type)
    # pick_optimal_speed() (Driver.cpp:7383-7398)
    if acceleration_desired > NO_ACCELERATION:
        _acceleration_average = SMOOTHING_NEW * acceleration_desired + SMOOTHING_KEPT * _acceleration_average
        acceleration_desired = _acceleration_average
    if velocity_desired == 0.0:
        acceleration_desired = minf(acceleration_desired, NO_ACCELERATION)
    acceleration_desired = clampf(minf(acceleration_desired, acceleration_preferred), -MAX_ACCELERATION, MAX_ACCELERATION)
    if velocity_desired == 0.0 and stop_reason == DriverState.STOP_REASON_NONE:
        stop_reason = DriverState.STOP_REASON_AHEAD


## adjust_desired_speed_for_obstacles() (Driver.cpp:7405-7527): a vehicle ahead not running away
## brings the next stop up to it and caps the speed; too close, the driver brakes and looks again
## sooner. Rail vehicles only - the original's road vehicles keep other distances.
func _adjust_for_obstacle(order:int, speed:float, route:MaszynaDriverRoute) -> void:
    var other_speed:float = route.obstacle_speed
    var obstacle_distance:float = route.obstacle.distance
    if other_speed - speed >= OBSTACLE_FASTER_SPEED:
        return
    proximity_distance = minf(proximity_distance, obstacle_distance)
    if obstacle_distance <= OBSTACLE_NEAR:
        # whatever it does, close up match its speed, or slow down behind one standing
        velocity_desired = min_speed(velocity_desired, maxf(other_speed, OBSTACLE_NEAR_SPEED))
        if speed > velocity_desired + route.velocity_plus:
            acceleration_preferred = minf(OBSTACLE_NEAR_ACCELERATION, acceleration_preferred)
    var coupling:int = MaszynaAIDriver.Order.CONNECT | MaszynaAIDriver.Order.LOOSE_SHUNT
    if obstacle_distance - route.max_proximity - route.brake_distance * OBSTACLE_BRAKE_SHARE >= 0.0:
        if order & MaszynaAIDriver.Order.CONNECT:
            velocity_desired = min_speed(velocity_desired,
                    OBSTACLE_CONNECT_FAR_SPEED if obstacle_distance > OBSTACLE_FAR else OBSTACLE_CONNECT_NEAR_SPEED)
        return
    # nearer than it could stop at a safe distance
    if other_speed < OBSTACLE_SLOW_SPEED:
        velocity_desired = floorf(min_speed(velocity_desired,
                other_speed + OBSTACLE_FAR_MARGIN if obstacle_distance > OBSTACLE_FAR
                else minf(OBSTACLE_CLOSE_SPEED, other_speed + OBSTACLE_CLOSE_MARGIN)))
        if order & coupling:
            # coupling up: drive on to it, without braking
            acceleration_preferred = minf(OBSTACLE_CONNECT_ACCELERATION, acceleration_preferred)
            velocity_next = floorf(minf(OBSTACLE_CLOSE_SPEED, other_speed + OBSTACLE_CONNECT_MARGIN))
        else:
            velocity_next = 0.0
            if obstacle_distance <= route.min_proximity:
                velocity_desired = 0.0
            # a train stops at a safe distance
            if order & MaszynaAIDriver.Order.OBEY_TRAIN:
                proximity_distance -= DRIVER_DISTANCE
    elif obstacle_distance < 2.0 * route.max_proximity + OBSTACLE_SPEED_DISTANCE * speed:
        # both moving, and it is within the braking distance: slow down below its speed
        acceleration_preferred = minf(OBSTACLE_BRAKE_ACCELERATION, acceleration_preferred)
        velocity_next = min_speed(roundf(other_speed) - OBSTACLE_SPEED_MARGIN, velocity_desired)
        if obstacle_distance <= 2.0 * route.max_proximity:
            velocity_desired = velocity_next
    reaction_time = HURRIED_REACTION_TIME if speed > MaszynaDriverTrainset.NO_MOVEMENT_SPEED \
            else MaszynaAIDriver.PREPARE_TIME


## adjust_desired_speed_for_target_speed() (Driver.cpp:7578-7720): the acceleration towards the
## next speed, braking to arrive at it within the distances kept to a stop
func _adjust_for_target_speed(
    order:int, speed:float, route:MaszynaDriverRoute, braking:MaszynaDriverBraking,
    trainset:MaszynaDriverTrainset, trainset_type:RailVehicleServer.TrainsetType
) -> void:
    acceleration_desired = acceleration_preferred if not velocity_desired == 0.0 else STANDING_ACCELERATION
    var next:float = velocity_next
    var distance:float = proximity_distance
    if next < 0.0 or distance > route.reach or speed < next:
        return
    if speed <= MaszynaDriverTrainset.NO_MOVEMENT_SPEED:
        # standing: go if it may, or creep up to the stop while far from it and nothing is in the way
        if next > 0.0:
            acceleration_desired = acceleration_preferred
        elif distance <= route.max_proximity or acceleration_preferred <= 0.0:
            velocity_desired = 0.0
        return
    if distance > route.min_proximity:
        if distance < route.max_proximity:
            if next == 0.0:
                var shunting:int = MaszynaAIDriver.Order.SHUNT | MaszynaAIDriver.Order.CONNECT
                if order & shunting and route.obstacle and route.obstacle.distance < OBSTACLE_COAST_DISTANCE:
                    # coast up to a vehicle ahead, to close up on a siding
                    velocity_desired = min_speed(COAST_SPEED_NEAR, velocity_desired)
                else:
                    # brake to stop there
                    velocity_desired = next
                    acceleration_desired = minf(
                            (next * next - speed * speed)
                            / (DECELERATION_FACTOR * (distance + STOP_MARGIN - STOP_MARGIN_SHARE * route.min_proximity)),
                            braking.acceleration_threshold)
        else:
            acceleration_desired = acceleration_preferred
            if speed > min_speed(COAST_SPEED_FAR if distance > COAST_DISTANCE else COAST_SPEED_NEAR, velocity_desired):
                # don't slow down while there is room to stop at a safe distance - a goods train and a
                # slow target need longer (braking_distance_multiplier(), Driver.cpp:7644-7654)
                var multiplier:float = braking.distance_multiplier(next, speed, trainset, trainset_type)
                var braking_distance:float = route.brake_distance * multiplier
                var slowdown:float = maxf(
                        SLOWDOWN_CONNECT if order & MaszynaAIDriver.Order.CONNECT else SLOWDOWN_DISTANCE,
                        braking_distance)
                if braking_distance + maxf(slowdown, route.max_proximity) >= distance - route.max_proximity:
                    var braking_point:float = next * multiplier
                    acceleration_desired = minf(acceleration_desired,
                            (next * next - speed * speed)
                            / (DECELERATION_FACTOR * maxf(distance - braking_point, minf(distance, braking_point)) + STOP_MARGIN))
        acceleration_desired = minf(acceleration_desired, acceleration_preferred)
    else:
        # closer than it keeps: stop, or go on over a small excess
        if next == 0.0:
            velocity_desired = next
        elif speed <= next + route.velocity_plus:
            acceleration_desired = maxf(0.0, acceleration_preferred)
        reaction_time = HURRIED_REACTION_TIME


## The rest of adjust_desired_speed_for_current_speed() (Driver.cpp:7747-7843): downhill braking
## starts before the brakes would catch up; a train with wagons accelerates no harder than half its
## coupler's strength allows, less at low speed; and it does not ask for power it would have to
## brake off again at once
func _adjust_for_slope_and_couplers(
    speed:float, trainset:MaszynaDriverTrainset, route:MaszynaDriverRoute,
    braking:MaszynaDriverBraking, trainset_type:RailVehicleServer.TrainsetType
) -> void:
    var gravity:float = trainset.gravity_acceleration
    if gravity > DOWNHILL_GRAVITY:
        # the speed the train gains before the brakes work
        var estimate:float = speed + (1.0 - braking.table_a0) * DOWNHILL_REACTION * trainset.acceleration
        if estimate > velocity_desired:
            if velocity_desired == 0.0:
                acceleration_desired = minf(acceleration_desired, STOP_ACCELERATION)
            else:
                acceleration_desired = minf(acceleration_desired, NO_ACCELERATION + braking.acceleration_threshold)
                # a goods train braking late crosses its threshold
                if trainset_type == RailVehicleServer.TRAINSET_TYPE_CARGO and braking.table_a0 > CARGO_LATE_A0:
                    acceleration_desired -= clampf(braking.table_a0 - CARGO_LATE_A0, 0.0, CARGO_LATE_EXTRA)
        elif route.velocity_minus > 0.0:
            # ease off closing in on the speed wanted
            acceleration_desired = minf(acceleration_desired, lerpf(NO_ACCELERATION, acceleration_preferred,
                    clampf(velocity_desired - estimate, 0.0, route.velocity_minus) / route.velocity_minus))
        if speed > MaszynaDriverTrainset.NO_MOVEMENT_SPEED:
            acceleration_desired -= gravity
            # over the speed allowed something went wrong: brake harder
            acceleration_desired -= OVERSPEED_BRAKING * clampf(speed - velocity_desired, 0.0, OVERSPEED_LIMIT)
    # to spare the couplers sudden jolts (Driver.cpp:7784-7789)
    if trainset.mass > 0.0:
        var acceleration_max:float = COUPLER_SHARE * trainset.coupler_strength / trainset.mass
        if trainset.vehicles.size() - trainset.controlled_engines > 0:
            acceleration_max *= clampf(speed * COUPLER_SPEED_SHARE, COUPLER_SPEED_MIN, 1.0)
        acceleration_desired = minf(acceleration_desired, clampf(acceleration_max, HEAVY_CARGO_ACCELERATION, acceleration_preferred))
    # not above the speed wanted: power it would brake off at once is not asked for (Driver.cpp:7826-7838)
    if speed < velocity_desired:
        var limit:float = braking.table_a0 * NOT_READY_BRAKE_SHARE \
                if not trainset.ready or velocity_next > speed - NEAR_NEXT_VELOCITY else -braking.acceleration_threshold
        if -acceleration_desired * braking.factor(self, route, trainset, speed) \
                < limit / (DISTANCE_MULTIPLIER_SHARE * braking.distance_multiplier(velocity_next, speed, trainset, trainset_type)):
            acceleration_desired = maxf(NO_ACCELERATION, acceleration_desired)


## min_speed() (utilities.h:284): the lower of two speeds, where NO_LIMIT is none
static func min_speed(left:float, right:float) -> float:
    if left == NO_LIMIT:
        return right
    if right == NO_LIMIT:
        return left
    return minf(left, right)
