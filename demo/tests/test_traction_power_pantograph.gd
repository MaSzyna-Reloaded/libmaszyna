extends MaszynaGutTest

## Regression test for the pantograph -> Mover voltage layer (TractionServer /
## RailVehicleServer::vehicle_collect_current() / RailVehicleEnginePowerSource::set_pantograph_wire_voltage()). The voltage
## fed here by hand is read back before the next step, which feeds the wire's - 0 V, no wire here.
## Bypasses scenery/geometry entirely (same style as test_train_electric_engine_power_source.gd)
## to isolate whether raising a pantograph with a wire voltage present actually reaches the
## mover's reported state - this is what a real EP07 on td.scn needs to work.

var train: VehicleController
var engine: RailVehicleElectricSeriesEngine
var power_source: RailVehicleEnginePowerSource


func before_each():
    train = build_vehicle("TestPantographTrain")
    train.add_component(build_power_supply(110.0))
    train.apply_configuration()
    engine = MoverRailVehicleElectricSeriesEngine.new()
    train.add_component(engine)
    power_source = MoverRailVehicleEnginePowerSource.new()
    power_source.source_type = RailVehicleController.POWER_SOURCE_CURRENTCOLLECTOR
    power_source.current_collector_physical_layout = 3 # both pantographs physically present
    power_source.current_collector_max_voltage = 3600.0
    power_source.current_collector_number_of_collectors = 2
    train.add_component(power_source)
    await step(2)


func test_raised_pantograph_with_wire_voltage_reaches_mover_state():
    # Only the two commands a real cabin click ever sends - no separate master-valve command,
    # since no cabin switch/keybind for that exists anywhere in this wrapper (see
    # RailVehicleEnginePowerSource::pantograph()'s own comment on why it opens the master valve itself).
    train.send_command("battery", true)
    await step(2)
    train.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    await step(2)

    assert_true(
            power_source.get_state().get("current_collector/pantograph_first_active", false),
            "pantograph should report raised once battery is on and it's been raised")

    power_source.set_pantograph_wire_voltage(RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, 3600.0)

    assert_almost_eq(
            float(power_source.get_state().get("current_collector/pantograph_first_voltage", 0.0)),
            3600.0, 1.0,
            "raised pantograph should read back the wire voltage fed in this frame")


func test_repeated_wire_voltage_updates_keep_reaching_the_mover():
    # Regression: set_pantograph_wire_voltage() used to only stash the value on the engine node;
    # RailVehicleElectricEngine::_do_update_internal_mover() (the only place that pushed it into the
    # mover) runs once at startup, so every voltage update after the first frame was silently
    # dropped. Calling this repeatedly, like RailVehicle3D does every frame, must keep working.
    train.send_command("battery", true)
    train.send_command("pantograph", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, true)
    await step(2)

    for i in range(5):
        power_source.set_pantograph_wire_voltage(RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, 3000.0 + i * 100.0)

    assert_almost_eq(
            float(power_source.get_state().get("current_collector/pantograph_first_voltage", 0.0)),
            3400.0, 1.0,
            "the mover should reflect the latest wire voltage, not just the first one ever set")


func test_lowered_pantograph_does_not_report_active():
    # current_collector/pantograph_first_voltage is a raw echo of the last wire voltage fed in,
    # not gated by is_active (pre-existing behavior, once of RailVehicleElectricEngine.cpp, unrelated to this
    # feature) - is_active is the actual gate EnginePowerSourceVoltage()/PantFrontVolt use, so
    # that's what this asserts instead of the echoed value.
    power_source.set_pantograph_wire_voltage(RailVehicleEnginePowerSource.PANTOGRAPH_FIRST, 3600.0)
    await step(2)

    assert_false(power_source.get_state().get("current_collector/pantograph_first_active", false))
