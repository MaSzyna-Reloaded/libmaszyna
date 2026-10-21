extends MaszynaGutTest

## A run records what its train did at the stations by their stop points: the arrival where it
## stops, the departure where it starts off again, the passing where it passes - and the delay of
## each; it stays with the trainset of the loco when the wagons are uncoupled

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## A trainset "freight" of `loco` (with a driver) and three wagons on main_track
const SCENERY:String = "trainsets_repeated_ids.scn"
const MINUTE:float = 1.0 / 60.0
const START_DEPARTURE:float = 8.0
const STOP_ARRIVAL:float = 8.5
const STOP_DEPARTURE:float = 8.55
const PASSING:float = 9.0

var _previous_game_dir:String = ""
var _previous_time:float = 0.0
var _scenery:MaszynaSceneryNode
var _stations:Array[RID] = []
var _timetable:RID
var _trainset:RID
var _loco:RID
var _track:RID
var _run:RID


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    _previous_time = SimulationServer.time_of_day
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    _scenery.autoload = false
    add_child(_scenery)
    await _scenery.load()
    _trainset = _scenery.get_trainsets()[0]
    _loco = RailVehicleServer.trainset_get_vehicles(_trainset)[0]
    _track = TrackServer.track_get_rid_by_name("main_track")
    # the start, where the train stands on no stop point; a stop and a station passed on its track
    for index:int in 3:
        _stations.append(StationServer.station_create())
    StationServer.station_add_stop_point(_stations[1], _track)
    StationServer.station_add_stop_point(_stations[2], _track)
    _timetable = StationServer.timetable_create()
    StationServer.timetable_add_entry(_timetable, _stations[0], StationServer.NO_TIME, START_DEPARTURE)
    StationServer.timetable_add_entry(_timetable, _stations[1], STOP_ARRIVAL, STOP_DEPARTURE)
    StationServer.timetable_add_entry(_timetable, _stations[2], StationServer.NO_TIME, PASSING)
    _run = StationServer.run_create(_timetable, _trainset)


func after_each() -> void:
    var run:RID = StationServer.run_find_by_trainset(RailVehicleServer.vehicle_get_trainset(_loco))
    if run.is_valid():
        StationServer.run_free(run)
    StationServer.timetable_free(_timetable)
    for station:RID in _stations:
        StationServer.station_free(station)
    _stations.clear()
    _scenery.free()
    SimulationServer.time_of_day = _previous_time
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_it_records_the_departure_the_arrival_and_the_passing_with_their_delays() -> void:
    watch_signals(StationServer)

    SimulationServer.time_of_day = START_DEPARTURE + 6.0 * MINUTE
    TrackServer.track_report_vehicle_started(_track, _loco, TrackServer.TRAVEL_TOWARD_END)
    assert_signal_emitted_with_parameters(StationServer, "run_departed", [_run, 0])
    assert_almost_eq(StationServer.run_get_delay(_run, SimulationServer.time_of_day), 6.0, 0.01,
            "it left the start six minutes late")

    StationServer.run_next_entry(_run)
    SimulationServer.time_of_day = STOP_ARRIVAL + 6.0 * MINUTE
    TrackServer.track_report_vehicle_stopped(_track, _loco)
    assert_signal_emitted_with_parameters(StationServer, "run_arrived", [_run, 1])
    assert_almost_eq(StationServer.run_entry_get_actual_arrival(_run, 1), SimulationServer.time_of_day, 0.0001)
    assert_almost_eq(StationServer.run_get_time_to_departure(_run, SimulationServer.time_of_day), -3.0, 0.01,
            "its departure was three minutes ago")
    assert_almost_eq(StationServer.run_get_delay(_run, STOP_DEPARTURE + 9.0 * MINUTE), 9.0, 0.01,
            "standing on, it gets later")

    SimulationServer.time_of_day = STOP_DEPARTURE + 9.0 * MINUTE
    TrackServer.track_report_vehicle_started(_track, _loco, TrackServer.TRAVEL_TOWARD_END)
    assert_signal_emitted_with_parameters(StationServer, "run_departed", [_run, 1])

    StationServer.run_next_entry(_run)
    SimulationServer.time_of_day = PASSING + 3.0 * MINUTE
    TrackServer.track_report_vehicle_entered(_track, _loco, TrackServer.TRAVEL_TOWARD_END)
    TrackServer.track_report_vehicle_left(_track, _loco)
    assert_signal_emitted_with_parameters(StationServer, "run_passed", [_run, 2])
    assert_almost_eq(StationServer.run_get_delay(_run, PASSING + 30.0 * MINUTE), 3.0, 0.01,
            "past the last station the delay stays as it passed it")


func test_a_stop_away_from_the_station_is_no_arrival() -> void:
    StationServer.run_next_entry(_run)
    # the station of the entry has its stop points on other tracks than the one the train stands on
    StationServer.station_clear_stop_points(_stations[1])
    watch_signals(StationServer)

    TrackServer.track_report_vehicle_stopped(_track, _loco)

    assert_signal_not_emitted(StationServer, "run_arrived")
    assert_eq(StationServer.run_entry_get_actual_arrival(_run, 1), StationServer.NO_TIME)


func test_the_run_stays_with_the_loco_when_the_wagons_are_uncoupled() -> void:
    (VehicleServer.vehicle_get_controller(_loco) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)

    assert_eq(RailVehicleServer.vehicle_get_trainset(_loco), _trainset)
    assert_eq(StationServer.run_find_by_trainset(_trainset), _run)
