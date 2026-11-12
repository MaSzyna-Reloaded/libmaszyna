extends MaszynaGutTest

## A run stays with the trainset of the loco when the wagons are uncoupled

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
    # the start, a stop and a station passed
    for index:int in 3:
        _stations.append(StationServer.station_create())
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


func test_the_run_stays_with_the_loco_when_the_wagons_are_uncoupled() -> void:
    (VehicleServer.vehicle_get_controller(_loco) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)

    assert_eq(RailVehicleServer.vehicle_get_trainset(_loco), _trainset)
    assert_eq(StationServer.run_find_by_trainset(_trainset), _run)
