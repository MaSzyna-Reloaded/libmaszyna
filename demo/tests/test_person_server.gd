extends MaszynaGutTest

## PersonServer - a person is a handle and nothing more; whatever a person is to another server (a
## seat in a cabin, a driver's implementation) goes away when the person is freed.

var _persons:Array[RID] = []
var _vehicles:Array[RID] = []


func after_each() -> void:
    for person:RID in _persons:
        PersonServer.person_free(person)
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    _persons.clear()
    _vehicles.clear()


func _create_person() -> RID:
    var person:RID = PersonServer.person_create()
    _persons.append(person)
    return person


## A vehicle with one cabin, freed with the vehicle
func _create_cabin() -> RID:
    var vehicle:RID = VehicleServer.vehicle_create()
    _vehicles.append(vehicle)
    var cabin:RID = VehicleServer.cabin_create()
    VehicleServer.vehicle_cabin_attach(vehicle, cabin)
    return cabin


func test_a_person_exists_until_freed() -> void:
    var person:RID = _create_person()

    assert_true(person.is_valid())
    assert_true(PersonServer.person_exists(person))
    PersonServer.person_free(person)
    assert_false(PersonServer.person_exists(person), "freed, it is gone")


func test_every_person_is_another_handle() -> void:
    var first:RID = _create_person()
    var second:RID = _create_person()

    assert_ne(first, second)


func test_freeing_a_person_is_announced_once() -> void:
    var person:RID = _create_person()
    watch_signals(PersonServer)

    PersonServer.person_free(person)
    PersonServer.person_free(person)

    assert_signal_emitted_with_parameters(PersonServer, "person_freed", [person])
    assert_signal_emit_count(PersonServer, "person_freed", 1, "a person already gone is not freed again")


func test_a_freed_person_leaves_its_cabin() -> void:
    var cabin:RID = _create_cabin()
    var person:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER), OK)
    watch_signals(VehicleServer)

    PersonServer.person_free(person)

    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_left", [cabin, person])
    assert_eq(VehicleServer.person_get_cabin(person), RID(), "no seat is kept for it")
    assert_false(VehicleServer.cabin_has_person_role(cabin, VehiclePersonRole.VEHICLE_PERSON_ROLE_ANY),
            "the cabin is empty")


func test_a_freed_person_is_no_driver() -> void:
    var cabin:RID = _create_cabin()
    var person:RID = _create_person()
    var implementation:IdleImplementation = IdleImplementation.new()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER), OK)
    attach_driver_implementation(person, implementation)
    assert_has(DriverServer.driver_get_rids(), person)
    assert_eq(DriverServer.vehicle_get_driver(VehicleServer.cabin_get_vehicle(cabin)), person)
    watch_signals(DriverServer)

    PersonServer.person_free(person)

    assert_signal_emitted_with_parameters(DriverServer, "driver_freed", [person])
    assert_does_not_have(DriverServer.driver_get_rids(), person)
    var detached:Array[RID] = [person]
    assert_eq(implementation.detached, detached, "its implementation learns it drives no more")
    assert_false(DriverServer.vehicle_get_driver(VehicleServer.cabin_get_vehicle(cabin)).is_valid(),
            "the vehicle has no driver")


class IdleImplementation extends DriverImplementation:
    var detached:Array[RID] = []

    func _driver_detached(driver:RID) -> void:
        detached.append(driver)
