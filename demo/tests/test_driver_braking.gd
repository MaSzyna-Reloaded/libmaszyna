extends MaszynaGutTest

## MaszynaDriverBraking: the original driver's brakes, through the cab
## (control_braking_force(), Driver.cpp:8065-8190).

const Order = MaszynaAIDriver.Order
const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const REACTION:float = MaszynaAIDriver.EASY_REACTION_TIME
## No dispatch at a stop (StationServer)
const NO_DISPATCH:StationServer.DispatchStep = StationServer.DISPATCH_STEP_NONE
## A trainset whose type was never determined
const NO_TRAINSET:RailVehicleServer.TrainsetType = RailVehicleServer.TRAINSET_TYPE_NONE
const STEP:float = 0.5

var train:VehicleController
var vehicle:RID
var braking:MaszynaDriverBraking
var speed:MaszynaDriverSpeed
var trainset:MaszynaDriverTrainset
var route:MaszynaDriverRoute
var state:MaszynaAIDriver.AIDriverState
var driver:RID


func before_each():
    train = build_vehicle("DriverBrakingTest", SM42, 0.0, MaszynaVehicleData.DriverType.DRIVER_HEAD)
    vehicle = train.get_rid()
    # a cab with no controls of its own: the knobs are there unmodelled
    var controls:MaszynaCabinControls = MaszynaCabinControls.new()
    CabinSystem.vehicle_attach_cab_logic(
            vehicle, MaszynaCabinLogic.new(func(_cabin:RID) -> MaszynaCabinControls: return controls))
    braking = MaszynaDriverBraking.new()
    speed = MaszynaDriverSpeed.new()
    trainset = MaszynaDriverTrainset.new()
    trainset.update(vehicle, 1, false)
    route = MaszynaDriverRoute.new()
    state = MaszynaAIDriver.AIDriverState.new()
    # the computer drives (AIControllFlag): a driver whose cues are taken
    driver = get_vehicle_driver(vehicle)
    attach_driver_implementation(driver, DriverImplementation.new())


func after_each():
    attach_driver_implementation(driver, null)
    CabinSystem.vehicle_attach_cab_logic(vehicle, null)


## One update of the driver: its decision, then the handles set (control_braking_force(),
## SetTimeControllers())
func _decide() -> void:
    var situation:MaszynaDriverTraction.Situation = MaszynaDriverTraction.Situation.new()
    situation.state = state
    situation.traction = state.traction
    situation.vehicle = vehicle
    situation.cabin = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    situation.controlling = vehicle
    situation.order = Order.SHUNT
    situation.speed = speed
    situation.trainset = trainset
    situation.route = route
    situation.braking = braking
    braking.control(situation, STEP)
    braking.set_time_controllers(situation)


func test_standing_it_holds_the_locomotive_with_its_own_brake():
    speed.pick(Order.SHUNT, false, false, NO_DISPATCH, 0.0, 0.0, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)

    # the fixture's handle starts at lap: the train brake to running first, then the local brake
    _decide()
    _decide()

    assert_eq(float(train.get_state()["brake_local_position_normalized"]), MaszynaDriverBraking.LOCAL_BRAKE_APPLIED)


func test_wanting_to_go_it_releases():
    speed.pick(Order.SHUNT, false, false, NO_DISPATCH, 0.0, 0.0, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)
    _decide()
    _decide()
    assert_eq(float(train.get_state()["brake_local_position_normalized"]), MaszynaDriverBraking.LOCAL_BRAKE_APPLIED)

    speed.pick(Order.SHUNT, true, false, NO_DISPATCH, 20.0, 20.0, -1.0, 0.0, trainset, route, REACTION, braking, NO_TRAINSET)
    _decide()

    assert_eq(float(train.get_state()["brake_local_position_normalized"]), MaszynaDriverBraking.LOCAL_BRAKE_RELEASED)
    assert_eq(braking.position, MaszynaDriverBraking.POSITION_RUNNING, "the train brake at running")
