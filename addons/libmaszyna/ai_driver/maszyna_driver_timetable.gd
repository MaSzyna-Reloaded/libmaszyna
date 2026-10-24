@tool
extends RefCounted
class_name MaszynaDriverTimetable

## How far the driver has got through its timetable (TTrainParameters' StationIndex,
## NextStationName, TTVmax, LastStationLatency, mtable.cpp:36-190, 632-640) and how it treats the
## passenger stops on the way (moveStopPoint, moveStopCloser, Driver.h:58-59). The timetable is
## StationServer's, the driver's train runs it: its run is moved on as the next station is shown,
## and the server records there what the train did. `station_index` counts the timetable's entries
## from 0, the original's from 1; the times are the run's, the timetable's shifted by its offset.
##
## Dropped: the original's TTVmax of 100 without a timetable (mtable.cpp:224, marked for removal
## there) - no timetable is no limit here; IsMaintenance(), whose flag the original reads from the
## wrong token and never sets (mtable.cpp:486).

## The timetable, or how far the driver got through it, has changed
signal changed

const MINUTES_PER_HOUR:float = 60.0
const SECONDS_PER_MINUTE:float = 60.0
## CompareTime() (utilities.cpp:50): a difference over half a day is the other way round the clock
const HALF_DAY_MINUTES:float = 720.0
const DAY_MINUTES:float = 1440.0
const HOURS_PER_DAY:float = 24.0

## StationServer's, RID() for none
var timetable:RID = RID()
## Its run by the driver's trainset, and the hours it shifts the timetable's times by
var run:RID = RID()
var offset:float = 0.0
## The entry it drives to next
var station_index:int = 0
## The entry shown as the station it stands at or has just left (StationStart): it catches up with
## `station_index` once the train has driven clear of the stop (UpdateNextStop(), Driver.cpp:6481)
var station_start:int = 0
## The station whose stop it looks for (asNextStop), empty past the last - it changes on leaving
## a station, not on arriving
var next_stop:String = ""
## The timetable's speed on the way to the next station [km/h], -1 for none (TTVmax)
var velocity:float = MaszynaDriverSpeed.NO_LIMIT
## Early on leaving the last station [min], late when negative (LastStationLatency: the departure
## less the arrival, mtable.cpp:122; late is below 0, UpdateDelayFlag(), Driver.cpp:5605)
var latency:float = 0.0
## Late at the station reached last [min], early when negative: its arrival as it arrives (its
## departure where it passes), its departure once the train has driven clear of it. The original
## keeps no such value - its timetable panel shows none (driveruipanels.cpp:298-466).
var delay:float = 0.0
## It has reached the next station (`station_index`) and not gone on from it yet
var arrived:bool = false
## It stops at passenger stops (moveStopPoint) - not while it couples up or turns there
var stop_point:bool = true
## It draws up close to the next passenger stop (moveStopCloser) - not at the start, nor on the far
## side after turning
var stop_closer:bool = false
## The station after the one arrived at (NextStationName)
var _next_station:String = ""


## A new timetable, run by `trainset` shifted by `minutes`, the first station next (`Timetable:`,
## Driver.cpp:4526-4535)
func take(new_timetable:RID, trainset:RID = RID(), minutes:float = 0.0) -> void:
    if run.is_valid() and StationServer.run_find_by_trainset(StationServer.run_get_trainset(run)) == run:
        StationServer.run_free(run)
    run = RID()
    timetable = new_timetable
    offset = minutes / MINUTES_PER_HOUR
    if timetable.is_valid() and trainset.is_valid() and not StationServer.run_find_by_trainset(trainset).is_valid():
        run = StationServer.run_create(timetable, trainset, offset)
    station_index = 0
    station_start = 0
    latency = 0.0
    delay = 0.0
    arrived = false
    next_stop = ""
    _next_station = ""
    velocity = MaszynaDriverSpeed.NO_LIMIT
    # a number instead of a timetable is the speed (mtable.cpp:279)
    if timetable.is_valid() and StationServer.timetable_get_speed_limit(timetable) > 0.0:
        velocity = StationServer.timetable_get_speed_limit(timetable)
    if get_entry_count() > 0:
        next_stop = station_name(0)
        _next_station = next_stop
        velocity = StationServer.timetable_entry_get_speed_limit(timetable, 0)
    changed.emit()


## How many stations the timetable has
func get_entry_count() -> int:
    return StationServer.timetable_get_entry_count(timetable) if timetable.is_valid() else 0


## The station's name, as its station is named
func station_name(entry:int) -> String:
    return StationServer.station_get_name(StationServer.timetable_entry_get_station(timetable, entry))


## The arrival and the departure planned at the entry [h], as the run's - NO_TIME for none
func arrival(entry:int) -> float:
    return _shifted(StationServer.timetable_entry_get_arrival(timetable, entry))


func departure(entry:int) -> float:
    return _shifted(StationServer.timetable_entry_get_departure(timetable, entry))


func radio_channel(entry:int) -> int:
    return StationServer.timetable_entry_get_radio_channel(timetable, entry)


## Whether it stops at the next station (IsStop()) - past the last one it always does
func is_stop() -> bool:
    return StationServer.timetable_entry_is_stop(timetable, station_index) if station_index < get_entry_count() else true


## Whether the next station is the last (IsLastStop(), "StationIndex < StationCount")
func is_last_station() -> bool:
    return station_index >= get_entry_count() - 1


## Arrived at the next station at `hours` (UpdateMTable()): the delay counted and the station after
## it named; true once, on the first arrival
func arrive(hours:float) -> bool:
    if station_index >= get_entry_count() or not next_stop == _next_station:
        return false
    latency = compare_time(hours, departure(station_index))
    delay = -compare_time(hours, arrival(station_index) if is_stop() else departure(station_index))
    arrived = true
    if station_index < get_entry_count() - 1:
        _next_station = station_name(station_index + 1)
        velocity = StationServer.timetable_entry_get_speed_limit(timetable, station_index + 1)
    else:
        _next_station = ""
    changed.emit()
    return true


## On to the next station (StationIndexInc(), NextStop())
func advance() -> void:
    station_index += 1
    arrived = false
    next_stop = _next_station
    changed.emit()


## The train has driven clear of the station it left at `hours`: its delay is the departure's, and
## the next station is shown as current (UpdateNextStop(), Driver.cpp:6481-6487)
func show_next_station(hours:float) -> void:
    if station_start < station_index and station_start < get_entry_count():
        if departure(station_start) >= 0.0:
            delay = -compare_time(hours, departure(station_start))
    station_start = station_index
    _move_run()
    changed.emit()


## Ended: no timetable any more (NewName("none"))
func finish() -> void:
    take(RID())


## It drives past the passenger stops - coupling up, or a locomotive turning at one
func pass_stops() -> void:
    stop_point = false


## It stops at the passenger stops again - as a train
func mind_stops() -> void:
    stop_point = true


## It draws up close to the next passenger stop - once it has left one
func draw_up_close() -> void:
    stop_closer = true


## It stops where it sees the next passenger stop - the far side after turning, the end
func stop_short() -> void:
    stop_closer = false


## Whether it may leave the station it stands at, at `hours` (IsTimeToGo()): at once where it only
## passes, at the departure time where it stops, never from the last
func is_time_to_go(hours:float) -> bool:
    if is_last_station():
        return false
    if not is_stop():
        return true
    return compare_time(hours, departure(station_index)) <= 0.0


## The seconds from `hours` to the departure from the station it stands at or has just left
## (StationStart), 0 where it only passes or follows no timetable (seconds_until_departure(),
## mtable.cpp:184-190)
func seconds_until_departure(hours:float) -> float:
    if station_start >= get_entry_count() or not StationServer.timetable_entry_is_stop(timetable, station_start):
        return 0.0
    return SECONDS_PER_MINUTE * compare_time(hours, departure(station_start))


## Whether the train turns at the station it stands at (DirectionChange(), `@`) - not at the last
func turns_here() -> bool:
    return not is_last_station() and StationServer.timetable_entry_is_direction_change(timetable, station_index)


## A passenger stop of another station than the next one: the timetable goes on from it
## (RewindTimeTable()); false when the timetable has no such station
func rewind(station:String) -> bool:
    for index:int in get_entry_count():
        if MaszynaStations.key(station_name(index)) == MaszynaStations.key(station):
            station_index = index
            arrived = false
            # shown from it at once (Driver.cpp:1092)
            station_start = index
            next_stop = station_name(index)
            _next_station = next_stop
            velocity = StationServer.timetable_entry_get_speed_limit(timetable, index)
            _move_run()
            changed.emit()
            return true
    return false


## The run is at the entry shown: the one the train drives to or stands at
func _move_run() -> void:
    if not run.is_valid():
        return
    while StationServer.run_get_current_entry(run) < station_start:
        StationServer.run_next_entry(run)
    while StationServer.run_get_current_entry(run) > station_start:
        StationServer.run_previous_entry(run)


## A time of the timetable as the run's [h]
func _shifted(hours:float) -> float:
    return hours if hours < 0.0 else fposmod(hours + offset, HOURS_PER_DAY)


## Whether the train of a timetable state (a driver timetable's timetable, offset, station_index,
## station_start, delay and arrived by their names) stands at its
## station, the entry `station_start` names: it has arrived there, or left it and not driven clear
## of it yet
static func state_is_standing(state:Dictionary) -> bool:
    return int(state.get("station_start", 0)) < int(state.get("station_index", 0)) or state.get("arrived", false)


## The delay [min] of a train by its timetable state (state_is_standing()) at
## `hours`, late when positive: at a stop, what it arrived with and, past the departure, the time
## since - an early arrival waits for the departure and is none; on the way, what it left the last
## station with
static func state_delay_minutes(state:Dictionary, hours:float) -> int:
    var state_timetable:RID = state.get("timetable", RID())
    var start:int = state.get("station_start", 0)
    var state_delay:int = roundi(state.get("delay", 0.0))
    if state_is_standing(state) and state_timetable.is_valid() \
            and start < StationServer.timetable_get_entry_count(state_timetable):
        var planned:float = StationServer.timetable_entry_get_departure(state_timetable, start)
        if StationServer.timetable_entry_is_stop(state_timetable, start) and planned >= 0.0:
            var departure_at:float = fposmod(planned + float(state.get("offset", 0.0)), HOURS_PER_DAY)
            return maxi(0, maxi(state_delay, floori(-compare_time(hours, departure_at))))
    return state_delay


## CompareTime() (utilities.cpp:50): `to` less `from` [min], the shorter way round the clock; 0 when
## `to` is not given
static func compare_time(from:float, to:float) -> float:
    if to < 0.0:
        return 0.0
    var minutes:float = (to - from) * MINUTES_PER_HOUR
    if minutes < -HALF_DAY_MINUTES:
        minutes += DAY_MINUTES
    if minutes > HALF_DAY_MINUTES:
        minutes -= DAY_MINUTES
    return minutes
