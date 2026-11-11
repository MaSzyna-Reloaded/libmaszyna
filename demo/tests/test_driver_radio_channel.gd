extends MaszynaGutTest

## A train's driver puts its radio on the next station's channel once, not on every update
## (iRadioChannel != radiochannel, Driver.cpp:1129-1132). 104e-039 (fixtures/scenery/startup_ep09_v1.scn)
## is started by its own driver, given a timetable whose stations name their channels.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "startup_ep09_v1.scn"
const VEHICLE:String = "104e-039"
## A timetable whose stations name their radio channels, looked up in the game directory's scenery
const TIMETABLE:String = "../timetables/ex6435"
const TIMETABLE_SPEED:float = 30.0
## Long enough for the driver to prepare its engine and take the train's order [s]
const ENGINE_READY_SECONDS:float = 240.0
## Long enough for the driver to update many times after [s]
const WATCH_SECONDS:float = 10.0

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _sets:int = 0
var _vehicle:RID


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(ProjectSettings.globalize_path(FIXTURES_GAME_DIR))
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    _scenery.autoload = false
    add_child(_scenery)
    await _scenery.load()
    _vehicle = VehicleServer.vehicle_get_rid_by_id(VEHICLE)


func after_each() -> void:
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _count(vehicle:RID, command:String, _p1:Variant, _p2:Variant) -> void:
    if vehicle == _vehicle and command == "radio_channel_set":
        _sets += 1


func test_the_next_stations_radio_channel_is_set_once() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(_vehicle)
    assert_true(driver.is_valid(), "the scenery's driver in the locomotive")
    if not driver.is_valid():
        return
    VehicleServer.vehicle_command_received.connect(_count)
    DriverServer.driver_send_command(driver, MaszynaSceneryProvider.TIMETABLE_ORDER + TIMETABLE, TIMETABLE_SPEED, 0.0)
    var driving:bool = await wait_simulated_until(
            func() -> bool: return get_driver_internal_state(driver)["order"] == MaszynaAIDriver.Order.OBEY_TRAIN,
            ENGINE_READY_SECONDS, "the train's order")
    if driving:
        await step(ticks(WATCH_SECONDS))
    VehicleServer.vehicle_command_received.disconnect(_count)

    if driving:
        assert_eq(_sets, 1, "the channel set once")
