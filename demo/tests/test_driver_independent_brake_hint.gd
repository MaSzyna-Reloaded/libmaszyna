extends MaszynaGutTest

## A standing driver secures its vehicle with the independent brake (apply_independent_brake_only(),
## Driver.cpp:8178-8189) - on a vehicle that has one. An EN57's motor car has none (no LocalBrake=),
## its cab cars only the hand wheel (LocalBrake=ManualBrake): the player is not hinted to apply a brake
## that is not there (maszyna-reloaded#4, reports#16).

const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6bs.fiz"
var _vehicle:RID


func before_each() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH)
    _vehicle = build_vehicle("IndependentBrakeHintTest", description, 0.0, MaszynaVehicleData.DriverType.DRIVER_HEAD).get_rid()


func after_each() -> void:
    PlayerServer.player_leave_vehicle()


func test_a_vehicle_without_an_independent_brake_is_not_hinted_to_apply_it() -> void:
    var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
            _vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    assert_eq(brake.cntrl_local_brake_type, RailVehicleBrake.LOCAL_BRAKE_TYPE_NONE, "the motor car has no local brake")
    var driver:RID = get_vehicle_driver(_vehicle)
    attach_driver_implementation(driver, MaszynaAIDriver.new())
    PlayerServer.player_take_over_vehicle(_vehicle)
    # the driver checks its trainset on its first update, at once on attaching (MaszynaAIDriver)
    if not await wait_simulated_until(
            func() -> bool: return get_driver_internal_state(driver).get("trainset_vehicles", []).size() > 0,
            TICK, "the driver's trainset checked"):
        return
    # a standing driver's next updates, its reaction time away at most
    await step(ticks(MaszynaAIDriver.PREPARE_TIME * 2.0))
    var hints:Array[MaszynaDriverHints.Hint] = []
    for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
        hints.append(entry["hint"])
    assert_does_not_have(hints, MaszynaDriverHints.Hint.INDEPENDENT_BRAKE_APPLY,
            "no hint to apply an independent brake the vehicle does not have")
    attach_driver_implementation(driver, null)
