extends MaszynaStartupTest

## A unit the player started from cold and moved off is ready for its driver: the driver's
## readiness (PrepareEngine(), Driver.cpp:2893) lists nothing missing, so the driving aid does not
## stand at "Vehicle not ready". 36WEa-014A (pkp/impuls_v1, fixtures/scenery/startup_36wea-014a.scn):
## two units of three cars, the middle one with a power of its own and no engine
## (docs/findings-archive.md, 2026-10-06 Vehicle not ready with nothing missing), and on the move
## the driver's hints ask for the A car's suggested pantograph setup.

## Simulated seconds the driver is given to update after the unit moved off
const DRIVER_UPDATE_SECONDS:float = 5.0


func test_a_unit_moved_off_is_ready_for_its_driver() -> void:
    await run_startup("startup_36wea-014a.scn", "36wea-014a", Kind.ELECTRIC_MULTIPLE_UNIT, Pantographs.SELECTOR)
    var driver:RID = DriverServer.vehicle_get_driver(occupied)
    assert_true(driver.is_valid(), "the scenery's driver is seated in the unit")
    if not driver.is_valid():
        return
    await step(ticks(DRIVER_UPDATE_SECONDS))
    var state:Dictionary = get_driver_internal_state(driver)
    var cars:Array[String] = []
    for car:RID in VehicleServer.vehicle_get_rids():
        var dump:Dictionary = VehicleServer.vehicle_dump_state(car)
        cars.append("%s power=%s main_switch=%s pantograph=%s/%s voltage=%s" % [
                VehicleServer.vehicle_get_id(car), VehicleServer.vehicle_get_controller(car).power,
                dump.get("main_switch_enabled"), dump.get("current_collector/pantograph_first_active"),
                dump.get("current_collector/pantograph_second_active"), dump.get("current_collector/voltage")])
    assert_eq(state.get("engine_missing", -1), 0, "nothing missing: %s" % "; ".join(cars))
    assert_true(state.get("engine_active", false), "the driver takes it ready")
    # the A car's setup is B alone (Pantstate=2), the C car's pantograph, raised and A lowered for
    # the unit from the A car (Driver.cpp:6306-6318)
    var texts:Array[String] = []
    for hint:Dictionary in state.get("hints", []):
        texts.append(hint["text"])
    var unit:RID = RailVehicleServer.vehicle_find_pantograph_carrier(occupied)
    var source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            unit, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    texts.append("unit %s collectors=%d valves=%s/%s" % [
            VehicleServer.vehicle_get_id(unit), source.current_collector_number_of_collectors,
            source.get_collector_pantograph_first_valve_active(), source.get_collector_pantograph_second_valve_active()])
    assert_true(MaszynaDriverHints.TEXTS[MaszynaDriverHints.Hint.REAR_PANTOGRAPH_VALVE_ON] in texts,
            "a hint to raise pantograph B: %s" % [texts])
    assert_true(MaszynaDriverHints.TEXTS[MaszynaDriverHints.Hint.FRONT_PANTOGRAPH_VALVE_OFF] in texts,
            "a hint to lower pantograph A: %s" % [texts])
