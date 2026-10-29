extends MaszynaGutTest

## 201e-rn (pkp/et22_v3, fixtures/scenery/startup_et22_v3.scn): an FV4a handle over an LSt
## distributor, whose brake pipe is locked under 2.75 bar until the releaser is held (lock_old,
## Mover.cpp:4034-4036). Report MaSzyna-Reloaded/reports#37: a player's ET22 standing at a stop
## signal drained its pipe with the handle in cutoff, and at running it stayed empty with 7.6 bar in
## the main reservoir - the original hints the releaser only to go (Driver.cpp:8194).

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "startup_et22_v3.scn"
## A scenery node's name, read in lower case (cParser::getTokens())
const VEHICLE:String = "201e-rn"
## Simulated seconds the driver is given to get the cold ET22 ready (engine_active): the pantographs,
## the line breaker, the converter and the compressor filling the main reservoir
const ENGINE_READY_SECONDS:float = 180.0
## Simulated seconds the emergency position is given to empty the pipe under the lock
const EMPTYING_SECONDS:float = 30.0
## The pipe taken as empty [bar], as the player's was (0.00 bar). Let go just under the lock, the
## pipe rose back past it after the handle came to running and the handle filled it (measured)
const EMPTY_PIPE_PRESSURE:float = 0.01
## Simulated seconds the handle at running is given to fill the pipe past the lock, the releaser held
const FILLING_SECONDS:float = 120.0
## Simulated seconds beyond the driver's update for the clock's slice it lands in
## (SimulationServer MAX_SLICE_TIME)
const SLICE_MARGIN:float = 0.1

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


func test_a_standing_player_is_hinted_the_releaser_for_a_locked_pipe() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(_vehicle)
    assert_true(driver.is_valid(), "201e-rn has its scenery driver (headdriver)")
    if not driver.is_valid():
        return
    var brake:MaszynaMoverRailVehicleBrake = RailVehicleServer.vehicle_component_get(
            _vehicle, RailVehicleComponentType.COMPONENT_BRAKES) as MaszynaMoverRailVehicleBrake
    assert_eq(brake.cntrl_brake_handle_type, MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A, "the ET22 has an FV4a")
    assert_eq(brake.valve_type, MaszynaMoverRailVehicleBrake.BRAKE_VALVE_LST, "over an LSt distributor")
    DriverServer.driver_send_command(driver, "Prepare_engine", 1.0, 0.0)
    if not await wait_simulated_until(func() -> bool: return get_driver_internal_state(driver)["engine_active"],
            ENGINE_READY_SECONDS, "the driver's locomotive ready"):
        return

    # the player takes it over standing, with no order to go, and empties the pipe under the lock
    PlayerServer.player_take_over_vehicle(_vehicle)
    VehicleServer.vehicle_send_command(_vehicle, "brake_level_set_position", "emergency")
    if not await wait_simulated_until(
            func() -> bool: return brake.get_pipe_pressure() < EMPTY_PIPE_PRESSURE,
            EMPTYING_SECONDS, "the pipe emptied"):
        return
    VehicleServer.vehicle_send_command(_vehicle, "brake_level_set_position", "drive")
    if not await wait_simulated_until(func() -> bool: return _hinted(driver, MaszynaDriverHints.Hint.RELEASER_ON),
            MaszynaAIDriver.PREPARE_TIME + SLICE_MARGIN, "the releaser hinted"):
        return
    assert_true(_hinted(driver, MaszynaDriverHints.Hint.RELEASER_ON), "the locked pipe hints the releaser")

    VehicleServer.vehicle_send_command(_vehicle, "brake_releaser", true)
    if not await wait_simulated_until(func() -> bool: return _hinted(driver, MaszynaDriverHints.Hint.RELEASER_OFF),
            FILLING_SECONDS, "the releaser let go once the pipe is past the lock"):
        return
    assert_gt(brake.get_pipe_pressure(), MaszynaDriverBraking.PIPE_LOCK_PRESSURE, "the pipe fills, the releaser held")
    assert_false(_hinted(driver, MaszynaDriverHints.Hint.RELEASER_ON), "the releaser held, it is not asked for again")

    VehicleServer.vehicle_send_command(_vehicle, "brake_releaser", false)
    if not await wait_simulated_until(func() -> bool: return not _hinted(driver, MaszynaDriverHints.Hint.RELEASER_OFF),
            MaszynaAIDriver.PREPARE_TIME + SLICE_MARGIN, "the releaser hint gone"):
        return
    assert_false(_hinted(driver, MaszynaDriverHints.Hint.RELEASER_OFF), "the releaser let go, the hint is gone")


func _hinted(driver:RID, hint:MaszynaDriverHints.Hint) -> bool:
    for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
        if entry["hint"] == hint:
            return true
    return false
