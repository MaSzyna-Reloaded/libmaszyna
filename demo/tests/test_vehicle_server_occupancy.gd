extends MaszynaGutTest

## VehicleServer's cabins and who sits in them - a vehicle of any kind, no controller needed: a
## person enters, leaves, changes role and moves between the cabins of one vehicle; a cabin has at
## most one DRIVER; every change is announced.

const DRIVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER
const OBSERVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER
const ANY:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_ANY

var _persons:Array[RID] = []
var _cabins:Array[RID] = []
var _vehicles:Array[RID] = []


func after_each() -> void:
    for person:RID in _persons:
        PersonServer.person_free(person)
    for cabin:RID in _cabins:
        VehicleServer.cabin_free(cabin)
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    _persons.clear()
    _cabins.clear()
    _vehicles.clear()


func _create_person() -> RID:
    var person:RID = PersonServer.person_create()
    _persons.append(person)
    return person


func _create_vehicle() -> RID:
    var vehicle:RID = VehicleServer.vehicle_create()
    _vehicles.append(vehicle)
    return vehicle


func _create_cabin() -> RID:
    var cabin:RID = VehicleServer.cabin_create()
    _cabins.append(cabin)
    return cabin


func _attach_cabin(vehicle:RID) -> RID:
    var cabin:RID = _create_cabin()
    VehicleServer.vehicle_cabin_attach(vehicle, cabin)
    return cabin


func test_cabins_are_attached_to_a_vehicle_and_detached() -> void:
    var vehicle:RID = _create_vehicle()
    assert_eq(VehicleServer.vehicle_get_cabin_count(vehicle), 0, "a new vehicle has no cabin")

    var first:RID = _attach_cabin(vehicle)
    var second:RID = _attach_cabin(vehicle)

    var cabins:Array[RID] = [first, second]
    assert_eq(VehicleServer.vehicle_get_cabin_count(vehicle), cabins.size())
    assert_eq(VehicleServer.vehicle_get_cabins(vehicle), cabins, "in the order attached")
    assert_eq(VehicleServer.cabin_get_vehicle(first), vehicle)
    VehicleServer.vehicle_cabin_detach(vehicle, first)
    var left:Array[RID] = [second]
    assert_eq(VehicleServer.vehicle_get_cabins(vehicle), left)
    assert_eq(VehicleServer.cabin_get_vehicle(first), RID(), "a detached cabin belongs to no vehicle")


func test_a_cabin_of_no_vehicle_cannot_be_entered() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _create_cabin()
    var person:RID = _create_person()

    assert_eq(VehicleServer.vehicle_get_cabin_count(vehicle), 0)
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, DRIVER), ERR_INVALID_PARAMETER,
            "a vehicle without cabins has nowhere to sit")
    assert_eq(VehicleServer.person_get_cabin(person), RID())
    assert_false(VehicleServer.vehicle_has_person_role(vehicle, ANY))


func test_a_person_enters_and_leaves() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var person:RID = _create_person()
    watch_signals(VehicleServer)

    assert_eq(VehicleServer.cabin_person_enter(cabin, person, OBSERVER), OK)
    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_entered", [cabin, person, OBSERVER])
    assert_eq(VehicleServer.person_get_cabin(person), cabin)
    assert_eq(VehicleServer.person_get_vehicle(person), vehicle)
    assert_eq(VehicleServer.person_get_role(person), OBSERVER)

    VehicleServer.cabin_person_leave(cabin, person)
    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_left", [cabin, person])
    assert_eq(VehicleServer.person_get_cabin(person), RID())
    assert_eq(VehicleServer.person_get_vehicle(person), RID())
    assert_eq(VehicleServer.person_get_role(person), ANY, "on foot, a person has no role")


func test_any_is_no_role_to_take() -> void:
    var cabin:RID = _attach_cabin(_create_vehicle())
    var person:RID = _create_person()

    assert_eq(VehicleServer.cabin_person_enter(cabin, person, ANY), ERR_INVALID_PARAMETER)
    assert_eq(VehicleServer.person_get_cabin(person), RID())
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, OBSERVER), OK)
    assert_eq(VehicleServer.cabin_person_change_role(cabin, person, ANY), ERR_INVALID_PARAMETER)
    assert_eq(VehicleServer.person_get_role(person), OBSERVER)


func test_a_person_aboard_cannot_enter_again() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var other_cabin:RID = _attach_cabin(vehicle)
    var person:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, OBSERVER), OK)

    assert_eq(VehicleServer.cabin_person_enter(cabin, person, OBSERVER), ERR_ALREADY_IN_USE)
    assert_eq(VehicleServer.cabin_person_enter(other_cabin, person, OBSERVER), ERR_ALREADY_IN_USE,
            "nor sit in two cabins")
    assert_eq(VehicleServer.person_get_cabin(person), cabin)


func test_a_cabin_has_one_driver() -> void:
    var cabin:RID = _attach_cabin(_create_vehicle())
    var driver:RID = _create_person()
    var second:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, driver, DRIVER), OK)

    assert_eq(VehicleServer.cabin_person_enter(cabin, second, DRIVER), ERR_UNAVAILABLE)
    assert_eq(VehicleServer.person_get_cabin(second), RID(), "refused, it is not aboard")
    assert_eq(VehicleServer.cabin_person_enter(cabin, second, OBSERVER), OK, "it may ride along")
    assert_eq(VehicleServer.cabin_person_change_role(cabin, second, DRIVER), ERR_UNAVAILABLE)
    assert_eq(VehicleServer.person_get_role(second), OBSERVER)


func test_a_take_over_hands_the_controls_from_one_to_the_other() -> void:
    var cabin:RID = _attach_cabin(_create_vehicle())
    var driver:RID = _create_person()
    var taking_over:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, driver, DRIVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(cabin, taking_over, OBSERVER), OK)
    watch_signals(VehicleServer)

    assert_eq(VehicleServer.cabin_person_change_role(cabin, driver, OBSERVER), OK)
    assert_eq(VehicleServer.cabin_person_change_role(cabin, taking_over, DRIVER), OK)

    assert_eq(VehicleServer.person_get_role(driver), OBSERVER)
    assert_eq(VehicleServer.person_get_role(taking_over), DRIVER)
    assert_signal_emit_count(VehicleServer, "cabin_person_role_changed", 2)
    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_role_changed", [cabin, driver, OBSERVER], 0)
    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_role_changed", [cabin, taking_over, DRIVER], 1)


func test_the_same_role_again_is_no_change() -> void:
    var cabin:RID = _attach_cabin(_create_vehicle())
    var person:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, DRIVER), OK)
    watch_signals(VehicleServer)

    assert_eq(VehicleServer.cabin_person_change_role(cabin, person, DRIVER), OK)

    assert_signal_not_emitted(VehicleServer, "cabin_person_role_changed")


func test_a_role_is_changed_only_in_the_cabin_sat_in() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var other_cabin:RID = _attach_cabin(vehicle)
    var person:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, OBSERVER), OK)

    assert_eq(VehicleServer.cabin_person_change_role(other_cabin, person, DRIVER), ERR_INVALID_PARAMETER)
    assert_eq(VehicleServer.person_get_role(person), OBSERVER)


func test_a_person_moves_within_its_vehicle_with_its_role() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var other_cabin:RID = _attach_cabin(vehicle)
    var person:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, DRIVER), OK)
    watch_signals(VehicleServer)

    assert_eq(VehicleServer.cabin_person_move(person, other_cabin), OK)

    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_moved", [person, other_cabin, cabin])
    assert_signal_not_emitted(VehicleServer, "cabin_person_left", "a move is not leaving")
    assert_signal_not_emitted(VehicleServer, "cabin_person_entered", "nor entering")
    assert_eq(VehicleServer.person_get_cabin(person), other_cabin)
    assert_eq(VehicleServer.person_get_role(person), DRIVER, "the role goes along")
    assert_false(VehicleServer.cabin_has_person_role(cabin, ANY), "the cabin left is empty")
    assert_eq(VehicleServer.cabin_person_move(person, other_cabin), OK, "where it sits already")
    assert_signal_emit_count(VehicleServer, "cabin_person_moved", 1, "is no move")


func test_a_driver_does_not_move_into_another_drivers_cabin() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var other_cabin:RID = _attach_cabin(vehicle)
    var driver:RID = _create_person()
    var other_driver:RID = _create_person()
    var observer:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, driver, DRIVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(other_cabin, other_driver, DRIVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(cabin, observer, OBSERVER), OK)

    assert_eq(VehicleServer.cabin_person_move(driver, other_cabin), ERR_UNAVAILABLE)
    assert_eq(VehicleServer.person_get_cabin(driver), cabin)
    assert_eq(VehicleServer.cabin_person_move(observer, other_cabin), OK, "one riding along goes over")


## A gangway leads into the next vehicle (RailVehicleServer.person_change_cabin())
func test_a_person_moves_to_another_vehicle_but_not_from_the_ground() -> void:
    var cabin:RID = _attach_cabin(_create_vehicle())
    var other_vehicles_cabin:RID = _attach_cabin(_create_vehicle())
    var person:RID = _create_person()
    var on_foot:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, OBSERVER), OK)

    assert_eq(VehicleServer.cabin_person_move(person, other_vehicles_cabin), OK)
    assert_eq(VehicleServer.person_get_cabin(person), other_vehicles_cabin)
    assert_eq(VehicleServer.cabin_person_move(on_foot, cabin), ERR_INVALID_PARAMETER, "entering is not a move")
    assert_eq(VehicleServer.person_get_cabin(on_foot), RID())


func test_the_persons_aboard_are_listed_by_role() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var other_cabin:RID = _attach_cabin(vehicle)
    var driver:RID = _create_person()
    var observer:RID = _create_person()
    var elsewhere:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, driver, DRIVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(cabin, observer, OBSERVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(other_cabin, elsewhere, OBSERVER), OK)

    var drivers:Array[VehiclePerson] = VehicleServer.cabin_list_persons(cabin, DRIVER)
    assert_eq(drivers.size(), 1)
    assert_eq(drivers[0].get_person(), driver)
    assert_eq(drivers[0].get_cabin(), cabin)
    assert_eq(drivers[0].get_role(), DRIVER)
    var in_cabin:Array[VehiclePerson] = VehicleServer.cabin_list_persons(cabin, ANY)
    var in_cabin_persons:Array[RID] = []
    for seated:VehiclePerson in in_cabin:
        in_cabin_persons.append(seated.get_person())
    assert_eq(in_cabin_persons.size(), 2)
    assert_has(in_cabin_persons, driver)
    assert_has(in_cabin_persons, observer)
    assert_eq(VehicleServer.cabin_list_persons(other_cabin, DRIVER).size(), 0)
    assert_eq(VehicleServer.vehicle_list_persons(vehicle, OBSERVER).size(), 2, "the vehicle's from every cabin")
    var aboard:Array[RID] = [driver, observer, elsewhere]
    assert_eq(VehicleServer.vehicle_list_persons(vehicle, ANY).size(), aboard.size())
    assert_true(VehicleServer.cabin_has_person_role(cabin, DRIVER))
    assert_false(VehicleServer.cabin_has_person_role(other_cabin, DRIVER))
    assert_true(VehicleServer.vehicle_has_person_role(vehicle, DRIVER))
    assert_true(VehicleServer.vehicle_has_person_role(vehicle, ANY))


func test_a_vehicle_with_nobody_aboard_has_no_role() -> void:
    var vehicle:RID = _create_vehicle()
    _attach_cabin(vehicle)

    assert_false(VehicleServer.vehicle_has_person_role(vehicle, ANY))
    assert_false(VehicleServer.vehicle_has_person_role(vehicle, DRIVER))
    assert_eq(VehicleServer.vehicle_list_persons(vehicle, ANY).size(), 0)


func test_a_detached_cabin_is_left_by_its_persons() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var person:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, person, DRIVER), OK)
    watch_signals(VehicleServer)

    VehicleServer.vehicle_cabin_detach(vehicle, cabin)

    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_left", [cabin, person])
    assert_eq(VehicleServer.person_get_cabin(person), RID())
    assert_false(VehicleServer.vehicle_has_person_role(vehicle, DRIVER))


func test_a_freed_vehicle_frees_its_cabins_and_its_persons_leave() -> void:
    var vehicle:RID = _create_vehicle()
    var cabin:RID = _attach_cabin(vehicle)
    var other_cabin:RID = _attach_cabin(vehicle)
    var driver:RID = _create_person()
    var observer:RID = _create_person()
    var latecomer:RID = _create_person()
    assert_eq(VehicleServer.cabin_person_enter(cabin, driver, DRIVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(other_cabin, observer, OBSERVER), OK)
    watch_signals(VehicleServer)

    VehicleServer.vehicle_free(vehicle)

    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_left", [cabin, driver], 0)
    assert_signal_emitted_with_parameters(VehicleServer, "cabin_person_left", [other_cabin, observer], 1)
    assert_signal_emitted_with_parameters(VehicleServer, "vehicle_freed", [vehicle])
    assert_eq(VehicleServer.person_get_cabin(driver), RID())
    assert_eq(VehicleServer.person_get_cabin(observer), RID())
    assert_eq(VehicleServer.cabin_get_vehicle(cabin), RID())
    assert_eq(VehicleServer.cabin_person_enter(cabin, latecomer, OBSERVER), ERR_INVALID_PARAMETER,
            "the cabin is gone with its vehicle")
