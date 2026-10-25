extends MaszynaGutTest

## The driver's door hints at the closing of the doors (Doors(), Driver.cpp:4380-4395): the doors
## closed first, then the permits revoked - closing the doors revokes the permits (Mover.cpp:
## 8745-8749), so the revoking hints have no key: a push permit button only grants
## (Train.cpp:7213-7220). On an EN57's driving trailer: a permit needed (DoorNeedPermit), no permit
## presets, the doors closed from the cab (CloseCtrl=DriverCtrl).

const CONTROL_CAR_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6bs.fiz"
var _vehicle:RID


func before_each() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(CONTROL_CAR_PATH)
    _vehicle = build_vehicle("DoorHintsTest", description, 0.0, MaszynaVehicleData.DriverType.DRIVER_HEAD).get_rid()
    # the unit's converter feeds the trailer's low voltage (BatteryStart=Disabled); alone, its own
    # battery does
    var power_supply:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            _vehicle, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    power_supply.cntrl_battery_start_mode = RailVehicleController.START_MODE_MANUAL
    power_supply.apply_config()
    # the doors are worked from the cab only with the low voltage (Mover.cpp:8669-8673)
    VehicleServer.vehicle_send_command(_vehicle, "battery", true)
    # the low voltage is reckoned on the vehicle's next step
    if not await wait_simulated_until(
            func() -> bool: return VehicleServer.vehicle_dump_state(_vehicle)["power24_available"],
            TICK, "the low voltage on the battery"):
        return
    VehicleServer.vehicle_send_command(_vehicle, "cab_activation", true)
    VehicleServer.vehicle_send_command(_vehicle, "doors_left_permit", true)
    VehicleServer.vehicle_send_command(_vehicle, "doors_right_permit", true)
    VehicleServer.vehicle_send_command(_vehicle, "doors_left", true)
    # the doors' travel: their delay, then their shift at their speed (update_doors(), Mover.cpp:8000-8008)
    await wait_simulated_until(func() -> bool: return _doors().get_left_open(),
            _doors().open_delay + _doors().max_shift / _doors().open_speed + TICK, "the left doors open")


func after_each() -> void:
    StationServer.dispatch_cancel(_vehicle)
    PlayerServer.player_leave_vehicle()


func test_a_revoking_hint_has_no_key_a_granting_one_has() -> void:
    for hint:MaszynaDriverHints.Hint in [
            MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_OFF, MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_OFF]:
        assert_false(MaszynaDriverHints.CONTROLS.has(hint), "a permit is revoked by closing the doors")
    for hint:MaszynaDriverHints.Hint in [
            MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_ON, MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_ON]:
        assert_true(MaszynaDriverHints.CONTROLS.has(hint), "a permit is granted by its button")


func test_closing_the_doors_revokes_their_permit() -> void:
    assert_true(_doors().get_left_open(), "the left doors open with their permit")
    VehicleServer.vehicle_send_command(_vehicle, "doors_left", false)
    # revoked on the doors' next step, as they start closing (Mover.cpp:7939-7944)
    if not await wait_simulated_until(func() -> bool: return not _doors().get_left_open_permit(), TICK,
            "the left doors' permit revoked"):
        return
    assert_false(_doors().get_left_open_permit(), "closing the doors revokes their permit")


func test_a_player_is_hinted_to_close_the_doors_before_the_permits() -> void:
    var driver:RID = get_vehicle_driver(_vehicle)
    attach_driver_implementation(driver, MaszynaAIDriver.new())
    PlayerServer.player_take_over_vehicle(_vehicle)
    # the hints look at the doors of the trainset the driver has checked - on its first update, at
    # once on attaching (MaszynaAIDriver)
    if not await wait_simulated_until(
            func() -> bool: return get_driver_internal_state(driver).get("trainset_vehicles", []).size() > 0,
            TICK, "the driver's trainset checked"):
        return
    var cars:Array[RID] = [_vehicle]
    StationServer.dispatch_start(_vehicle, cars)
    StationServer.dispatch_depart(_vehicle)
    # on the driver's next update, a standing driver's reaction time away at most
    if not await wait_simulated_until(
            func() -> bool: return _hints(driver).has(MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_OFF),
            MaszynaAIDriver.PREPARE_TIME + TICK, "the hint to revoke the left doors' permit"):
        return
    var hints:Array[MaszynaDriverHints.Hint] = _hints(driver)
    assert_has(hints, MaszynaDriverHints.Hint.DOOR_LEFT_CLOSE, "the open doors are to be closed")
    assert_lt(hints.find(MaszynaDriverHints.Hint.DOOR_LEFT_CLOSE),
            hints.find(MaszynaDriverHints.Hint.DOOR_LEFT_PERMIT_OFF), "the doors first, then the permit")
    assert_lt(hints.find(MaszynaDriverHints.Hint.DOOR_LEFT_CLOSE),
            hints.find(MaszynaDriverHints.Hint.DOOR_RIGHT_PERMIT_OFF), "the doors first, then the permits")
    attach_driver_implementation(driver, null)


func _doors() -> RailVehicleDoors:
    return VehicleServer.vehicle_component_get(_vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors


func _hints(driver:RID) -> Array[MaszynaDriverHints.Hint]:
    var hints:Array[MaszynaDriverHints.Hint] = []
    for entry:Dictionary in get_driver_internal_state(driver).get("hints", []):
        hints.append(entry["hint"])
    return hints
