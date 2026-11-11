extends MaszynaGutTest

## What the scripts of the data are given of a train: PythonScreenState.timetable_state() - the keys a
## Python script reads (mtable.cpp:641-671) - and, for a `make:` texture drawn by Lua, the vehicle its
## `$timetable=` names, whose trainset's run the script reads itself (reloaded.station)

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## A trainset "freight" of `loco` (with a driver) and three wagons on main_track
const SCENERY:String = "trainsets_repeated_ids.scn"
const TRAIN:String = "IC 1234"
const CATEGORY:String = "EI"
const START_DEPARTURE:float = 8.0
const STOP_ARRIVAL:float = 8.5
const STOP_DEPARTURE:float = 8.55
## The run's times are the timetable's one hour later
const OFFSET:float = 1.0
const KILOMETRE:float = 12.5
const TRACKS:int = 2
const REQUEST:String = "make:lua/timetable_sign?$timetable=loco&_id1=IC 1234&kod_e=IC"

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _stations:Array[RID] = []
var _timetable:RID
var _loco:RID
var _run:RID


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    _scenery.autoload = false
    add_child(_scenery)
    await _scenery.load()
    var trainset:RID = _scenery.get_trainsets()[0]
    _loco = RailVehicleServer.trainset_get_vehicles(trainset)[0]
    for station_name:String in ["Krakow", "Olkusz"]:
        var station:RID = StationServer.station_create()
        StationServer.station_set_name(station, station_name)
        _stations.append(station)
    _timetable = StationServer.timetable_create()
    StationServer.timetable_set_train_name(_timetable, TRAIN)
    StationServer.timetable_set_train_category(_timetable, CATEGORY)
    StationServer.timetable_set_origin(_timetable, _stations[0])
    StationServer.timetable_set_destination(_timetable, _stations[1])
    StationServer.timetable_add_entry(_timetable, _stations[0], StationServer.NO_TIME, START_DEPARTURE)
    var stop:int = StationServer.timetable_add_entry(_timetable, _stations[1], STOP_ARRIVAL, STOP_DEPARTURE)
    StationServer.timetable_entry_set_kilometre(_timetable, stop, KILOMETRE)
    StationServer.timetable_entry_set_track_count(_timetable, stop, TRACKS)
    StationServer.timetable_entry_set_extra(_timetable, stop, {MaszynaTimetableFactory.EXTRA_FACILITIES: "R4,PP"})
    _run = StationServer.run_create(_timetable, trainset, OFFSET)


func after_each() -> void:
    var run:RID = StationServer.run_find_by_trainset(RailVehicleServer.vehicle_get_trainset(_loco))
    if run.is_valid():
        StationServer.run_free(run)
    StationServer.timetable_free(_timetable)
    for station:RID in _stations:
        StationServer.station_free(station)
    _stations.clear()
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_a_python_script_gets_the_train_its_trainset_runs() -> void:
    var state:Dictionary = PythonScreenState.timetable_state(_loco)

    assert_eq(state["trainnumber"], TRAIN)
    assert_eq(state["traincategory"], CATEGORY)
    assert_eq(state["train_stationfrom"], "Krakow")
    assert_eq(state["train_stationto"], "Olkusz")
    assert_eq(state["train_stationcount"], 2)
    assert_eq(state["train_station2_name"], "Olkusz")
    assert_eq(state["train_station2_fclt"], "R4,PP")
    assert_eq(state["train_station2_lctn"], KILOMETRE)
    assert_eq(state["train_station2_tracks"], TRACKS)
    assert_eq([state["train_station2_ah"], state["train_station2_am"]], [9, 30.0], "the run's time, an hour later")
    assert_eq([state["train_station2_dh"], state["train_station2_dm"]], [9, 33.0])
    assert_eq([state["train_station1_ah"], state["train_station1_am"]], [-1, -1.0], "no arrival planned at the start")
    assert_eq(state["train_stationstart"], 1, "it stands at its start, counted from 1")
    assert_eq(state["train_stationindex"], 2, "and has arrived there - the next station is the second")


func test_a_trainset_running_no_timetable_is_train_none() -> void:
    StationServer.run_free(_run)

    assert_eq(PythonScreenState.timetable_state(_loco)["trainnumber"], PythonScreenState.NO_TRAIN)
    assert_eq(PythonScreenState.timetable_state(_loco)["train_stationcount"], 0)


func test_a_make_texture_with_a_lua_script_gets_the_vehicle_and_the_request() -> void:
    watch_signals(ScenarioScriptServer)

    var texture:Texture2D = MaszynaMaterialManager.load_texture("", REQUEST)

    assert_true(texture is DrawingTexture, "drawn by Lua")
    assert_signal_not_emitted(ScenarioScriptServer, "script_error", "the script got what it asserts")
    assert_eq(MaszynaMaterialManager.load_texture("", REQUEST), texture, "drawn once, then found by its name")
