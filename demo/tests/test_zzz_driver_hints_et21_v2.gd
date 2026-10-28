extends MaszynaStartupTest

## 3e2 (pkp/et21_v2, fixtures/scenery/startup_et21_v2.scn) started cold by a player, its pantograph
## tank on the main reservoir behind a manual three-way valve (no PantAutoValve). Report
## MaSzyna-Reloaded/reports#36: the tank spawns with enough air, the pantographs are hinted up, and
## once the main reservoir falls short the valve's hint came last, below them - the player ran the
## small compressor into the main reservoir, which it never fills (Mover.cpp:802-806).

## The pantographs' raising pressure with its margin [bar] (maszyna_driver_pantographs.gd)
const SHORT_OF_AIR:float = MaszynaDriverPantographs.RAISING_PRESSURE + MaszynaDriverPantographs.RAISING_MARGIN
## Simulated seconds the main reservoir is given to fall short, the train brake released from it
const LEAK_SECONDS:float = 400.0
## Simulated seconds the driver is given to update: a standing driver's reaction time at most
const DRIVER_UPDATE_SECONDS:float = 5.0


func test_pantograph_air_is_hinted_before_the_pantographs() -> void:
    if not await enter_vehicle("startup_et21_v2.scn", "3e2"):
        return
    var driver:RID = DriverServer.vehicle_get_driver(occupied)
    assert_true(driver.is_valid(), "the scenery's driver is seated in the locomotive")
    if not driver.is_valid():
        return
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            carrier, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    assert_false(power_source.cntrl_pantograph_auto_valve, "the three-way valve is turned by hand")
    await key_tap(&"battery_toggle")
    await key_tap(&"cab_activation_toggle")
    await key_tap(&"brake_level_drive")
    if not await wait_simulated_until(func() -> bool: return _index(driver, MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_ON) >= 0,
            DRIVER_UPDATE_SECONDS, "the pantographs hinted up with the tank on the main reservoir"):
        return
    if not await wait_simulated_until(
            func() -> bool: return power_source.get_collector_pantograph_tank_pressure() < SHORT_OF_AIR,
            LEAK_SECONDS, "the main reservoir short of the pantographs' air"):
        return
    if not await wait_simulated_until(
            func() -> bool: return _index(driver, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY) >= 0,
            DRIVER_UPDATE_SECONDS, "the three-way valve hinted"):
        return
    var valve:int = _index(driver, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_AUXILIARY)
    var compressor:int = _index(driver, MaszynaDriverHints.Hint.PANTOGRAPH_COMPRESSOR_ON)
    var raising:int = _index(driver, MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_ON)
    var texts:String = str(_texts(driver))
    assert_lt(valve, compressor, "the valve before the compressor: %s" % texts)
    assert_lt(compressor, raising, "the compressor before the pantographs: %s" % texts)


func test_pantograph_compressor_off_is_hinted_with_its_switch() -> void:
    assert_eq(MaszynaDriverHints.CONTROLS[MaszynaDriverHints.Hint.PANTOGRAPH_COMPRESSOR_OFF],
            MaszynaDriverHints.CONTROLS[MaszynaDriverHints.Hint.PANTOGRAPH_COMPRESSOR_ON],
            "the compressor runs while its switch is held - let go of the same one")


## The position of the driver's `hint` in the list the HUD reads; -1 while it is not there
func _index(driver:RID, hint:MaszynaDriverHints.Hint) -> int:
    var hints:Array = get_driver_internal_state(driver).get("hints", [])
    for index:int in hints.size():
        if hints[index]["hint"] == hint:
            return index
    return -1


func _texts(driver:RID) -> Array:
    return get_driver_internal_state(driver).get("hints", []).map(func(entry:Dictionary) -> String: return entry["text"])
