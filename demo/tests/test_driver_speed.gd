extends MaszynaGutTest

## MaszynaDriverSpeed: the speed and acceleration the original's driver wants
## (pick_optimal_speed(), Driver.cpp:7297-7400), without a speed table; a vehicle ahead set by hand.

const Order = MaszynaAIDriver.Order
const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const REACTION:float = MaszynaAIDriver.EASY_REACTION_TIME
## No dispatch at a stop, and the passengers getting off and on (StationServer)
const NO_DISPATCH:StationServer.DispatchStep = StationServer.DISPATCH_STEP_NONE
const EXCHANGING:StationServer.DispatchStep = StationServer.DISPATCH_STEP_EXCHANGE
## A trainset whose type was never determined
const NO_TRAINSET:RailVehicleServer.TrainsetType = RailVehicleServer.TRAINSET_TYPE_NONE
const SHUNT_VELOCITY:float = 25.0
const BRAKING_DISTANCE:float = 50.0

var speed:MaszynaDriverSpeed
var trainset:MaszynaDriverTrainset
var route:MaszynaDriverRoute
## shunting: no table, the threshold of shunting
var braking:MaszynaDriverBraking


func before_each():
    speed = MaszynaDriverSpeed.new()
    trainset = MaszynaDriverTrainset.new()
    # nothing read ahead: no next speed, no limit
    route = MaszynaDriverRoute.new()
    braking = MaszynaDriverBraking.new()
    var vehicles:Array[RID] = [build_vehicle("DriverSpeedTest", SM42).get_rid()]
    trainset.vehicles = vehicles


func test_shunting_drives_at_the_shunting_speed():
    speed.pick(Order.SHUNT, true, false, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, SHUNT_VELOCITY)
    assert_gt(speed.acceleration_desired, 0.0, "it asks for power")


func test_a_driver_not_ready_wants_nothing():
    speed.pick(Order.SHUNT, false, false, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, 0.0)
    assert_lte(speed.acceleration_desired, MaszynaDriverSpeed.NO_ACCELERATION)


## Driver.cpp:7454-7457: while its train is dispatched it does not drive
func test_it_waits_for_the_dispatch():
    speed.pick(Order.OBEY_TRAIN, true, false, EXCHANGING, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, 0.0)
    assert_eq(speed.stop_reason, DriverState.STOP_REASON_DISPATCH)


func test_told_to_stand_it_stays():
    speed.pick(Order.SHUNT, true, true, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, 0.0, "standing, told to stop here, no signal ahead")


func test_the_track_limit_holds_and_too_fast_brakes():
    # the track under the trainset allows 10, as the speed table read it
    route.velocity_limit = 10.0
    speed.pick(Order.SHUNT, true, false, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 20.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, 10.0, "the track allows 10")
    assert_lt(speed.acceleration_desired, MaszynaDriverSpeed.NO_ACCELERATION, "20 against 10: brake")


func test_a_vehicle_standing_close_ahead_stops_it():
    # at 15 km/h it needs about 50 m to stop (determine_braking_distance()); the vehicle stands
    # nearer than the least distance it keeps
    route.brake_distance = BRAKING_DISTANCE
    route.obstacle = _vehicle_ahead(route.min_proximity - 1.0)
    speed.pick(Order.SHUNT, true, false, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 15.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, 0.0, "within the distance it keeps")
    assert_lt(speed.acceleration_desired, MaszynaDriverSpeed.NO_ACCELERATION, "brake")
    assert_eq(speed.reaction_time, MaszynaDriverSpeed.HURRIED_REACTION_TIME, "and look again soon")


func test_a_vehicle_far_ahead_changes_nothing():
    route.obstacle = _vehicle_ahead(900.0)
    speed.pick(Order.SHUNT, true, false, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, SHUNT_VELOCITY)
    assert_gt(speed.acceleration_desired, 0.0, "it asks for power")
    assert_eq(speed.reaction_time, REACTION)


func test_a_vehicle_running_away_is_not_minded():
    route.obstacle = _vehicle_ahead(20.0)
    route.obstacle_speed = 60.0
    speed.pick(Order.SHUNT, true, false, NO_DISPATCH, SHUNT_VELOCITY, SHUNT_VELOCITY, -1.0, 15.0, trainset, route, REACTION, braking, NO_TRAINSET)

    assert_eq(speed.velocity_desired, SHUNT_VELOCITY)


func _vehicle_ahead(distance:float) -> RailVehicleNeighbour:
    var neighbour:RailVehicleNeighbour = RailVehicleNeighbour.new()
    neighbour.distance = distance
    return neighbour
