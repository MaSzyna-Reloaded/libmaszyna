extends MaszynaGutTest

## The oil pump raises the pressure by this much a second while the engine stands
## (Mover.cpp:2063, enrot <= 5) [bar/s]
const OIL_PUMP_PRESSURE_RATE:float = 0.035
## The brake released to below this pressure [bar] - what the tests assert
const RELEASED_BRAKE_PRESSURE:float = 0.1
## Simulated seconds between the two notches of the master controller, and within which the loco
## moves off after the second - a driver's pace, what the test claims of the SM42
const NOTCH_SECONDS:float = 1.0
const MOVING_OFF_SECONDS:float = 2.0

var train: VehicleController
var engine: RailVehicleDieselEngine
var master_controller: RailVehicleMasterController
var brake: RailVehicleBrake

func before_each():
    # A startup sequence is a driver operating the loco, so the cab is occupied and switched on.
    # Without it CabActive stays 0 and TMoverParameters::ComputeTotalForce() switches the physics
    # off once LastSwitchingTime passes 5 s (Mover.cpp:4485) - the engine runs and the vehicle
    # never moves.
    train = build_vehicle("TestTrain", load("res://tests/fixtures/sm42_vehicle.tres"), 0.0,
            MaszynaDynamicData.DriverType.DRIVER_HEAD)
    engine = train.get_component(VehicleComponentType.COMPONENT_ENGINE) as RailVehicleDieselEngine
    master_controller = train.get_rail_component(RailVehicleComponentType.COMPONENT_MASTER_CONTROLLER) \
            as RailVehicleMasterController
    brake = train.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
    train.send_command("battery", true)
    train.send_command("cab_activation", true)
    assert_true(train.get_state()["battery_enabled"], "the battery switched on")
    assert_false(int(train.get_state()["cabin"]) == 0, "the cab switched on")

## Simulated seconds the oil pump takes from cold to the pressure the engine starts with
## (dizel_StartupCheck, Mover.cpp:7072): its minimum at OIL_PUMP_PRESSURE_RATE, from the next step
## on
func _oil_pressure_seconds() -> float:
    return engine.oil_pump_pressure_minimum / OIL_PUMP_PRESSURE_RATE + TICK

## Simulated seconds the engine takes to fire once the main switch starts it: the controller's
## InitialCtrlDelay (Mover.cpp:7103), from the next step on
func _ignition_seconds() -> float:
    return master_controller.initial_delay + TICK

## Simulated seconds the brake takes to release: no longer than the release time its fixture
## declares for its delay setting - BrakeDelays=GP sets G (Mover.cpp:8988), released in BDelay3
## (the original's driver reads it so, Driver.cpp:8143); the releaser only hastens it
func _brake_release_seconds() -> float:
    return brake.cntrl_brake_delay_3 + TICK

func test_successful_enabling_oil_pump():
    train.send_command("oil_pump", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["oil_pump_active"],
            TICK, "the oil pump running"):
        return
    assert_true(train.get_state()["oil_pump_active"], "Oil pump should be active")

func test_successful_enabling_fuel_pump():
    train.send_command("fuel_pump", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["fuel_pump_active"],
            TICK, "the fuel pump running"):
        return
    assert_true(train.get_state()["fuel_pump_active"], "Fuel pump should be active")

func test_successful_pumping_the_oil():
    var before:float = train.get_state()["oil_pump_pressure"]
    train.send_command("oil_pump", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["oil_pump_pressure"] > before,
            TICK, "the oil pressure rising"):
        return
    var after:float = train.get_state()["oil_pump_pressure"]
    assert_true(after > before, "There should be a oil pump pressure increase")

func test_successful_turning_engine_on():
    train.send_command("fuel_pump", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["fuel_pump_active"],
            TICK, "the fuel pump running"):
        return
    train.send_command("oil_pump", true)
    if not await wait_simulated_until(func() -> bool:
            return train.get_state()["oil_pump_pressure"] >= engine.oil_pump_pressure_minimum,
            _oil_pressure_seconds(), "the oil pressure to start with"):
        return
    train.send_command("oil_pump", false)
    train.send_command("fuel_pump", false)
    train.send_command("main_switch", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["engine_rpm"] > 0,
            _ignition_seconds(), "the engine firing"):
        return
    assert_true(train.get_state()["engine_rpm"] > 0, "Engine RPM should be > 0")

func test_successful_brake_releasing():
    train.send_command("brake_level_set_position", "drive")
    train.send_command("brake_releaser", true)
    if not await wait_simulated_until(
            func() -> bool: return train.get_state()["brake_air_pressure"] < RELEASED_BRAKE_PRESSURE,
            _brake_release_seconds(), "the brake released"):
        return
    var value = train.get_state()["brake_air_pressure"]
    assert_true(value < RELEASED_BRAKE_PRESSURE, "Brake air pressure should be < 0.1, got %s" % value)

func test_successful_moving_on():
    train.send_command("fuel_pump", true)
    train.send_command("brake_level_set_position", "drive")
    train.send_command("brake_releaser", true)
    train.send_command("oil_pump", true)
    if not await wait_simulated_until(func() -> bool:
            return train.get_state()["oil_pump_pressure"] >= engine.oil_pump_pressure_minimum,
            _oil_pressure_seconds(), "the oil pressure to start with"):
        return
    train.send_command("oil_pump", false)
    train.send_command("fuel_pump", false)
    train.send_command("main_switch", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["engine_rpm"] > 0,
            _ignition_seconds(), "the engine firing"):
        return
    if not await wait_simulated_until(
            func() -> bool: return train.get_state()["brake_air_pressure"] < RELEASED_BRAKE_PRESSURE,
            _brake_release_seconds(), "the brake released"):
        return
    train.send_command("direction_increase")
    train.send_command("main_controller_increase", true)
    await step(ticks(NOTCH_SECONDS))
    train.send_command("main_controller_increase", true)
    if not await wait_simulated_until(func() -> bool: return train.get_state()["speed"] > 0, MOVING_OFF_SECONDS,
            "the loco moving off"):
        return
    assert_true(train.get_state()["engine_rpm"] > 0, "Engine RPM should be > 0")  # recheck
    assert_true(train.get_state()["brake_air_pressure"] < 0.1, "Brake air pressure should be < 0.1")  # recheck
    assert_true(train.get_state()["speed"] > 0, "Speed should be > 0")
