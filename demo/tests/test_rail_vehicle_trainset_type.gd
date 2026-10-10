extends MaszynaGutTest

## What a trainset carries (RailVehicleServer.trainset_determine_type()): its cars - the vehicles
## without power - by their brake delays, as AutoRewident() counts them (Driver.cpp:2154-2165): a
## car with R, or without G, carries passengers, one with G and without R goods. The engine's own
## brake setting decides nothing.

const ENGINE_PATH:String = "res://tests/fixtures/test_vehicle.fiz"
const CAR_PATH:String = "res://tests/fixtures/test_wagon.fiz"
const COUPLING:int = RailVehicleController.COUPLING_FLAG_COUPLER

var engine:RailVehicleController


func before_each() -> void:
    engine = build_vehicle("TestTypeEngine", FizVehicleBuilder.build_description_at(ENGINE_PATH)) as RailVehicleController
    await wait_idle_frames(2)


## A car behind `previous`, its brake able to take `delays` (RailVehicleBrake.BrakeDelaySetting)
func _add_car(train_id:String, previous:RailVehicleController, delays:int) -> RailVehicleController:
    var car:RailVehicleController = build_vehicle(train_id, FizVehicleBuilder.build_description_at(CAR_PATH)) \
            as RailVehicleController
    await wait_idle_frames(2)
    (car.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake).cntrl_brake_delays = delays
    previous.couple(car, RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLER_END_FRONT, COUPLING)
    return car


func test_not_determined_is_none() -> void:
    await _add_car("TestTypeCar", engine, RailVehicleBrake.BRAKE_DELAY_PR)
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_NONE)


func test_lone_engine_is_none() -> void:
    RailVehicleServer.trainset_determine_type(engine.get_rid())
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_NONE)


func test_passenger_cars_behind_an_engine_at_g_are_passenger() -> void:
    var brake:RailVehicleBrake = engine.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    brake.cntrl_brake_delays = RailVehicleBrake.BRAKE_DELAY_G
    var car:RailVehicleController = await _add_car("TestTypeCar", engine, RailVehicleBrake.BRAKE_DELAY_PR)
    await _add_car("TestTypeCar2", car, RailVehicleBrake.BRAKE_DELAY_P)
    RailVehicleServer.trainset_determine_type(engine.get_rid())
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_PASSENGER)
    assert_eq(RailVehicleServer.trainset_get_type(car.get_rid()), RailVehicleServer.TRAINSET_TYPE_PASSENGER,
            "every vehicle of the trainset has its type")


func test_goods_cars_are_cargo() -> void:
    await _add_car("TestTypeCar", engine, RailVehicleBrake.BRAKE_DELAY_GP)
    RailVehicleServer.trainset_determine_type(engine.get_rid())
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_CARGO)


func test_passenger_and_goods_cars_are_mixed() -> void:
    var car:RailVehicleController = await _add_car("TestTypeCar", engine, RailVehicleBrake.BRAKE_DELAY_GP)
    await _add_car("TestTypeCar2", car, RailVehicleBrake.BRAKE_DELAY_GPR)
    RailVehicleServer.trainset_determine_type(engine.get_rid())
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_MIXED)


func test_type_stays_until_determined_again() -> void:
    await _add_car("TestTypeCar", engine, RailVehicleBrake.BRAKE_DELAY_GP)
    RailVehicleServer.trainset_determine_type(engine.get_rid())
    engine.uncouple(RailVehicleController.COUPLER_END_REAR)
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_CARGO,
            "an uncoupling alone does not determine it")
    RailVehicleServer.trainset_determine_type(engine.get_rid())
    assert_eq(RailVehicleServer.trainset_get_type(engine.get_rid()), RailVehicleServer.TRAINSET_TYPE_NONE)
