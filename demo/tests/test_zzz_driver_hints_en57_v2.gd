extends MaszynaStartupTest

## EN57-636ra (pkp/en57_v2, fixtures/scenery/startup_en57-636ra.scn) stopped on its EP brake and
## told to move off: its FVel6 handle releases the EP brake at the EP releasing position, below the
## driving position, which only holds the EP brake applied (bh_EPN = bh_RP, hamulce.cpp:34) - where
## the driver's own release leaves it (DecBrake(), Driver.cpp:3321-3325). Released there, the player
## is not sent back to the driving position (reports#16).

## The train brake handle's normalized setting for FVel6's position 3, the EP brake applied
## (pos_table, hamulce.cpp:34: -1 to 6)
const EP_BRAKE_LEVEL:float = 4.0 / 7.0
## Simulated seconds the cylinders are given to fill and to empty
const BRAKE_FILL_SECONDS:float = 10.0
## Simulated seconds the driver is given to update: a standing driver's reaction time at most
const DRIVER_UPDATE_SECONDS:float = 5.0
## The scenery's order that makes the driver want to move off, as l053_poranek.scn gives the unit
## (ShuntVelocity, km/h)
const SHUNT_VELOCITY:float = 40.0
## [bar]
const RELEASED_PRESSURE:float = 0.1
## [handle position]
const HANDLE_TOLERANCE:float = 0.001


func test_an_ep_brake_released_at_ep_releasing_is_not_hinted_back_to_driving() -> void:
    await run_startup("startup_en57-636ra.scn", "en57-636ra", Kind.ELECTRIC_MULTIPLE_UNIT)
    var driver:RID = DriverServer.vehicle_get_driver(occupied)
    assert_true(driver.is_valid(), "the scenery's driver is seated in the unit")
    if not driver.is_valid():
        return
    var brake:RailVehicleBrake = _brake(occupied)
    VehicleServer.vehicle_send_command(occupied, "brake_level_set", EP_BRAKE_LEVEL)
    await step(ticks(BRAKE_FILL_SECONDS))
    await key_tap(&"brake_level_drive")
    await step(ticks(BRAKE_FILL_SECONDS))
    assert_gt(brake.get_air_pressure(), RELEASED_PRESSURE, "the driving position holds the EP brake applied")
    DriverServer.driver_send_command(driver, "ShuntVelocity", SHUNT_VELOCITY, 0.0)
    if not await wait_simulated_until(
            func() -> bool: return _hint(driver, MaszynaDriverHints.Hint.BRAKING_FORCE_DECREASE).size() > 0,
            DRIVER_UPDATE_SECONDS, "the hint to reduce the braking force"):
        return
    var decrease:Dictionary = _hint(driver, MaszynaDriverHints.Hint.BRAKING_FORCE_DECREASE)
    assert_eq(decrease["control"], MaszynaDriverHints.TRAIN_BRAKE, "by the train brake handle")
    assert_eq(decrease["gesture"], CabinLogic.Gesture.DECREASE, "stepped down")

    await key_tap(&"brake_level_decrease")
    assert_almost_eq(brake.get_controller_position(),
            brake.get_handle_position(RailVehicleBrake.HANDLE_POSITION_EP_RELEASE), HANDLE_TOLERANCE, "at EP releasing")
    if not await wait_simulated_until(func() -> bool: return brake.get_air_pressure() < RELEASED_PRESSURE,
            BRAKE_FILL_SECONDS, "the EP brake released"):
        return
    await step(ticks(DRIVER_UPDATE_SECONDS))
    var release:Dictionary = _hint(driver, MaszynaDriverHints.Hint.BRAKING_FORCE_SET_ZERO)
    assert_true(release.is_empty() or release["done"],
            "released at EP releasing, not sent back to the driving position: %s" % release)


## The driver's `hint` as the HUD reads it; empty while there is none
func _hint(driver:RID, hint:MaszynaDriverHints.Hint) -> Dictionary:
    for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
        if entry["hint"] == hint:
            return entry
    return {}
