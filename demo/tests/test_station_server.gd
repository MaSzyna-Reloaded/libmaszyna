extends MaszynaGutTest

## StationServer: a train's dispatch at a stop, one step after another, each over when the car
## reports it - its passengers exchanged, its doors closed - on a car built in the test.

const TRACK_NAME:String = "station_server_test"
const TRACK_LENGTH:float = 400.0
const TRACK_OFFSET:float = 100.0
const CAPACITY:float = 100.0
## Passengers a second through one open side
const EXCHANGE_SPEED:float = 5.0
const BOARDING:float = 10.0
## Steps a vehicle just built takes to stand ready: its node takes the controller within the
## frames, the vehicle its configuration on its first step
const SETTLE_TICKS:int = 2

var _track:RID
var _car:RailVehicle3D
var _vehicle:RID
var _cars:Array[RID] = []


func before_each() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)
    _car = build_passenger_car("StationServerTest", TRACK_NAME, TRACK_OFFSET, CAPACITY, EXCHANGE_SPEED)
    await step(SETTLE_TICKS)
    _vehicle = _car.get_rid()
    _cars = [_vehicle]


func after_each() -> void:
    StationServer.dispatch_cancel(_vehicle)
    free_rail_vehicle(_car)
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func test_without_an_exchange_the_dispatch_waits_for_the_departure() -> void:
    StationServer.dispatch_start(_vehicle, _cars)
    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_WAIT_DEPARTURE)

    watch_signals(StationServer)
    StationServer.dispatch_depart(_vehicle)

    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_NONE, "the doors are closed")
    assert_signal_emitted_with_parameters(StationServer, "dispatch_finished", [_vehicle])


func test_the_train_waits_for_its_passengers_then_for_its_doors() -> void:
    RailVehicleServer.load_add(_vehicle, BOARDING, RailVehicleLoad.PLATFORM_SIDE_LEFT, PASSENGERS)
    StationServer.dispatch_start(_vehicle, _cars)
    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_EXCHANGE)
    assert_gt(StationServer.dispatch_get_exchange_time(_vehicle), 0.0)

    # the doors open, then a second's worth of passengers at a time (update_exchange(), DynObj.cpp:2872-2960)
    var exchanged:Callable = func() -> bool:
        return not StationServer.dispatch_get_step(_vehicle) == StationServer.DISPATCH_STEP_EXCHANGE
    if not await wait_simulated_until(exchanged, _doors_opening() + BOARDING / EXCHANGE_SPEED + TICK,
            "the passengers exchanged"):
        return

    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_WAIT_DEPARTURE,
            "exchanged, not let go yet")
    # the car's doors held open, whatever its passengers did
    VehicleServer.vehicle_send_command(_vehicle, "doors_left_local", true)
    var held_open:Callable = func() -> bool:
        return VehicleServer.vehicle_dump_state(_vehicle).get("doors_left_open", false)
    if not await wait_simulated_until(held_open, _doors_opening() + TICK, "the left doors held open"):
        return
    StationServer.dispatch_depart(_vehicle)
    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_CLOSE_DOORS)

    watch_signals(StationServer)
    VehicleServer.vehicle_send_command(_vehicle, "doors_left_local", false)
    # the doors' closing: their delay, then their shift at their speed (update_doors(), Mover.cpp:8010-8017)
    var finished:Callable = func() -> bool: return get_signal_emit_count(StationServer, "dispatch_finished") > 0
    if not await wait_simulated_until(finished,
            _doors().close_delay + _doors().max_shift / _doors().close_speed + TICK, "the dispatch finished"):
        return

    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_NONE)


func test_let_go_during_the_exchange_it_closes_the_doors_once_done() -> void:
    RailVehicleServer.load_add(_vehicle, BOARDING, RailVehicleLoad.PLATFORM_SIDE_LEFT, PASSENGERS)
    StationServer.dispatch_start(_vehicle, _cars)
    StationServer.dispatch_depart(_vehicle)
    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_EXCHANGE)

    # the doors open, then a second's worth of passengers at a time (update_exchange(), DynObj.cpp:2872-2960)
    var exchanged:Callable = func() -> bool:
        return not StationServer.dispatch_get_step(_vehicle) == StationServer.DISPATCH_STEP_EXCHANGE
    if not await wait_simulated_until(exchanged, _doors_opening() + BOARDING / EXCHANGE_SPEED + TICK,
            "the passengers exchanged"):
        return

    assert_ne(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_WAIT_DEPARTURE,
            "no wait for a departure given already")


func test_a_cancelled_dispatch_is_gone() -> void:
    RailVehicleServer.load_add(_vehicle, BOARDING, RailVehicleLoad.PLATFORM_SIDE_LEFT, PASSENGERS)
    StationServer.dispatch_start(_vehicle, _cars)

    StationServer.dispatch_cancel(_vehicle)

    assert_eq(StationServer.dispatch_get_step(_vehicle), StationServer.DISPATCH_STEP_NONE)
    assert_eq(StationServer.dispatch_get_exchange_time(_vehicle), 0.0)


func _doors() -> RailVehicleDoors:
    return VehicleServer.vehicle_component_get(_vehicle, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors


## The doors' opening: their delay, then their shift at their speed (update_doors(), Mover.cpp:8000-8008)
func _doors_opening() -> float:
    return _doors().open_delay + _doors().max_shift / _doors().open_speed
