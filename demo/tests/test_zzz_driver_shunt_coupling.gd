extends MaszynaGutTest

## SN61-02 told by the scenery to couple up to the SN61 it stands against - `Shunt -3 -99`, as
## calkowo's shunting events say it (events_noc_zimowa.ctr:1398). 99 asks for the heating line too,
## and no SN61 coupler has one (AllowedFlag=39): the driver joined what the two couplers can join and
## then asked for the heating line for ever, standing in its CONNECT order (report 2026-10-09,
## calkowo SN61 at night). The original's Attach() sets the asked couplings whatever the couplers
## allow (Mover.cpp:576-583), so its UpdateConnect() ends there (Driver.cpp:7028).
##
## Followed by events, not by time: a `coupler_connect` sent once the vehicles are joined by all they
## can join is the deadlock; `driver_order_changed` to an order past CONNECT is the end of it.

const Order = MaszynaAIDriver.Order
const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "shunt_sn61_couple.scn"
## `Shunt <vehicles> <coupler>` of the calkowo events: no vehicles left behind, coupled by 99 -
## the screw coupler, both hoses and the heating line
const SHUNT_VEHICLES:float = -3.0
const SHUNT_COUPLER:float = -99.0
const SHUNT_COUPLINGS:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_COUPLER
        | RailVehicleController.COUPLING_FLAG_BRAKEHOSE | RailVehicleController.COUPLING_FLAG_MAINHOSE
        | RailVehicleController.COUPLING_FLAG_HEATING) as RailVehicleController.CouplingFlags
## Real seconds the run may take before it counts as hung - only a hang guard, as LOAD_TIMEOUT:
## the outcome is decided by the events above
const HANG_GUARD_SECONDS:float = 60.0
const MSEC_PER_SECOND:float = 1000.0

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _driven:RID
var _standing:RID
var _driver:RID
## The end of the driven vehicle the driver couples, from its first `coupler_connect`
var _coupling_end:RailVehicleController.CouplerEnd = RailVehicleController.COUPLER_END_FRONT
var _connecting:bool = false
## Joined as asked after the last `coupler_connect` - the announcement comes once the command ran
var _joined_after_last_connect:bool = false
## A `coupler_connect` sent with everything asked for that can be joined joined already
var _asked_beyond_joinable:bool = false
## The driver took up an order past CONNECT, coupled
var _connected:bool = false


func before_each() -> void:
    _connecting = false
    _joined_after_last_connect = false
    _asked_beyond_joinable = false
    _connected = false
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    add_child(_scenery)
    if not await wait_loaded(_scenery.scenery_loaded, SCENERY):
        return
    _driven = VehicleServer.vehicle_get_rid_by_id("sn61-02")
    _standing = VehicleServer.vehicle_get_rid_by_id("sn61-03")
    _driver = DriverServer.vehicle_get_driver(_driven)
    VehicleServer.vehicle_command_received.connect(_on_vehicle_command_received)
    DriverServer.driver_order_changed.connect(_on_driver_order_changed)


func after_each() -> void:
    VehicleServer.vehicle_command_received.disconnect(_on_vehicle_command_received)
    DriverServer.driver_order_changed.disconnect(_on_driver_order_changed)
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_shunt_couples_by_what_the_couplers_have_and_drives_on() -> void:
    assert_true(_driver.is_valid(), "the scenery's driver is seated in SN61-02")
    if not is_passing():
        return
    DriverServer.driver_send_command(_driver, "Shunt", SHUNT_VEHICLES, SHUNT_COUPLER)
    var started:int = Time.get_ticks_msec()
    while not _connected and not _asked_beyond_joinable:
        if Time.get_ticks_msec() - started > HANG_GUARD_SECONDS * MSEC_PER_SECOND:
            fail_test("the run hung: neither coupled nor asking beyond what can be joined")
            return
        await step(1)
    assert_false(_asked_beyond_joinable,
            "no coupling asked for once joined by all the couplers can join: %s" % _coupling_text())
    assert_true(_connected, "on to the order after CONNECT, coupled: %s" % _coupling_text())
    var both_have:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_COUPLER
            | RailVehicleController.COUPLING_FLAG_BRAKEHOSE | RailVehicleController.COUPLING_FLAG_MAINHOSE) \
            as RailVehicleController.CouplingFlags
    assert_true(_joined_by(both_have), "joined by the screw coupler and both hoses, which both have")
    assert_false(_joined_by(RailVehicleController.COUPLING_FLAG_HEATING), "no heating line, which neither has")


func _on_vehicle_command_received(vehicle:RID, command:String, p1:Variant, _p2:Variant) -> void:
    if not vehicle == _driven or not command == "coupler_connect":
        return
    if _joined_after_last_connect:
        _asked_beyond_joinable = true
    _connecting = true
    _coupling_end = p1
    _joined_after_last_connect = _joined_as_asked()


func _on_driver_order_changed(driver:RID) -> void:
    if not driver == _driver or not _connecting:
        return
    var order:Order = get_driver_internal_state(driver)["order"] as Order
    if not order & Order.CONNECT and _joined_as_asked():
        _connected = true


## Joined by everything the shunt asks for that the two couplers can join
func _joined_as_asked() -> bool:
    return _joined_by(_asked_joinable())


func _asked_joinable() -> RailVehicleController.CouplingFlags:
    return (SHUNT_COUPLINGS & RailVehicleServer.vehicle_get_coupler_joinable_flags(_driven, _coupling_end)) \
            as RailVehicleController.CouplingFlags


func _joined_by(flags:RailVehicleController.CouplingFlags) -> bool:
    return not flags == RailVehicleController.COUPLING_FLAG_NONE \
            and _standing in RailVehicleServer.vehicle_get_coupled(_driven, _coupling_end, flags)


## The flags by their names
static func _flag_names(flags:RailVehicleController.CouplingFlags) -> String:
    var names:PackedStringArray = []
    # a native enum is no value in GDScript: its names come from the class database
    for flag_name:String in ClassDB.class_get_enum_constants(&"RailVehicleController", &"CouplingFlags"):
        var flag:RailVehicleController.CouplingFlags = ClassDB.class_get_integer_constant(
                &"RailVehicleController", flag_name) as RailVehicleController.CouplingFlags
        if not flag == RailVehicleController.COUPLING_FLAG_NONE and (flags & flag) == flag:
            names.append(flag_name)
    return " | ".join(names) if names else "COUPLING_FLAG_NONE"


func _coupling_text() -> String:
    return "asked and joinable %s, order %s" % [_flag_names(_asked_joinable()),
            MaszynaAIDriver.order_text(get_driver_internal_state(_driver)["order"], 0, false)]
