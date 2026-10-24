extends MaszynaStartupTest

## An EN57 the player started from cold and moved off - its compressor switched on - is not hinted
## to switch the compressor on: the hint is done once any vehicle under control has its compressor
## enabled (IsAnyCompressorEnabled, driverhints.cpp:394-403), not the controlling car's compressor
## running. EN57-2067ra (pkp/en57-2000_v1, fixtures/scenery/startup_en57-2000_v1.scn).

## Simulated seconds the driver is given to update after the unit moved off
const DRIVER_UPDATE_SECONDS:float = 5.0


func test_a_compressor_switched_on_is_not_hinted() -> void:
    await run_startup("startup_en57-2000_v1.scn", "en57-2067ra", Kind.ELECTRIC_MULTIPLE_UNIT)
    var driver:RID = DriverServer.vehicle_get_driver(occupied)
    assert_true(driver.is_valid(), "the scenery's driver is seated in the unit")
    if not driver.is_valid():
        return
    await step(ticks(DRIVER_UPDATE_SECONDS))
    var cars:Array[String] = []
    var allowed:bool = false
    for car:RID in VehicleServer.vehicle_get_rids():
        var brake:RailVehicleBrake = RailVehicleServer.vehicle_component_get(
                car, RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake
        if not brake:
            continue
        allowed = allowed or (brake.compressor_speed > 0.0 and brake.get_compressor_allowed())
        cars.append("%s speed=%s power=%s allowed=%s running=%s" % [
                VehicleServer.vehicle_get_id(car), brake.compressor_speed, brake.compressor_power,
                brake.get_compressor_allowed(), brake.get_compressor_enabled()])
    assert_true(allowed, "the start-up switched a compressor on: %s" % "; ".join(cars))
    var hinted:bool = false
    for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
        hinted = hinted or entry["hint"] == MaszynaDriverHints.Hint.COMPRESSOR_ON
    assert_false(hinted, "no hint to switch on a compressor switched on: %s" % "; ".join(cars))
