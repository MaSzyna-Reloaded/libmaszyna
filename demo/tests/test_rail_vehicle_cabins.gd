extends MaszynaGutTest

## RailVehicleServer's cabins - the railway's names for a vehicle's cabins (front, rear, machine
## room), the driver's cabin the vehicle answers to (the original's occupied cab, CabOccupied), the
## cabin leading the way, and the player and the AI driver taking turns at the controls.

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const DRIVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER
const OBSERVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER
const ANY:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_ANY
## Enough frames for a built vehicle to take its controller
const SETTLE_FRAMES:int = 4

var _persons:Array[RID] = []


func after_each() -> void:
    PlayerServer.player_leave_vehicle()
    for person:RID in _persons:
        PersonServer.person_free(person)
    _persons.clear()


func _create_person() -> RID:
    var person:RID = PersonServer.person_create()
    _persons.append(person)
    return person


## A rail vehicle nobody sits in, with a front and a rear cabin
func _build_vehicle(train_id:String) -> RID:
    var vehicle:RID = build_vehicle(train_id, SM42).get_rid()
    if not RailVehicleServer.vehicle_get_front_cabin(vehicle).is_valid():
        RailVehicleServer.vehicle_add_front_cabin(vehicle)
        RailVehicleServer.vehicle_add_rear_cabin(vehicle)
    await wait_idle_frames(SETTLE_FRAMES)
    return vehicle


func test_a_cabin_has_its_kind() -> void:
    var vehicle:RID = await _build_vehicle("CabinKindTest")
    assert_false(RailVehicleServer.vehicle_get_machine_room(vehicle).is_valid(), "no machine room until added")

    var machine_room:RID = RailVehicleServer.vehicle_add_machine_room(vehicle)

    assert_eq(RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_front_cabin(vehicle)),
            RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT)
    assert_eq(RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_rear_cabin(vehicle)),
            RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    assert_eq(RailVehicleServer.vehicle_get_machine_room(vehicle), machine_room)
    assert_eq(RailVehicleServer.cabin_get_kind(machine_room), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE)
    assert_eq(VehicleServer.cabin_get_vehicle(machine_room), vehicle, "a cabin of VehicleServer's")
    assert_eq(RailVehicleServer.cabin_get_kind(RID()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_NONE)


func test_a_vehicle_has_one_cabin_of_each_kind() -> void:
    var vehicle:RID = await _build_vehicle("CabinOneOfEachTest")
    var front:RID = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    var count:int = VehicleServer.vehicle_get_cabin_count(vehicle)

    assert_false(RailVehicleServer.vehicle_add_front_cabin(vehicle).is_valid())

    assert_engine_error("The vehicle has that cabin already")
    assert_eq(RailVehicleServer.vehicle_get_front_cabin(vehicle), front)
    assert_eq(VehicleServer.vehicle_get_cabin_count(vehicle), count)


func test_a_person_enters_and_moves_between_cabins_by_their_kind() -> void:
    var vehicle:RID = await _build_vehicle("CabinEnterTest")
    var person:RID = _create_person()

    assert_eq(RailVehicleServer.person_enter_machine_room(person, vehicle, OBSERVER), ERR_DOES_NOT_EXIST)
    assert_eq(RailVehicleServer.person_enter_front_cabin(person, vehicle, OBSERVER), OK)
    assert_eq(VehicleServer.person_get_cabin(person), RailVehicleServer.vehicle_get_front_cabin(vehicle))
    assert_true(RailVehicleServer.vehicle_front_cabin_has_person_role(vehicle, OBSERVER))
    assert_eq(RailVehicleServer.vehicle_front_cabin_list_persons(vehicle, ANY).size(), 1)
    assert_eq(RailVehicleServer.person_move_to_machine_room(person), ERR_DOES_NOT_EXIST)

    assert_eq(RailVehicleServer.person_move_to_rear_cabin(person), OK)
    assert_eq(VehicleServer.person_get_cabin(person), RailVehicleServer.vehicle_get_rear_cabin(vehicle))
    assert_false(RailVehicleServer.vehicle_front_cabin_has_person_role(vehicle, ANY))
    assert_true(RailVehicleServer.vehicle_rear_cabin_has_person_role(vehicle, OBSERVER))

    RailVehicleServer.vehicle_add_machine_room(vehicle)
    assert_eq(RailVehicleServer.person_move_to_machine_room(person), OK)
    assert_eq(VehicleServer.person_get_cabin(person), RailVehicleServer.vehicle_get_machine_room(vehicle))
    assert_true(RailVehicleServer.vehicle_machine_room_has_person_role(vehicle, OBSERVER))
    assert_eq(RailVehicleServer.person_move_to_front_cabin(person), OK)
    assert_eq(VehicleServer.person_get_cabin(person), RailVehicleServer.vehicle_get_front_cabin(vehicle))


func test_a_missing_kind_cannot_be_entered() -> void:
    var vehicle:RID = await _build_vehicle("CabinMissingTest")
    var other:RID = await _build_vehicle("CabinMissingOtherTest")
    var person:RID = _create_person()
    var on_foot:RID = _create_person()

    assert_eq(RailVehicleServer.person_enter_machine_room(person, vehicle, DRIVER), ERR_DOES_NOT_EXIST)
    assert_eq(VehicleServer.person_get_cabin(person), RID())
    assert_eq(RailVehicleServer.person_move_to_front_cabin(on_foot), ERR_DOES_NOT_EXIST, "on foot, no cabin to go to")
    assert_eq(RailVehicleServer.person_enter_front_cabin(person, other, DRIVER), OK)
    assert_eq(RailVehicleServer.person_enter_rear_cabin(person, vehicle, DRIVER), ERR_ALREADY_IN_USE,
            "a person aboard another vehicle stays there")


func test_the_driver_cabin_is_where_somebody_drives() -> void:
    var vehicle:RID = await _build_vehicle("DriverCabinTest")
    var front:RID = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    var rear:RID = RailVehicleServer.vehicle_get_rear_cabin(vehicle)
    var driver:RID = _create_person()
    var observer:RID = _create_person()
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), RID(), "nobody drives a new vehicle")
    watch_signals(RailVehicleServer)

    assert_eq(RailVehicleServer.person_enter_front_cabin(observer, vehicle, OBSERVER), OK)
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), RID(), "one riding along drives nothing")
    assert_signal_not_emitted(RailVehicleServer, "vehicle_driver_cabin_changed")

    assert_eq(RailVehicleServer.person_enter_rear_cabin(driver, vehicle, DRIVER), OK)
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), rear)
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed", [vehicle, rear], 0)

    assert_eq(RailVehicleServer.person_move_to_front_cabin(driver), OK)
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), front, "the driver's cabin goes with the driver")
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed", [vehicle, front], 1)

    VehicleServer.cabin_person_leave(front, driver)
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), RID(), "the driver gone, nobody drives")
    assert_signal_emitted_with_parameters(RailVehicleServer, "vehicle_driver_cabin_changed", [vehicle, RID()], 2)


func test_a_driver_sitting_down_in_another_cab_takes_nothing_over() -> void:
    var vehicle:RID = await _build_vehicle("DriverCabinKeptTest")
    var front:RID = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    var rear:RID = RailVehicleServer.vehicle_get_rear_cabin(vehicle)
    var first_driver:RID = _create_person()
    var second_driver:RID = _create_person()

    assert_eq(VehicleServer.cabin_person_enter(rear, first_driver, DRIVER), OK)
    assert_eq(VehicleServer.cabin_person_enter(front, second_driver, DRIVER), OK)

    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), rear, "the first driver keeps it")
    VehicleServer.cabin_person_leave(rear, first_driver)
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), front, "until it is gone")


func test_a_driver_keeps_the_vehicle_going_over_to_another_cabin() -> void:
    var vehicle:RID = await _build_vehicle("DriverCabinMovesTest")
    var machine_room:RID = RailVehicleServer.vehicle_add_machine_room(vehicle)
    var first_driver:RID = _create_person()
    var second_driver:RID = _create_person()
    assert_eq(RailVehicleServer.person_enter_front_cabin(first_driver, vehicle, DRIVER), OK)
    assert_eq(RailVehicleServer.person_enter_rear_cabin(second_driver, vehicle, DRIVER), OK)

    assert_eq(VehicleServer.cabin_person_move(first_driver, machine_room), OK)

    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), machine_room, "the driver, not the rear cab")


func test_with_its_driver_gone_the_vehicle_answers_to_the_driver_of_the_cab_switched_on() -> void:
    var vehicle:RID = await _build_vehicle("DriverCabinActiveTest")
    var machine_room:RID = RailVehicleServer.vehicle_add_machine_room(vehicle)
    var first_driver:RID = _create_person()
    var rear_driver:RID = _create_person()
    var front_driver:RID = _create_person()
    assert_eq(RailVehicleServer.person_enter_rear_cabin(first_driver, vehicle, DRIVER), OK)
    VehicleServer.vehicle_send_command(vehicle, "cab_activation", true)
    assert_eq(VehicleServer.cabin_person_move(first_driver, machine_room), OK)
    assert_eq(RailVehicleServer.person_enter_rear_cabin(rear_driver, vehicle, DRIVER), OK)
    assert_eq(RailVehicleServer.person_enter_front_cabin(front_driver, vehicle, DRIVER), OK)
    assert_eq(int(VehicleServer.vehicle_dump_state(vehicle)["cabin"]), -1, "the rear cab is switched on")

    VehicleServer.cabin_person_leave(machine_room, first_driver)

    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), RailVehicleServer.vehicle_get_rear_cabin(vehicle),
            "the rear cab's driver, not the front one's")


func test_the_leading_cabin_is_the_way_the_reverser_points() -> void:
    var vehicle:RID = await _build_vehicle("LeadingCabinTest")
    var driver:RID = _create_person()
    assert_eq(RailVehicleServer.vehicle_get_leading_cabin(vehicle), RailVehicleServer.vehicle_get_front_cabin(vehicle),
            "standing with the reverser at neutral, the front leads")
    assert_eq(RailVehicleServer.person_enter_front_cabin(driver, vehicle, DRIVER), OK)
    VehicleServer.vehicle_send_command(vehicle, "cab_activation", true)

    VehicleServer.vehicle_send_command(vehicle, "direction_decrease")

    assert_eq(int(VehicleServer.vehicle_dump_state(vehicle)["direction"]), VehicleController.DIRECTION_BACKWARD)
    assert_eq(RailVehicleServer.vehicle_get_leading_cabin(vehicle), RailVehicleServer.vehicle_get_rear_cabin(vehicle))


func test_taking_over_seats_the_player_at_the_controls_and_the_driver_rides_along() -> void:
    var vehicle:RID = await _build_vehicle("TakeOverTest")
    var front:RID = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    var driver:RID = _create_person()
    var player:RID = PlayerServer.player_get_person()
    assert_eq(RailVehicleServer.person_enter_front_cabin(driver, vehicle, DRIVER), OK)

    PlayerServer.player_take_over_vehicle(vehicle)

    assert_eq(PlayerServer.player_get_vehicle(), vehicle)
    assert_eq(VehicleServer.person_get_cabin(player), front, "in the cabin driven from")
    assert_eq(VehicleServer.person_get_role(player), DRIVER)
    assert_eq(VehicleServer.person_get_cabin(driver), front, "the driver stays in its cabin")
    assert_eq(VehicleServer.person_get_role(driver), OBSERVER, "and rides along")
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), front)


func test_a_vehicle_nobody_drives_is_taken_over_in_its_leading_cabin() -> void:
    var vehicle:RID = await _build_vehicle("TakeOverEmptyTest")
    var player:RID = PlayerServer.player_get_person()

    PlayerServer.player_take_over_vehicle(vehicle)

    assert_eq(VehicleServer.person_get_cabin(player), RailVehicleServer.vehicle_get_leading_cabin(vehicle))
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), VehicleServer.person_get_cabin(player))


func test_handing_over_gives_the_ai_driver_the_players_cabin_and_leaving_hands_it_back() -> void:
    var vehicle:RID = await _build_vehicle("HandOverTest")
    var front:RID = RailVehicleServer.vehicle_get_front_cabin(vehicle)
    var driver:RID = _create_person()
    var player:RID = PlayerServer.player_get_person()
    attach_driver_implementation(driver, IdleImplementation.new())
    assert_eq(RailVehicleServer.person_enter_rear_cabin(driver, vehicle, DRIVER), OK)
    assert_true(DriverServer.vehicle_is_control_active(vehicle), "the AI drives")
    PlayerServer.player_take_over_vehicle(vehicle)
    assert_false(DriverServer.vehicle_is_control_active(vehicle), "the player drives")
    assert_eq(DriverServer.vehicle_get_driver(vehicle), driver, "the AI is aboard still")
    # the player walks over to the other end, at the controls
    assert_eq(RailVehicleServer.person_move_to_front_cabin(player), OK)

    PlayerServer.player_hand_over_vehicle()

    assert_eq(VehicleServer.person_get_cabin(driver), front, "the AI sits down in the player's cabin")
    assert_eq(VehicleServer.person_get_role(driver), DRIVER)
    assert_eq(VehicleServer.person_get_role(player), OBSERVER, "the player rides along")
    assert_true(DriverServer.vehicle_is_control_active(vehicle))
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), front)
    assert_eq(PlayerServer.player_get_vehicle(), vehicle, "the player stays aboard")

    PlayerServer.player_take_over_vehicle(vehicle)
    assert_eq(VehicleServer.person_get_role(player), DRIVER, "taken back")
    assert_eq(VehicleServer.person_get_role(driver), OBSERVER)

    PlayerServer.player_leave_vehicle()

    assert_eq(VehicleServer.person_get_cabin(player), RID(), "the player is on foot")
    assert_eq(PlayerServer.player_get_vehicle(), RID())
    assert_eq(VehicleServer.person_get_role(driver), DRIVER, "the AI drives again")
    assert_true(DriverServer.vehicle_is_control_active(vehicle))


func test_a_drivers_cab_change_leaves_the_controls_at_rest() -> void:
    var vehicle:RID = await _build_vehicle("CabChangeResetTest")
    var driver:RID = _create_person()
    assert_eq(RailVehicleServer.person_enter_front_cabin(driver, vehicle, DRIVER), OK)
    VehicleServer.vehicle_send_command(vehicle, "cab_activation", true)
    var at_rest:int = int(VehicleServer.vehicle_dump_state(vehicle)["master_controller_position"])
    VehicleServer.vehicle_send_command(vehicle, "main_controller_increase", 1)
    assert_eq(int(VehicleServer.vehicle_dump_state(vehicle)["master_controller_position"]), at_rest + 1,
            "the controller off its rest")

    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD)

    assert_eq(VehicleServer.person_get_cabin(driver), RailVehicleServer.vehicle_get_rear_cabin(vehicle))
    assert_eq(RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(vehicle)),
            RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR, "the vehicle is driven from the rear")
    assert_eq(int(VehicleServer.vehicle_dump_state(vehicle)["master_controller_position"]), at_rest,
            "the controller back at its no-power position (Mover.cpp:735-749)")


func test_one_riding_along_changes_cabins_without_touching_the_controls() -> void:
    var vehicle:RID = await _build_vehicle("CabChangeObserverTest")
    var driver:RID = _create_person()
    var observer:RID = _create_person()
    assert_eq(RailVehicleServer.person_enter_front_cabin(driver, vehicle, DRIVER), OK)
    assert_eq(RailVehicleServer.person_enter_front_cabin(observer, vehicle, OBSERVER), OK)
    VehicleServer.vehicle_send_command(vehicle, "cab_activation", true)
    VehicleServer.vehicle_send_command(vehicle, "main_controller_increase", 1)
    var position:int = int(VehicleServer.vehicle_dump_state(vehicle)["master_controller_position"])

    RailVehicleServer.person_change_cabin(observer, RailVehicleServer.CABIN_CHANGE_BACKWARD)

    assert_eq(VehicleServer.person_get_cabin(observer), RailVehicleServer.vehicle_get_rear_cabin(vehicle))
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(vehicle), RailVehicleServer.vehicle_get_front_cabin(vehicle))
    assert_eq(int(VehicleServer.vehicle_dump_state(vehicle)["master_controller_position"]), position)


func test_a_cab_change_stops_at_the_end_of_the_vehicle() -> void:
    var vehicle:RID = await _build_vehicle("CabChangeEndTest")
    var person:RID = _create_person()
    assert_eq(RailVehicleServer.person_enter_front_cabin(person, vehicle, OBSERVER), OK)

    RailVehicleServer.person_change_cabin(person, RailVehicleServer.CABIN_CHANGE_FORWARD)

    assert_eq(VehicleServer.person_get_cabin(person), RailVehicleServer.vehicle_get_front_cabin(vehicle),
            "nothing ahead of the front cabin")


class IdleImplementation extends DriverImplementation:
    pass
