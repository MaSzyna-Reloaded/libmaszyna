extends MaszynaGutTest

## RailVehicleLoad's exchange at a platform (TDynamicObject::LoadExchange(), update_exchange(),
## DynObj.cpp:2813-2960): the vehicle opens its doors at the platform's side, exchanges a second's
## worth per open side and gives the exchange up once moving.

const MAX_LOAD:float = 100.0
## Passengers a second through one open side
const EXCHANGE_SPEED:float = 5.0
const BOARDING:float = 20.0
## Both sides are served twice as fast as one (DynObj.cpp:2848)
const BOTH_SIDES:float = 2.0
## Faster than the exchange's limit of 2 km/h [m/s]
const MOVING_VELOCITY:float = 5.0
## Steps a vehicle just built takes to stand ready: its node takes the controller within the
## frames, the vehicle its configuration on its first step
const SETTLE_TICKS:int = 2

const TRACK_NAME:String = "load_exchange_test"
const TRACK_LENGTH:float = 400.0
const TRACK_OFFSET:float = 100.0

var _track:RID
var _vehicle:RailVehicle3D
var _load:RailVehicleLoad


func after_each() -> void:
    free_rail_vehicle(_vehicle)
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func _build(initial_velocity:float = 0.0) -> RID:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)
    _vehicle = build_passenger_car("LoadExchangeTest", TRACK_NAME, TRACK_OFFSET, MAX_LOAD, EXCHANGE_SPEED, initial_velocity)
    await step(SETTLE_TICKS)
    var vehicle:RID = _vehicle.get_rid()
    # the vehicle's own component, not the description it was built from
    _load = VehicleServer.vehicle_component_get(vehicle, VehicleComponentType.COMPONENT_LOAD)
    return vehicle


func test_an_empty_car_takes_the_load_and_the_exchange_counts_both_sides() -> void:
    await _build()

    _load.load_add(BOARDING, RailVehicleLoad.PLATFORM_SIDE_BOTH, PASSENGERS)

    assert_eq(_load.get_load_name(), PASSENGERS)
    assert_almost_eq(_load.get_load_exchange_time(), BOARDING / EXCHANGE_SPEED / BOTH_SIDES, 0.001,
            "no door open yet: the nominal speed of both sides")


func test_the_car_opens_its_doors_and_boards_then_empties_and_loses_the_load() -> void:
    var vehicle:RID = await _build()

    watch_signals(_load)
    _load.load_add(BOARDING, RailVehicleLoad.PLATFORM_SIDE_LEFT, PASSENGERS)
    if not await wait_simulated_until(func() -> bool: return _load.get_load_exchange_speed() > 0,
            _doors_opening(vehicle) + TICK, "the doors open at the platform"):
        return
    var state:Dictionary = VehicleServer.vehicle_dump_state(vehicle)
    assert_true(state.get("doors_left_open", false), "opened at the platform")
    assert_false(state.get("doors_right_open", true), "and only there")
    # a second's worth through the one open side at a time (update_exchange(), DynObj.cpp:2872-2960)
    if not await wait_simulated_until(func() -> bool: return _load.get_load_exchange_time() == 0.0,
            BOARDING / EXCHANGE_SPEED + TICK, "the boarding"):
        return

    assert_almost_eq(_load.get_load_amount(), BOARDING, 0.001)
    assert_signal_emit_count(_load, "load_exchange_finished", 1, "the exchange reports it is over, once")

    _load.load_remove(BOARDING, RailVehicleLoad.PLATFORM_SIDE_LEFT)
    # the doors the passengers may have closed once done open again, then the exchange
    if not await wait_simulated_until(func() -> bool: return _load.get_load_exchange_time() == 0.0,
            _doors_closing(vehicle) + _doors_opening(vehicle) + BOARDING / EXCHANGE_SPEED + TICK,
            "the alighting"):
        return

    assert_almost_eq(_load.get_load_amount(), 0.0, 0.001)
    assert_eq(_load.get_load_name(), "", "emptied, it carries nothing")


func test_a_moving_car_gives_the_exchange_up() -> void:
    await _build(MOVING_VELOCITY)

    _load.load_add(BOARDING, RailVehicleLoad.PLATFORM_SIDE_BOTH, PASSENGERS)
    await step(SETTLE_TICKS)

    assert_eq(_load.get_load_exchange_time(), 0.0)
    assert_eq(_load.get_load_amount(), 0.0, "nobody got on")


## RailVehicleServer's operations under the vehicle's handle, and the vehicle's commands that come
## to them
func test_the_server_and_the_commands_exchange_the_load() -> void:
    var vehicle:RID = await _build()

    RailVehicleServer.load_add(vehicle, BOARDING, RailVehicleLoad.PLATFORM_SIDE_BOTH)
    assert_eq(_load.get_load_name(), PASSENGERS, "the first load the car accepts")
    assert_almost_eq(RailVehicleServer.load_get_exchange_time(vehicle), BOARDING / EXCHANGE_SPEED / BOTH_SIDES, 0.001)
    if not await wait_simulated_until(func() -> bool: return RailVehicleServer.load_get_exchange_time(vehicle) == 0.0,
            _doors_opening(vehicle) + BOARDING / EXCHANGE_SPEED / BOTH_SIDES + TICK, "the boarding"):
        return

    VehicleServer.vehicle_send_command(vehicle, "load_remove", BOARDING, RailVehicleLoad.PLATFORM_SIDE_RIGHT)
    assert_gt(RailVehicleServer.load_get_exchange_time(vehicle), 0.0, "the command is the server's operation")
    # the right doors, closed once the boarding was done, open again
    if not await wait_simulated_until(func() -> bool: return RailVehicleServer.load_get_exchange_time(vehicle) == 0.0,
            _doors_closing(vehicle) + _doors_opening(vehicle) + BOARDING / EXCHANGE_SPEED + TICK,
            "the alighting"):
        return
    assert_almost_eq(_load.get_load_amount(), 0.0, 0.001)


## The doors' opening: their delay, then their shift at their speed (update_doors(), Mover.cpp:8000-8008)
func _doors_opening(vehicle:RID) -> float:
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    return doors.open_delay + doors.max_shift / doors.open_speed


## The doors' closing, the same way (Mover.cpp:8010-8017)
func _doors_closing(vehicle:RID) -> float:
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
            vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    return doors.close_delay + doors.max_shift / doors.close_speed
