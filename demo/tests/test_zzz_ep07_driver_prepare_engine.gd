extends MaszynaGutTest

## The AI driver prepares the REAL EP07-424 (ep07.scn, its own .fiz and .mmd) for the road through
## its cab, to the end of the start sequence: low voltage, line breaker, converter
## (Driver.cpp PrepareEngine). The tester's report of 2026-10-04 had the driver stop at the line
## breaker; test_driver_server.gd checks the order only up to the battery, on a diesel.

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"
## The driver's update the converter comes on with: the battery and the pantographs on the first,
## the line breaker closed on the fourth, its button held over InitialCtrlDelay between them, the
## converter on the fifth (8.1 s measured) - each PREPARE_TIME after the one before, the first
## within PREPARE_TIME of the order
const CONVERTER_UPDATE:int = 5
## Simulated seconds beyond the driver's update for the clock's slice it lands in
## (SimulationServer MAX_SLICE_TIME)
const SLICE_MARGIN:float = 0.1

var _previous_game_dir:String = ""
var scenery:MaszynaSceneryNode
var vehicle_rid:RID


func before_each():
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    scenery = MaszynaSceneryNode.new()
    scenery.filename = SCENERY
    add_child(scenery)
    # the scenery is announced once its vehicles are built and its drivers given their AI
    if not await wait_loaded(scenery.scenery_loaded, SCENERY):
        return
    vehicle_rid = VehicleServer.vehicle_get_rid_by_id("ep07-424")


func after_each():
    scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_the_driver_prepares_an_electric_locomotive_to_the_converter() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(vehicle_rid)
    assert_true(driver.is_valid(), "EP07-424 has its scenery driver (headdriver)")
    if not driver.is_valid():
        return
    var power_supply:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            vehicle_rid, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    var engine:RailVehicleEngine = VehicleServer.vehicle_component_get(
            vehicle_rid, VehicleComponentType.COMPONENT_ENGINE) as RailVehicleEngine
    assert_not_null(power_supply, "the EP07's FIZ (Light: LMaxVoltage) gives it a power supply")
    assert_not_null(engine, "and an engine")
    if not power_supply or not engine:
        return
    assert_false(engine.get_main_switch_enabled(), "the locomotive stands cold, the line breaker open")

    DriverServer.driver_send_command(driver, "Prepare_engine", 1.0, 0.0)
    if not await wait_simulated_until(power_supply.get_converter_enabled,
            CONVERTER_UPDATE * MaszynaAIDriver.PREPARE_TIME + SLICE_MARGIN, "the converter the driver starts"):
        return

    assert_true(power_supply.get_power24_available(), "the battery gives the low voltage")
    assert_true(engine.get_main_switch_enabled(), "the driver closes the line breaker")
    assert_true(power_supply.get_converter_enabled(), "and starts the converter")
    assert_true(power_supply.get_power110_available(), "which gives the 110 V circuits")
