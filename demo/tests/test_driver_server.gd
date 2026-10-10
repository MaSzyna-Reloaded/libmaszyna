extends MaszynaGutTest

## The name of an implementation registered only after its driver was declared
const REGISTERED_LATER:StringName = &"test_registered_later"


func test_a_driver_is_a_person_aboard_a_vehicle() -> void:
    var vehicle:RID = build_vehicle("DriverTest", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD).get_rid()
    var driver:RID = get_vehicle_driver(vehicle)

    attach_driver_implementation(driver, UpdateCounter.new())

    assert_eq(VehicleServer.person_get_vehicle(driver), vehicle)
    assert_eq(DriverServer.vehicle_get_driver(vehicle), driver)
    PersonServer.person_free(driver)
    assert_false(DriverServer.vehicle_get_driver(vehicle).is_valid(), "a freed driver leaves its vehicle")


func test_the_drivers_are_listed_and_their_vehicles_announced() -> void:
    var vehicle:RID = build_vehicle("DriverListTest").get_rid()
    watch_signals(DriverServer)
    watch_signals(VehicleServer)
    var driver:RID = PersonServer.person_create()

    RailVehicleServer.person_enter_front_cabin(driver, vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    attach_driver_implementation(driver, UpdateCounter.new())
    assert_has(DriverServer.driver_get_rids(), driver)
    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_entered",
            [RailVehicleServer.vehicle_get_front_cabin(vehicle), driver, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER])
    PersonServer.person_free(driver)
    assert_signal_emitted_with_parameters(DriverServer, "driver_freed", [driver])
    assert_does_not_have(DriverServer.driver_get_rids(), driver)


func test_a_vehicle_is_driven_by_its_driver_or_a_player() -> void:
    var vehicle:RID = build_vehicle("DrivenTest").get_rid()
    var front:RID = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    watch_signals(RailVehicleServer)
    var driver:RID = PersonServer.person_create()

    assert_false(VehicleServer.vehicle_has_person_role(vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER), "nobody drives it yet")
    RailVehicleServer.person_enter_front_cabin(driver, vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER)
    attach_driver_implementation(driver, UpdateCounter.new())
    assert_true(VehicleServer.vehicle_has_person_role(vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER))
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed", [vehicle, front])
    PlayerServer.player_take_over_vehicle(vehicle)
    assert_true(VehicleServer.vehicle_has_person_role(vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER), "a player taking over changes nothing")
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), front, "a player taking over changes nothing")
    PersonServer.person_free(driver)
    assert_true(VehicleServer.vehicle_has_person_role(vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER), "the player still drives it")
    PlayerServer.player_leave_vehicle()
    assert_false(VehicleServer.vehicle_has_person_role(vehicle, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER), "the player left, and it has no driver")
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed", [vehicle, RID()])


func test_a_scheduled_update_reaches_the_implementation() -> void:
    var implementation:UpdateCounter = UpdateCounter.new()
    var driver:RID = _create_driver(implementation)

    DriverServer.driver_schedule_update(driver, 0.0)
    DriverServer.driver_schedule_update(driver, 0.0)
    # an update scheduled at once is delivered on the clock's next step - and only one
    if not await wait_simulated_until(func() -> bool: return implementation.updates > 0, TICK, "the scheduled update"):
        return
    await step(1)

    assert_eq(implementation.updates, 1, "a later schedule replaces the pending one")


## A driver is declared with the name of what it thinks with: until an implementation is registered
## under it, it thinks with nothing; registered, the implementation thinks for it - declared before
## or after - and with the name unregistered, nothing does again
func test_a_driver_thinks_with_the_implementation_registered_under_its_name() -> void:
    var vehicle:VehicleController = build_vehicle("TestTrain", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var driver:RID = get_vehicle_driver(vehicle.get_rid())
    DriverServer.driver_attach_implementation(driver, REGISTERED_LATER)
    assert_has(DriverServer.driver_get_rids(), driver, "declared, it is a driver")
    assert_null(DriverServer.driver_get_implementation(driver), "nothing registered under its name yet")
    var implementation:UpdateCounter = UpdateCounter.new()
    DriverServer.implementation_register(REGISTERED_LATER, implementation)
    assert_same(DriverServer.driver_get_implementation(driver), implementation, "registered, it thinks for it")
    DriverServer.implementation_unregister(REGISTERED_LATER)
    assert_null(DriverServer.driver_get_implementation(driver), "unregistered, nothing thinks for it")
    DriverServer.driver_attach_implementation(driver, &"")
    assert_does_not_have(DriverServer.driver_get_rids(), driver, "an empty name makes it no driver")


## The person build_vehicle() seats at the controls of a vehicle of its own, driven by the implementation
func _create_driver(implementation:DriverImplementation) -> RID:
    var driver:RID = get_vehicle_driver(build_vehicle("AIDriverTest", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD).get_rid())
    attach_driver_implementation(driver, implementation)
    return driver


class UpdateCounter extends DriverImplementation:
    var updates:int = 0

    func _update(_driver:RID) -> void:
        updates += 1
