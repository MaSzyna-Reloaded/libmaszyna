extends MaszynaGutTest

## MaszynaStation: the passengers a stop of the timetable lets off and takes on
## (basic_station::update_load(), station.cpp:25-88), on a car built in the test.

const TRACK_NAME:String = "station_test"
const TRACK_LENGTH:float = 400.0
const TRACK_OFFSET:float = 100.0
const CAPACITY:float = 100.0
const EXCHANGE_SPEED:float = 5.0
## Steps a vehicle just built takes to stand ready: its node takes the controller within the
## frames, the vehicle its configuration on its first step
const SETTLE_TICKS:int = 2
## The global random generator's seed, so the groups drawn are the same every run
const RANDOM_SEED:int = 7
const DEPARTURE:float = 10.5
## A car driven along itself, and one the other way round
const ALONG:int = 1
const REVERSED:int = -1

var _track:RID
var _car:RailVehicle3D
var _trainset:MaszynaDriverTrainset
var _timetable:MaszynaDriverTimetable
## The timetable it is the driver's of
var _made:RID


func before_each() -> void:
    seed(RANDOM_SEED)
    _track = build_track(TRACK_NAME, TRACK_LENGTH)
    _car = build_passenger_car("StationTest", TRACK_NAME, TRACK_OFFSET, CAPACITY, EXCHANGE_SPEED)
    await step(SETTLE_TICKS)
    _trainset = MaszynaDriverTrainset.new()
    var vehicles:Array[RID] = [_car.get_rid()]
    _trainset.vehicles = vehicles
    var directions:Array[int] = [ALONG]
    _trainset.directions = directions
    _timetable = MaszynaDriverTimetable.new()
    _made = StationServer.timetable_create()
    _entry("Start", false)
    _entry("Depot", true)
    _entry("End", false)
    _timetable.take(_made)


func after_each() -> void:
    _timetable.finish()
    StationServer.timetable_free(_made)
    free_rail_vehicle(_car)
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func test_at_the_first_station_passengers_get_on_and_the_train_waits_for_them() -> void:
    MaszynaStation.update_load(_trainset, _timetable, RailVehicleLoad.PLATFORM_SIDE_LEFT)

    # the component, as the dump is the one of the last step
    var load:RailVehicleLoad = VehicleServer.vehicle_component_get(_car.get_rid(), VehicleComponentType.COMPONENT_LOAD)
    assert_gt(RailVehicleServer.load_get_exchange_time(_car.get_rid()), 0.0, "a group gets on")
    assert_eq(load.get_load_name(), MaszynaStation.PASSENGERS, "the empty car takes passengers")


func test_at_a_maintenance_stop_and_the_last_station_an_empty_car_takes_nobody() -> void:
    _timetable.rewind("Depot")
    MaszynaStation.update_load(_trainset, _timetable, RailVehicleLoad.PLATFORM_SIDE_BOTH)
    assert_eq(RailVehicleServer.load_get_exchange_time(_car.get_rid()), 0.0, "a maintenance stop exchanges nothing")
    _timetable.rewind("End")
    MaszynaStation.update_load(_trainset, _timetable, RailVehicleLoad.PLATFORM_SIDE_BOTH)
    assert_eq(RailVehicleServer.load_get_exchange_time(_car.get_rid()), 0.0, "nobody gets on at the last station")


func test_a_car_the_other_way_round_has_the_platform_on_its_other_side() -> void:
    assert_eq(MaszynaStation.opposite_side(RailVehicleLoad.PLATFORM_SIDE_LEFT), RailVehicleLoad.PLATFORM_SIDE_RIGHT)
    assert_eq(MaszynaStation.opposite_side(RailVehicleLoad.PLATFORM_SIDE_BOTH), RailVehicleLoad.PLATFORM_SIDE_BOTH)
    var directions:Array[int] = [REVERSED]
    _trainset.directions = directions

    MaszynaStation.update_load(_trainset, _timetable, RailVehicleLoad.PLATFORM_SIDE_LEFT)
    # the doors' travel: their delay, then their shift at their speed (update_doors(), Mover.cpp:8000-8008)
    var doors:RailVehicleDoors = VehicleServer.vehicle_component_get(
            _car.get_rid(), VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    var opened:Callable = func() -> bool:
        var state:Dictionary = VehicleServer.vehicle_dump_state(_car.get_rid())
        return state.get("doors_right_open", false) or state.get("doors_left_open", false)
    if not await wait_simulated_until(opened, doors.open_delay + doors.max_shift / doors.open_speed + TICK,
            "the car's doors open"):
        return

    var state:Dictionary = VehicleServer.vehicle_dump_state(_car.get_rid())
    assert_true(state.get("doors_right_open", false), "the train's left is the reversed car's right")
    assert_false(state.get("doors_left_open", true))


func _entry(station:String, technical_stop:bool) -> void:
    var entry:int = StationServer.timetable_add_entry(_made, MaszynaStations.station_of(station), DEPARTURE, DEPARTURE)
    StationServer.timetable_entry_set_technical_stop(_made, entry, technical_stop)
