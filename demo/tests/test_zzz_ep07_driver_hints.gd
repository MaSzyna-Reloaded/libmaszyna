extends MaszynaGutTest

## The driver's hints to a player on the REAL EP07-424 (ep07.scn, its own .fiz and .mmd), whose
## pantograph tank has a manual three-way valve (303e-ep.fiz has no PantAutoValve). Report
## MaSzyna-Reloaded/reports#9: a player took the locomotive with the valve on the small compressor,
## the tank leaked, the line breaker and the pantograph went down, and nothing told the player to
## turn the valve back (pantographairsourcesetmain, Driver.cpp:6226; driverhints.cpp:214).

## EP07-424 of td.scn on a cut of its line, with the EP07's own .fiz and .mmd (demo/tests/fixtures)
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "ep07.scn"
## A scenery node's name, read in lower case (cParser::getTokens())
const VEHICLE:String = "ep07-424"
## Simulated seconds the driver takes to get the cold EP07 ready (engine_active): the converter on
## its fifth update, PREPARE_TIME apart, then the compressor filling the main reservoir past
## MIN_MAIN_RESERVOIR_PRESSURE from the 4.1 bar it spawns with while the brake pipe charges from it,
## and the update that sees it - 17.7 s measured - and one more of its updates (PREPARE_TIME, 2 s)
const ENGINE_READY_SECONDS:float = 20.0
## Simulated seconds beyond the driver's update for the clock's slice it lands in
## (SimulationServer MAX_SLICE_TIME)
const SLICE_MARGIN:float = 0.1
## How long the scenery's horn sounds [s] (`Warning_signal`)
const WARNING_DURATION:float = 3.0

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _vehicle:RID


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    add_child(_scenery)
    # the scenery is announced once its vehicles are built and its drivers given their AI
    if not await wait_loaded(_scenery.scenery_loaded, SCENERY):
        return
    _vehicle = VehicleServer.vehicle_get_rid_by_id(VEHICLE)


func after_each() -> void:
    PlayerServer.player_leave_vehicle()
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_a_player_with_the_pantograph_tank_cut_off_is_hinted_to_turn_the_valve() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(_vehicle)
    assert_true(driver.is_valid(), "EP07-424 has its scenery driver (headdriver)")
    if not driver.is_valid():
        return
    var power_source:RailVehicleEnginePowerSource = RailVehicleServer.vehicle_component_get(
            _vehicle, RailVehicleComponentType.COMPONENT_ENGINE_POWER_SOURCE) as RailVehicleEnginePowerSource
    assert_false(power_source.cntrl_pantograph_auto_valve, "the EP07's three-way valve is turned by hand")
    DriverServer.driver_send_command(driver, "Prepare_engine", 1.0, 0.0)
    if not await wait_simulated_until(func() -> bool: return get_driver_internal_state(driver)["engine_active"],
            ENGINE_READY_SECONDS, "the driver's locomotive ready"):
        return
    assert_true(get_driver_internal_state(driver)["engine_active"], "the driver gets the locomotive ready")

    # the player takes it over with the valve on the small compressor
    PlayerServer.player_take_over_vehicle(_vehicle)
    VehicleServer.vehicle_send_command(_vehicle, "pantograph_compressor_valve", true)
    # the driver hints on its next update, PREPARE_TIME at the latest for a standing vehicle
    if not await wait_simulated_until(
            func() -> bool: return _hinted(driver, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN),
            MaszynaAIDriver.PREPARE_TIME + SLICE_MARGIN, "the hint to turn the valve"):
        return

    assert_true(_hinted(driver, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN),
            "the player is hinted to turn the valve to the main reservoir")
    assert_true(power_source.get_collector_pantograph_compressor_valve(), "and the driver leaves the valve to the player")

    VehicleServer.vehicle_send_command(_vehicle, "pantograph_compressor_valve", false)
    if not await wait_simulated_until(
            func() -> bool: return not _hinted(driver, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN),
            MaszynaAIDriver.PREPARE_TIME + SLICE_MARGIN, "the hint to turn the valve gone"):
        return
    assert_false(_hinted(driver, MaszynaDriverHints.Hint.PANTOGRAPH_AIR_SOURCE_SET_MAIN),
            "the valve turned, the hint is gone")


## `Warning_signal`: the horn the scenery asks for is hinted to a player, and goes once it sounds
## (Driver.cpp:4860-4861; driverhints.cpp:993-1004)
func test_the_horn_a_scenery_asks_for_is_hinted() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(_vehicle)
    PlayerServer.player_take_over_vehicle(_vehicle)

    DriverServer.driver_send_command(driver, "Warning_signal", WARNING_DURATION, float(MaszynaDriverHints.HORN_LOW))
    assert_true(_hinted(driver, MaszynaDriverHints.Hint.HORN_ON), "the player is hinted to sound the horn")

    VehicleServer.vehicle_send_command(_vehicle, "horn_low", true)
    # the driver sees the horn on its next update, PREPARE_TIME at the latest for a standing vehicle
    if not await wait_simulated_until(func() -> bool: return not _hinted(driver, MaszynaDriverHints.Hint.HORN_ON),
            MaszynaAIDriver.PREPARE_TIME + SLICE_MARGIN, "the horn hint gone"):
        return
    assert_false(_hinted(driver, MaszynaDriverHints.Hint.HORN_ON), "the horn sounding, the hint is gone")
    VehicleServer.vehicle_send_command(_vehicle, "horn_low", false)


func _hinted(driver:RID, hint:MaszynaDriverHints.Hint) -> bool:
    for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
        if entry["hint"] == hint:
            return true
    return false
