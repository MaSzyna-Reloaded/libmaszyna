extends MaszynaGutTest

## A cab's `brakes: i j` gauge reads pressure j of car i of the train, counted from the end the
## occupied cab faces over the control couplings (TTrain::fPress, Train.cpp:8670-8680, 12158-12166).

const CAB_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.fiz"
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6bs.fiz"
const PIPE:int = 1

var cab_car:RID
var motor_car:RID
var pressures:LegacyCabinTrainsetPressures


func before_each() -> void:
    var cab:VehicleController = build_vehicle("TestPressuresCabCar", FizVehicleBuilder.build_description_at(CAB_CAR_PATH),
            0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var motor:VehicleController = build_vehicle("TestPressuresMotorCar", FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH))
    await step(2)
    cab.couple(motor, RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLER_END_FRONT,
            RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_PERMANENT
            | RailVehicleController.COUPLING_FLAG_CONTROL)
    cab_car = cab.get_rid()
    motor_car = motor.get_rid()
    pressures = LegacyCabinTrainsetPressures.new()
    pressures.state_keys = [LegacyCabinTrainsetPressures.state_key(1, PIPE), LegacyCabinTrainsetPressures.state_key(2, PIPE),
            LegacyCabinTrainsetPressures.state_key(3, PIPE)]
    add_child_autofree(pressures)
    pressures.set_vehicle_rid(cab_car)
    await step(2)


func test_each_car_gauge_reads_its_car() -> void:
    assert_eq(CabinSystem.vehicle_state_value(cab_car, LegacyCabinTrainsetPressures.state_key(1, PIPE)),
            float(VehicleServer.vehicle_dump_state(cab_car).get("pipe_pressure")), "car 1 is the cab's own")
    assert_eq(CabinSystem.vehicle_state_value(cab_car, LegacyCabinTrainsetPressures.state_key(2, PIPE)),
            float(VehicleServer.vehicle_dump_state(motor_car).get("pipe_pressure")), "car 2 the next one")
    assert_eq(CabinSystem.vehicle_state_value(cab_car, LegacyCabinTrainsetPressures.state_key(3, PIPE)), 0.0,
            "no car 3")
