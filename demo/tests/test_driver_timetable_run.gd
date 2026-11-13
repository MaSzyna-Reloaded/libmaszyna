extends MaszynaGutTest

## A virtual run of EX6435 (Krzyżowa 2's timetable, fixtures/timetables/ex6435.txt) along a line
## with a W4 (PassengerStopPoint) for each of its stations, through the driver's route
## (MaszynaDriverRoute.update(), TableUpdateStopPoint(), Driver.cpp:1093-1380): how far the
## timetable has got at every step - early, on time and late, the departure, a station passed
## without stopping, one stopped at, the terminus, and a station left behind without going on from
## it at its stop.

const Order = MaszynaAIDriver.Order
const EventImporter = preload("res://addons/libmaszyna/scenery/maszyna_event_importer.gd")
const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const FIXTURES:String = "res://tests/fixtures/timetables"
const LINE_VELOCITY:float = 100.0
const LINE_LENGTH:float = 9000.0
## The line is tracks of this length [m], end to end: the route forgets a track the whole train has
## left, and the stops on it
const TRACK_LENGTH:float = 1000.0
## Where the train stands at the start [m along the line], and the W4 of each station, in the
## timetable's order: Krzyżowa, Markowo_Górne (passed), Drawowo_Główne, Chełminów (the terminus)
const START:float = 300.0
const STOPS:Array[float] = [400.0, 2500.0, 4500.0, 6500.0]
## Their W4 as Krzyżowa 2 names them (krzyzowa2/sc2.scm, `include ip/pkp/w4n.inc <name> ...`)
const STOP_NAMES:Array[String] = ["Krzyżowa#tor6end", "Markowo_Górne#01", "Drawowo_Główne#01", "Chełminów#01"]
## The timetable's times [h]
const KRZYZOWA_DEPARTURE:float = 10.0 + 32.0 / 60.0
const DRAWOWO_ARRIVAL:float = 10.0 + 46.0 / 60.0
const DRAWOWO_DEPARTURE:float = 10.0 + 47.0 / 60.0
const CHELMINOW_ARRIVAL:float = 10.0 + 54.0 / 60.0
const MINUTE:float = 1.0 / 60.0
## The file of an entry's stop name, by the entry
const NAME_SOUND:String = "stop%d.ogg"
## Arriving at Krzyżowa at 10:35 (the test below), late against its arrival at 10:31 [min]; driven
## clear of it at 10:36, late against its departure at 10:32; and some time later on the way
const KRZYZOWA_ARRIVAL_LATE_MINUTES:int = 4
const KRZYZOWA_DEPARTURE_LATE_MINUTES:int = 4
const LATER_ON_MINUTES:float = 10.0
## Beyond a stop, farther than the train needs to be clear of it (NEXT_STATION_AFTER_DEPARTURE plus
## the train's length) [m]
const CLEAR_OF_THE_STOP:float = 200.0
## Float rounding of the hours-to-minutes conversion [min]
const EPSILON:float = 0.01
## Driving and standing [km/h]
const RUNNING_SPEED:float = 80.0
const STANDING_SPEED:float = 0.0
## One step of the run [m]
const STEP:float = 20.0
## Beyond the track of a stop, the whole train clear of it [m along the line]
const KRZYZOWA_LEFT_BEHIND:float = 1200.0
const DRAWOWO_LEFT_BEHIND:float = 5200.0

var _tracks:Array[RID] = []
var _events:Array[RID] = []
var _scenery:MaszynaIncludeNode = null
var _vehicles:Array[RailVehicle3D] = []
var _vehicle:RID = RID()
var _trainset:MaszynaDriverTrainset = null
var _route:MaszynaDriverRoute = null
var _timetable:MaszynaDriverTimetable = null
## What the train announced on the updates so far
var _announced:Array[MaszynaVehicleSoundServer.Announcement] = []
## Where the front of the train is [m along the line]
var _position:float = START
## The order the driver drives under on the updates (MaszynaAIDriver.Order)
var _order:int = Order.OBEY_TRAIN


func before_each() -> void:
    for index:int in roundi(LINE_LENGTH / TRACK_LENGTH):
        var track:RID = TrackServer.track_create()
        _tracks.append(track)
        var curve:TrackCurve = TrackCurve.new()
        curve.p1 = Vector3(index * TRACK_LENGTH, 0.0, 0.0)
        curve.p2 = Vector3((index + 1) * TRACK_LENGTH, 0.0, 0.0)
        TrackServer.track_update_curves(track, curve, null)
        TrackServer.track_update(track, TrackServer.TRACK_NORMAL, "line" if index == 0 else "line%d" % index, 1.435)
        TrackServer.track_set_velocity(track, LINE_VELOCITY)
    TrackServer.topology_rebuild()
    _order = Order.OBEY_TRAIN
    var data:RID = MaszynaTimetableFactory.load_timetable(FIXTURES, "ex6435")
    # each W4 as ip/pkp/w4n.inc writes it, in the scenery's cp1250, read by the scenery's parser
    var text:String = ""
    for index:int in STOPS.size():
        text += "event %s_stopinfo putvalues 0.0 none %s 0 0 PassengerStopPoint:%s -11 352 endevent\n" % [
            STOP_NAMES[index], STOPS[index], STOP_NAMES[index]]
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(_cp1250(text))
    var context:MaszynaImporterContext = MaszynaImporterContext.new()
    parser.register_handler("event", func(p:MaszynaParser) -> Array: return EventImporter.new().import(p, context))
    parser.parse()
    _scenery = MaszynaIncludeNode.new()
    _scenery.autoload = false
    add_child(_scenery)
    var memcells:Array[MaszynaMemcellData] = []
    var launchers:Array[MaszynaEventLauncherData] = []
    var sounds:Array[SoundData] = []
    var isolated:Array[MaszynaIsolatedData] = []
    var track_data:Array[TrackData] = []
    var track_rids:Array[RID] = []
    var models:Array[ModelData] = []
    var model_rids:Array[RID] = []
    var power_sources:Array[PowerSourceData] = []
    await MaszynaEventFactory.build(_scenery, context.events, memcells, launchers, sounds, isolated, track_data,
            track_rids, models, model_rids, power_sources)
    for index:int in STOP_NAMES.size():
        var event:RID = ScenarioEventServer.event_get_rid_by_name(StringName((STOP_NAMES[index] + "_stopinfo").to_lower()))
        _events.append(event)
        var track:RID = _tracks[floori(STOPS[index] / TRACK_LENGTH)]
        MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENT1, event)
        MaszynaScenarioEventServer.track_add_event(track, MaszynaScenarioEventServer.TRACK_EVENT2, event)
    var physics_node:VehiclePhysicsNode = build_vehicle_node("TimetableRunTest", SM42)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = "line"
    vehicle.start_track_offset = START
    add_child(vehicle)
    _vehicles.append(vehicle)
    vehicle.controller_path = vehicle.get_path_to(physics_node)
    await wait_idle_frames(2)
    _vehicle = vehicle.get_rid()
    _position = START
    # the way the line goes on: the long way, towards its end
    var ahead:float = 0.0
    for segment:TrackRouteSegment in RailVehicleServer.vehicle_trace_route(_vehicle, 1, LINE_LENGTH):
        ahead += segment.length
    _trainset = MaszynaDriverTrainset.new()
    _trainset.update(_vehicle, 1 if ahead > LINE_LENGTH / 2.0 else -1, true)
    _route = MaszynaDriverRoute.new()
    _timetable = MaszynaDriverTimetable.new()
    _timetable.take(data)


func after_each() -> void:
    for vehicle:RailVehicle3D in _vehicles:
        remove_child(vehicle)
        vehicle.queue_free()
    _vehicles.clear()
    # the events are the scenery's (MaszynaEventFactory.build()) - freed with it
    _events.clear()
    remove_child(_scenery)
    _scenery.free()
    for track:RID in _tracks:
        TrackServer.track_free(track)
    _tracks.clear()
    TrackServer.topology_rebuild()


func test_the_timetable_is_the_fixture() -> void:
    var names:PackedStringArray = []
    for entry:int in _timetable.get_entry_count():
        names.append(_timetable.station_name(entry))
    assert_eq(names, PackedStringArray(["Krzyzowa", "Markowo Gorne", "Drawowo Glowne", "Chelminow"]),
            "plain ASCII, as the original compares them (mtable.cpp:430), a station of each - its `_` a space")


func test_the_stops_are_named_as_the_timetable_names_them() -> void:
    for index:int in _events.size():
        var action:MaszynaVehicleCommandAction = ScenarioEventServer.event_get_action(_events[index])
        var stop:String = action.command.trim_prefix(MaszynaEventFactory.PASSENGER_STOP_POINT)
        assert_true(action.command.begins_with(MaszynaEventFactory.PASSENGER_STOP_POINT))
        assert_eq(MaszynaStations.key(stop), MaszynaStations.key(_timetable.station_name(index)),
                "cut at its # and plain ASCII (Event.cpp:715-719)")


func test_early_at_the_first_station_it_waits_for_the_departure() -> void:
    _update(KRZYZOWA_DEPARTURE - 5.0 * MINUTE, STANDING_SPEED)

    assert_true(_route.at_passenger_stop, "standing at the W4")
    assert_has(_route.stop_orders, MaszynaDriverRoute.StopOrder.LOAD_EXCHANGE, "its passengers get off and on")
    assert_eq(_route.exchange_platform, RailVehicleLoad.PLATFORM_SIDE_RIGHT, "at the platform its W4 names (352)")
    assert_eq(_timetable.station_index, 0, "Krzyżowa, until the departure")
    assert_eq(_timetable.station_start, 0)
    assert_almost_eq(_timetable.latency, 5.0, EPSILON, "5 min early")
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), KRZYZOWA_DEPARTURE - 5.0 * MINUTE), 0, "on time while it waits")


func test_on_time_it_leaves_and_is_shown_at_the_station_until_clear_of_it() -> void:
    _update(KRZYZOWA_DEPARTURE - MINUTE, STANDING_SPEED)
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)

    assert_eq(_timetable.station_index, 1, "on to Markowo_Górne")
    assert_eq(_timetable.station_start, 0, "still shown at Krzyżowa")
    assert_has(_route.stop_orders, MaszynaDriverRoute.StopOrder.GUARD_SIGNAL,
            "the guard's message is due (moveGuardSignal, Driver.cpp:1313-1316)")
    _run_to(STOPS[0] + 200.0, KRZYZOWA_DEPARTURE + MINUTE)
    assert_eq(_timetable.station_start, 1, "clear of Krzyżowa, Markowo_Górne is shown")


func test_late_it_leaves_at_once_and_the_delay_counts() -> void:
    _update(KRZYZOWA_DEPARTURE + 3.0 * MINUTE, STANDING_SPEED)

    assert_eq(_timetable.station_index, 1, "late: it goes at once")
    assert_almost_eq(_timetable.latency, -3.0, EPSILON, "3 min late")
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), KRZYZOWA_DEPARTURE + 3.5 * MINUTE), KRZYZOWA_ARRIVAL_LATE_MINUTES,
            "still standing: late against its arrival (10:31)")
    _run_to(STOPS[0] + CLEAR_OF_THE_STOP, KRZYZOWA_DEPARTURE + KRZYZOWA_DEPARTURE_LATE_MINUTES * MINUTE)
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), KRZYZOWA_DEPARTURE + LATER_ON_MINUTES * MINUTE),
            KRZYZOWA_DEPARTURE_LATE_MINUTES, "on the way: as it drove clear of Krzyżowa")


func test_a_station_without_a_stop_is_passed_at_speed() -> void:
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)
    _run_to(STOPS[1] - 300.0, KRZYZOWA_DEPARTURE + 5.0 * MINUTE)
    assert_eq(_timetable.station_index, 1, "Markowo_Górne ahead")

    _run_to(STOPS[1] + 100.0, KRZYZOWA_DEPARTURE + 8.0 * MINUTE)
    assert_eq(_timetable.station_index, 2, "passed, on to Drawowo_Główne")
    _run_to(STOPS[1] + 600.0, KRZYZOWA_DEPARTURE + 9.0 * MINUTE)
    assert_eq(_timetable.station_start, 2, "Drawowo_Główne shown")


func test_a_station_with_a_stop_is_stopped_at_and_left_at_the_departure() -> void:
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)
    _run_to(STOPS[2] - 100.0, DRAWOWO_ARRIVAL - MINUTE)
    assert_eq(_timetable.station_index, 2, "Drawowo_Główne ahead")

    _update(DRAWOWO_ARRIVAL - MINUTE, STANDING_SPEED)
    assert_true(_route.at_passenger_stop, "stopped at it")
    assert_eq(_timetable.station_index, 2, "waiting for the departure")
    _update(DRAWOWO_DEPARTURE, STANDING_SPEED)
    assert_eq(_timetable.station_index, 3, "left, on to Chełminów")
    assert_eq(_timetable.station_start, 2, "still shown at Drawowo_Główne")


func test_the_terminus_ends_the_timetable() -> void:
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)
    _run_to(STOPS[2] - 100.0, DRAWOWO_ARRIVAL)
    _update(DRAWOWO_DEPARTURE, STANDING_SPEED)
    _run_to(STOPS[3] - 100.0, CHELMINOW_ARRIVAL)
    assert_eq(_timetable.station_index, 3, "Chełminów ahead")

    _update(CHELMINOW_ARRIVAL, STANDING_SPEED)
    assert_eq(_timetable.timetable, RID(), "the timetable ends at the terminus")


## The train announces the stop it stands at, the next stop once clear of it, and a stop ahead
## between 250 and 750 m from it - each once (Driver.cpp:1222-1228, 1252-1254, 6588-6592)
func test_the_train_announces_its_stops() -> void:
    _announced.clear()
    _update(KRZYZOWA_DEPARTURE - MINUTE, STANDING_SPEED)
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)
    _run_to(STOPS[0] + CLEAR_OF_THE_STOP, KRZYZOWA_DEPARTURE + MINUTE)
    assert_eq(_announced, [MaszynaVehicleSoundServer.ANNOUNCEMENT_CURRENT, MaszynaVehicleSoundServer.ANNOUNCEMENT_NEXT],
            "Krzyżowa at its W4, then the next stop")
    _announced.clear()

    _run_to(STOPS[2] - 100.0, DRAWOWO_ARRIVAL - MINUTE)
    assert_eq(_announced, [MaszynaVehicleSoundServer.ANNOUNCEMENT_APPROACHING],
            "Drawowo_Główne ahead; Markowo_Górne, passed at speed, not")



## The driver tells the train's run what it did, as it knows where the train stops: standing at its
## first W4 it has arrived, and it has not left while it stands; it left once clear of the stop
## (UpdateNextStop()), passed the station it did not stop at, and arrived at its next stop
## (UpdateMTable() at the W4, Driver.cpp:1231, 6581-6592)
func test_the_driver_tells_the_run_where_the_train_arrived_and_left() -> void:
    _timetable.take(_timetable.timetable, RailVehicleServer.vehicle_get_trainset(_vehicle))
    var run:RID = _timetable.run

    _update(KRZYZOWA_DEPARTURE - MINUTE, STANDING_SPEED)
    assert_ne(StationServer.run_entry_get_actual_arrival(run, 0), StationServer.NO_TIME, "arrived at its W4")
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)
    assert_eq(StationServer.run_entry_get_actual_departure(run, 0), StationServer.NO_TIME, "not left while it stands")

    _run_to(STOPS[0] + CLEAR_OF_THE_STOP, KRZYZOWA_DEPARTURE + MINUTE)
    assert_ne(StationServer.run_entry_get_actual_departure(run, 0), StationServer.NO_TIME, "left once clear of it")
    _run_to(STOPS[2] - 100.0, DRAWOWO_ARRIVAL)
    assert_eq(StationServer.run_entry_get_actual_arrival(run, 1), StationServer.NO_TIME, "Markowo_Górne not stopped at")
    assert_ne(StationServer.run_entry_get_actual_departure(run, 1), StationServer.NO_TIME, "but passed")
    _update(DRAWOWO_ARRIVAL, STANDING_SPEED)
    assert_eq(StationServer.run_get_current_entry(run), 2)
    assert_ne(StationServer.run_entry_get_actual_arrival(run, 2), StationServer.NO_TIME, "arrived at Drawowo_Główne")
    _timetable.take(RID())

## A late train leaves at once on the update it stops in, and still announces the stop it stood at,
## not the next one (current_stop_sound() while StationIndex is still the stop's, DynObj.cpp:7991-7993)
func test_a_late_train_announces_the_stop_it_stands_at() -> void:
    var extras:Array[Dictionary] = []
    for entry:int in _timetable.get_entry_count():
        var extra:Dictionary = StationServer.timetable_entry_get_extra(_timetable.timetable, entry)
        extras.append(extra)
        var named:Dictionary = extra.duplicate()
        named[MaszynaTimetableFactory.EXTRA_NAME_SOUND] = NAME_SOUND % entry
        StationServer.timetable_entry_set_extra(_timetable.timetable, entry, named)

    _update(KRZYZOWA_DEPARTURE + 3.0 * MINUTE, STANDING_SPEED)

    assert_eq(_timetable.station_index, 1, "late: it went on at once")
    assert_eq(_route.announcements, [MaszynaVehicleSoundServer.ANNOUNCEMENT_CURRENT])
    assert_eq(_route.announcement_files, [PackedStringArray([NAME_SOUND % 0, NAME_SOUND % 3])],
            "Krzyżowa's name, then the last station's")
    for entry:int in extras.size():
        StationServer.timetable_entry_set_extra(_timetable.timetable, entry, extras[entry])

## A train driven past its first station without stopping at it: once the whole train has left the
## stop's track the station is passed, the next one follows and is shown, and the run is told it
## passed it - not kept as the next station until another station's stop rewinds the timetable
func test_a_station_driven_past_without_stopping_is_left_behind() -> void:
    _timetable.take(_timetable.timetable, RailVehicleServer.vehicle_get_trainset(_vehicle))
    var run:RID = _timetable.run

    _run_to(STOPS[0] + CLEAR_OF_THE_STOP, KRZYZOWA_DEPARTURE - MINUTE)
    assert_eq(_timetable.station_index, 0, "Krzyżowa, while the train is on the stop's track")
    _run_to(KRZYZOWA_LEFT_BEHIND, KRZYZOWA_DEPARTURE)

    assert_eq(_timetable.station_index, 1, "on to Markowo_Górne")
    assert_eq(_timetable.station_start, 1, "Markowo_Górne shown")
    assert_almost_eq(_timetable.latency, 0.0, EPSILON, "against the departure, as it left")
    assert_eq(StationServer.run_get_current_entry(run), 1)
    assert_eq(StationServer.run_entry_get_actual_arrival(run, 0), StationServer.NO_TIME, "not stopped at")
    assert_ne(StationServer.run_entry_get_actual_departure(run, 0), StationServer.NO_TIME, "but passed")
    _timetable.take(RID())


## A train that stopped and left before the departure: once the whole train has left the stop's
## track it goes on to the next station, and the run is told it left
func test_a_station_left_before_the_departure_is_left_behind() -> void:
    _timetable.take(_timetable.timetable, RailVehicleServer.vehicle_get_trainset(_vehicle))
    var run:RID = _timetable.run
    _update(KRZYZOWA_DEPARTURE, STANDING_SPEED)
    _run_to(STOPS[2] - 100.0, DRAWOWO_ARRIVAL - MINUTE)
    _update(DRAWOWO_ARRIVAL - MINUTE, STANDING_SPEED)
    assert_eq(_timetable.station_index, 2, "standing at Drawowo_Główne")

    _run_to(DRAWOWO_LEFT_BEHIND, DRAWOWO_ARRIVAL)

    assert_eq(_timetable.station_index, 3, "on to Chełminów")
    assert_eq(_timetable.station_start, 3, "Chełminów shown")
    assert_ne(StationServer.run_entry_get_actual_arrival(run, 2), StationServer.NO_TIME, "arrived")
    assert_ne(StationServer.run_entry_get_actual_departure(run, 2), StationServer.NO_TIME, "and left")
    _timetable.take(RID())


## A train put at its first station, its driver not driving yet (the engine to be got ready): it
## starts its run there without arriving, and driven off before the departure it has left it - a
## departure once the driver drives as a train, not a station passed (MaSzyna-Reloaded/reports#87)
func test_a_train_put_at_its_first_station_starts_its_run_there() -> void:
    _timetable.take(_timetable.timetable, RailVehicleServer.vehicle_get_trainset(_vehicle))
    var run:RID = _timetable.run
    watch_signals(StationServer)
    _order = Order.PREPARE_ENGINE

    _update(KRZYZOWA_DEPARTURE - 5.0 * MINUTE, STANDING_SPEED)
    assert_ne(StationServer.run_entry_get_actual_arrival(run, 0), StationServer.NO_TIME, "standing there")
    assert_signal_not_emitted(StationServer, "run_arrived", "from the start, not arrived")
    assert_false(_timetable.arrived, "its driver arrives there once it drives as a train")

    _run_to(KRZYZOWA_LEFT_BEHIND, KRZYZOWA_DEPARTURE - MINUTE)
    _order = Order.OBEY_TRAIN
    _update(KRZYZOWA_DEPARTURE - MINUTE, RUNNING_SPEED)

    assert_eq(_timetable.station_start, 1, "Markowo_Górne shown")
    assert_eq(StationServer.run_get_current_entry(run), 1)
    assert_signal_emitted_with_parameters(StationServer, "run_departed", [run, 0])
    assert_signal_not_emitted(StationServer, "run_passed", "it left its start, it did not pass it")
    _timetable.take(RID())


## The train driven forward to `position` [m] in steps at RUNNING_SPEED, the route read at every
## step, the time `hours`
func _run_to(position:float, hours:float) -> void:
    while _position < position:
        RailVehicleServer.vehicle_move(_vehicle, STEP * _trainset.direction)
        _position += STEP
        _update(hours, RUNNING_SPEED)


## One update of the route as the driver does it for a train (MaszynaAIDriver._update())
func _update(hours:float, speed:float) -> void:
    _trainset.update(_vehicle, _trainset.direction, true)
    _route.update(_vehicle, _order, false, LINE_VELOCITY, speed, MaszynaDriverSpeed.EASY_ACCELERATION,
            LINE_VELOCITY, _trainset, _timetable, hours, LINE_VELOCITY, LINE_VELOCITY, false,
            MaszynaDriverBraking.new(), false)
    _announced.append_array(_route.announcements)


## The driver's timetable state (MaszynaDriverTimetable.state_is_standing())
func _state() -> Dictionary:
    return {
        "timetable": _timetable.timetable,
        "station_index": _timetable.station_index,
        "station_start": _timetable.station_start,
        "latency": _timetable.latency,
        "delay": _timetable.delay,
        "arrived": _timetable.arrived,
    }


## `text` in cp1250, as the original's scenery files are written (Windows1250.decode() reversed)
func _cp1250(text:String) -> PackedByteArray:
    var bytes:PackedByteArray = []
    for index:int in text.length():
        var code:int = text.unicode_at(index)
        bytes.append(code if code < Windows1250.ASCII_END
                else Windows1250.ASCII_END + Windows1250.HIGH_CODE_POINTS.find(code))
    return bytes
