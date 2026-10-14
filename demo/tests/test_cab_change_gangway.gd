extends MaszynaGutTest

## A cab change past the end of a vehicle goes through the gangway into the next one
## (OnCommand_cabchangeforward/backward, Train.cpp:8291-8352; TTrain::MoveToVehicle(),
## Train.cpp:10879) - RailVehicleServer.person_change_cabin().

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const DRIVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER
const OBSERVER:VehiclePersonRole.Role = VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER
const GANGWAY:int = RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_GANGWAY
## Enough frames for a built vehicle to take its controller
const SETTLE_FRAMES:int = 4

var first:RID
var second:RID
var driver:RID


func before_each() -> void:
    first = build_vehicle("GangwayFirst", SM42, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD).get_rid()
    second = build_vehicle("GangwaySecond", SM42).get_rid()
    driver = get_vehicle_driver(first)
    await wait_idle_frames(SETTLE_FRAMES)


func after_each() -> void:
    PlayerServer.player_leave_vehicle()


func _couple(second_end:RailVehicleController.CouplerEnd, coupling:int) -> void:
    RailVehicleServer.vehicle_couple(first, RailVehicleController.COUPLER_END_REAR, second, second_end, coupling)


func test_past_the_rear_cab_the_driver_goes_into_the_cab_of_the_next_vehicle_facing_it() -> void:
    _couple(RailVehicleController.COUPLER_END_FRONT, GANGWAY)

    assert_eq(RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD), OK)
    assert_eq(VehicleServer.person_get_cabin(driver), RailVehicleServer.vehicle_get_rear_cabin(first), "its own rear cab first")
    assert_eq(RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD), OK)

    assert_eq(VehicleServer.person_get_cabin(driver), RailVehicleServer.vehicle_get_front_cabin(second))
    assert_false(RailVehicleServer.vehicle_get_driver_cabin(first).is_valid(), "the vehicle left has no driver")
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(second), RailVehicleServer.vehicle_get_front_cabin(second))


func test_a_neighbour_standing_the_other_way_round_is_entered_by_its_rear_cab() -> void:
    _couple(RailVehicleController.COUPLER_END_REAR, GANGWAY)
    RailVehicleServer.person_move_to_rear_cabin(driver)

    assert_eq(RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD), OK)

    assert_eq(VehicleServer.person_get_cabin(driver), RailVehicleServer.vehicle_get_rear_cabin(second))


func test_without_a_gangway_the_end_of_the_vehicle_stops_the_change() -> void:
    _couple(RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)
    RailVehicleServer.person_move_to_rear_cabin(driver)

    assert_eq(RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD), ERR_UNAVAILABLE)

    assert_eq(VehicleServer.person_get_cabin(driver), RailVehicleServer.vehicle_get_rear_cabin(first))


func test_whoever_drove_the_vehicle_entered_rides_along() -> void:
    _couple(RailVehicleController.COUPLER_END_FRONT, GANGWAY)
    var other:RID = PersonServer.person_create()
    RailVehicleServer.person_enter_rear_cabin(other, second, DRIVER)
    RailVehicleServer.person_move_to_rear_cabin(driver)

    RailVehicleServer.person_change_cabin(driver, RailVehicleServer.CABIN_CHANGE_BACKWARD)

    assert_eq(VehicleServer.person_get_role(other), OBSERVER)
    assert_eq(RailVehicleServer.vehicle_get_driver_cabin(second), VehicleServer.person_get_cabin(driver))
    PersonServer.person_free(other)


func test_the_player_goes_along_into_the_next_vehicle_with_its_driver() -> void:
    _couple(RailVehicleController.COUPLER_END_FRONT, GANGWAY)
    attach_driver_implementation(driver, DriverImplementation.new())
    var other:RID = PersonServer.person_create()
    RailVehicleServer.person_enter_rear_cabin(other, second, DRIVER)
    attach_driver_implementation(other, DriverImplementation.new())
    PlayerServer.player_take_over_vehicle(first)
    var player:RID = PlayerServer.player_get_person()
    RailVehicleServer.person_move_to_rear_cabin(player)
    watch_signals(PlayerServer)

    RailVehicleServer.person_change_cabin(player, RailVehicleServer.CABIN_CHANGE_BACKWARD)

    assert_eq(PlayerServer.player_get_vehicle(), second, "the player is in the vehicle entered")
    assert_signal_emitted_with_parameters(PlayerServer, "player_vehicle_changed", [second, first])
    assert_eq(DriverServer.vehicle_get_driver(second), driver, "its driver came along")
    assert_false(VehicleServer.person_get_vehicle(other).is_valid(), "the one there got out")
    PersonServer.person_free(other)
