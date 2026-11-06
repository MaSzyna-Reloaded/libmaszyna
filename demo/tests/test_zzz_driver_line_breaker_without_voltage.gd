extends MaszynaGutTest

## The AI driver prepares EN57AL-1230ra (pkp/en57al_v1, fixtures/scenery/startup_en57al_v1_no_wire.scn)
## on a track without a wire: the line breaker is neither cued nor worked while the pantographs find
## no voltage. The original prepares a vehicle on pantographs only with the line's voltage over its
## line breaker's lowest (ispoweravailable, Driver.cpp:2818-2825). Report 2026-10-29
## (baltyk_skm1.scn, MaSzyna-Reloaded/reports#66): the pantograph off the wire, the driver hinted to
## close the line breaker and pressed and let go of its control on every update.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "startup_en57al_v1_no_wire.scn"
const DRIVEN:String = "en57al-1230ra"
## The driver's updates watched - past the battery, the cab and the pantographs, where the line
## breaker comes next
const WATCHED_UPDATES:int = 10

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
    vehicle_rid = VehicleServer.vehicle_get_rid_by_id(DRIVEN)


func after_each():
    scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_the_line_breaker_is_left_alone_without_the_line_voltage() -> void:
    var driver:RID = DriverServer.vehicle_get_driver(vehicle_rid)
    assert_true(driver.is_valid(), "%s has its scenery driver (headdriver)" % DRIVEN)
    if not driver.is_valid():
        return
    var cabin:RID = VehicleServer.person_get_cabin(driver)
    DriverServer.driver_send_command(driver, "Prepare_engine", 1.0, 0.0)
    var worked:Array[String] = []
    var hinted:bool = false
    for _tick:int in ticks(WATCHED_UPDATES * MaszynaAIDriver.PREPARE_TIME):
        await step(1)
        # main_sw has no value until it is first moved
        var lever:Variant = CabinSystem.get_control(cabin, MaszynaCabinMainSwitch.TOGGLE_SWITCH)
        if CabinSystem.get_control(cabin, MaszynaCabinMainSwitch.ON_BUTTON) \
                or (lever and float(lever) > MaszynaCabinMainSwitch.LEVER_HELD_THRESHOLD):
            worked.append("%.1f s" % (_tick * TICK))
        for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
            hinted = hinted or entry["hint"] == MaszynaDriverHints.Hint.LINE_BREAKER_CLOSE
    var state:Dictionary = get_driver_internal_state(driver)
    assert_false(hinted, "no hint to close the line breaker without the line's voltage")
    assert_eq(worked.size(), 0, "the line breaker's control left alone, held from: %s" % ", ".join(worked.slice(0, 1)))
    assert_false(state.get("engine_active", true), "the unit not ready without the line's voltage")
    var hints:Array[String] = []
    for entry:Dictionary in state.get("hints", []):
        hints.append(entry["text"])
    assert_true(hints.size() > 0, "the driver prepares the rest of the unit: %s" % ", ".join(hints))
