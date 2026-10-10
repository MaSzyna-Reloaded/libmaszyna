extends MaszynaGutTest

## The cab car of an EZT has no engine, yet its reverser steps back from "forward" through the
## high start (DirectionBackward, Mover.cpp:3250) - on the thresholds every EZT gets from Param:
## (LoadFIZ_Param, Mover.cpp:10300-10305). Without them the step back never leaves "forward".

const CAB_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.fiz"
const MOTOR_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6bs.fiz"
## Original engine: LoadFIZ_Param (Mover.cpp:10303-10304)
const EZT_IMIN_LOW:int = 1
const EZT_IMIN_HIGH:int = 2


func _direction_after(vehicle_rid:RID, command:String) -> int:
    VehicleServer.vehicle_send_command(vehicle_rid, command)
    await wait_idle_frames(2)
    return int(VehicleServer.vehicle_dump_state(vehicle_rid).get("direction", 0))


func test_the_cab_car_reverser_goes_back_to_reverse() -> void:
    var train:VehicleController = build_vehicle("TestEn57CabCar", FizVehicleBuilder.build_description_at(CAB_CAR_PATH),
            0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var vehicle_rid:RID = train.get_rid()
    await wait_idle_frames(2)
    VehicleServer.vehicle_send_command(vehicle_rid, "cab_activation", true)
    await wait_idle_frames(2)
    assert_eq(await _direction_after(vehicle_rid, "direction_increase"), 1, "forward")
    assert_eq(await _direction_after(vehicle_rid, "direction_decrease"), 0, "neutral")
    assert_eq(await _direction_after(vehicle_rid, "direction_decrease"), -1, "reverse")


func test_the_motor_car_keeps_its_circuit_thresholds() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(MOTOR_CAR_PATH)
    var engine:RailVehicleElectricSeriesEngine = null
    for component:VehicleComponent in description.get_components():
        if component is RailVehicleElectricSeriesEngine:
            engine = component
    assert_not_null(engine)
    assert_eq(engine.circuit_imin_low, 135, "Circuit: IminLo")
    assert_eq(engine.circuit_imin_high, 175, "Circuit: IminHi")


func test_an_ezt_engine_without_circuit_thresholds_has_those_of_param() -> void:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize("EngineType=ElectricSeriesMotor Volt=1500".to_utf8_buffer())
    var context:FizImportContext = FizImportContext.new()
    context.train_type = RailVehicleController.TRAIN_TYPE_EZT
    FizTrainEngineParser.new().parse(parser, context, "Engine:")
    var engine:RailVehicleElectricEngine = context.get_part("RailVehicleEngine") as RailVehicleElectricEngine
    assert_eq(engine.circuit_imin_low, EZT_IMIN_LOW)
    assert_eq(engine.circuit_imin_high, EZT_IMIN_HIGH)
