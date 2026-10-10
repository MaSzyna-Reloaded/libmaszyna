extends MaszynaGutTest

## The reverser key of an EZT steps past "forward" to the high start (DirectionForward,
## Mover.cpp:719) and shows it one position further - DirActive + (Imin == IminHi), as TTrain::Update
## feeds ggDirKey (Train.cpp:9451-9458). The cab learns it from the engine's config, never from the
## kind of train; a locomotive's key ends at "forward".

const EZT_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6bs.fiz"
const LOCOMOTIVE_PATH:String = "res://tests/fixtures/dynamic/pkp/ep09_v1/104e-039.fiz"
const DIRECTION:String = "direction"

var logic:LegacyCabinLogic


func after_each() -> void:
    if logic:
        logic.unregister()
    logic = null


func _drive(fiz_path:String) -> RID:
    var train:VehicleController = build_vehicle("TestDirectionKey", FizVehicleBuilder.build_description_at(fiz_path),
            0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var vehicle_rid:RID = train.get_rid()
    await wait_idle_frames(2)
    var controls:LegacyCabinControls = LegacyCabinControls.new()
    logic = LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return controls)
    logic.register(vehicle_rid, RailVehicleServer.vehicle_get_front_cabin(vehicle_rid))
    VehicleServer.vehicle_send_command(vehicle_rid, "battery", true)
    VehicleServer.vehicle_send_command(vehicle_rid, "cab_activation", true)
    await wait_idle_frames(2)
    return vehicle_rid


func _step(vehicle_rid:RID, command:String) -> int:
    VehicleServer.vehicle_send_command(vehicle_rid, command)
    await wait_idle_frames(2)
    return int(CabinSystem.vehicle_state_value(vehicle_rid, DIRECTION, 0))


func test_the_ezt_key_steps_to_the_high_start_and_back() -> void:
    var vehicle_rid:RID = await _drive(EZT_PATH)
    var config:Dictionary = CabinSystem.vehicle_config(vehicle_rid)
    assert_true(config.get("direction_switches_circuit_imin_high", false))
    assert_eq(config.get("direction_position_max", 0), 2)
    assert_eq(await _step(vehicle_rid, "direction_increase"), 1, "forward")
    assert_eq(await _step(vehicle_rid, "direction_increase"), 2, "the high start")
    assert_true(VehicleServer.vehicle_dump_state(vehicle_rid).get("circuit_imin_high_enabled", false))
    assert_eq(VehicleServer.vehicle_dump_state(vehicle_rid).get(DIRECTION), 1, "the vehicle still goes forward")
    assert_eq(await _step(vehicle_rid, "direction_decrease"), 1, "the high start off first")
    assert_false(VehicleServer.vehicle_dump_state(vehicle_rid).get("circuit_imin_high_enabled", true))
    assert_eq(await _step(vehicle_rid, "direction_decrease"), 0, "neutral")


func test_the_locomotive_key_ends_at_forward() -> void:
    var vehicle_rid:RID = await _drive(LOCOMOTIVE_PATH)
    var config:Dictionary = CabinSystem.vehicle_config(vehicle_rid)
    assert_false(config.get("direction_switches_circuit_imin_high", true))
    assert_eq(config.get("direction_position_max", 0), 1)
    assert_eq(await _step(vehicle_rid, "direction_increase"), 1, "forward")
    assert_eq(await _step(vehicle_rid, "direction_increase"), 1, "nothing past it")
    assert_eq(CabinSystem.vehicle_state_value(vehicle_rid, DIRECTION),
            VehicleServer.vehicle_dump_state(vehicle_rid).get(DIRECTION), "the vehicle's own direction")
