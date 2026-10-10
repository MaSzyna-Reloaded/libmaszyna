extends MaszynaGutTest

## Simulated seconds the battery's drain takes to show: 2 V/s at 110 V without a converter
## (Mover.cpp:887-888), so the vehicle's next step lowers it - and that shows within a frame of the
## clock after it, which counts at most SimulationServer::MAX_FRAME_DELTA; the done check comes
## before the limit, so the frame that shows it passes however long it was
const BATTERY_DRAIN_SECONDS:float = 0.25

var train: VehicleController

## The battery voltage is configuration the Mover reads while the vehicle is being built, so it
## is authored into the model - writing it afterwards does not reach the backend until a step
## (see test_battery_start_disabled_from_zero_voltage_blocks_switching).
func _model(battery_voltage:float) -> VehicleController:
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.add_component(build_power_supply(battery_voltage))
    return model


func before_each():
    train = build_vehicle("TestTrain", _model(110.0))

# Original engine: Battery defaults to false and CheckLocomotiveParameters() only turns it on
# for a vehicle spawned ready to depart (Mover.cpp:8943), i.e. with a non-zero scenery velocity
# (DynObj.cpp:1851, `driveractive = (fVel != 0.0)`).
func test_battery_starts_off_when_not_ready_to_depart():
    assert_false(train.get_state()["battery_enabled"], "Battery should start off for initial_velocity == 0")

func test_battery_starts_on_when_ready_to_depart():
    var ready_train: VehicleController = build_vehicle("TestTrainReady", _model(110.0), 10.0)

    assert_true(ready_train.get_state()["battery_enabled"], "Battery should start on for initial_velocity != 0")


func test_successful_battery_enabling():
    train.send_command("battery", true)
    assert_true(train.get_state()["battery_enabled"], "Battery should be enabled")

func test_battery_start_disabled_from_zero_voltage_blocks_switching():
    # Original engine: CheckLocomotiveParameters() (Mover.cpp) forces BatteryStart to Disabled
    # when NominalBatteryVoltage is 0 - a load-time FIZ misconfiguration guard, not something a
    # real vehicle's voltage changes into at runtime. So this must be set before the vehicle
    # ever initializes, not mutated afterward (Battery itself, once on, isn't retroactively
    # switched off by a later voltage change - only BatterySwitch()'s manual-mode gate is).
    var disabled_train: VehicleController = build_vehicle("TestTrainZeroVoltage", _model(0.0))

    assert_false(disabled_train.get_state()["battery_enabled"], "Battery should start off when BatteryStart is forced Disabled")
    disabled_train.send_command("battery", true)
    assert_false(disabled_train.get_state()["battery_enabled"], "BatterySwitch should have no effect while BatteryStart is Disabled")


func test_successful_battery_voltage_drop_after_two_seconds():
    train.send_command("battery", true)
    assert_true(train.get_state()["battery_enabled"], "Battery should be enabled")
    var before:float = train.get_state()["battery_voltage"]
    if not await wait_simulated_until(func() -> bool: return train.get_state()["battery_voltage"] < before,
            BATTERY_DRAIN_SECONDS, "the battery's drain"):
        return
    var after:float = train.get_state()["battery_voltage"]

    assert_true(before > after, "There should be a battery voltage drop")
