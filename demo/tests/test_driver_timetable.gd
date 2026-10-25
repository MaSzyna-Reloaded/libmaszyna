extends MaszynaGutTest

## MaszynaDriverTimetable: how far a driver got through its timetable (TTrainParameters'
## StationIndex, NextStationName, UpdateMTable(), IsTimeToGo(), mtable.cpp:36-190).

const FIRST_VELOCITY:float = 40.0
const SECOND_VELOCITY:float = 70.0
## 10:30, and a minute either side of it
const DEPARTURE:float = 10.5
const MINUTE:float = 1.0 / 60.0
## A time left out of the table
const NO_TIME:float = StationServer.NO_TIME
## Float rounding of the hours-to-minutes conversion
const EPSILON:float = 1e-6
## The delay cases [min]: the stop's dwell (the arrival before the departure), a train late or
## early at the arrival, standing past the departure, late on leaving, and some time later on the way
const DWELL_MINUTES:int = 5
const ARRIVAL_LATE_MINUTES:int = 3
const ARRIVAL_EARLY_MINUTES:int = 3
const STANDING_LATE_MINUTES:int = 2
const DEPARTURE_LATE_MINUTES:int = 4
const LATER_ON_MINUTES:int = 10
## Less than a minute more, counted as none
const PART_OF_A_MINUTE:float = 0.5

var timetable:MaszynaDriverTimetable
var _timetables:Array[RID] = []


func before_each():
    timetable = MaszynaDriverTimetable.new()
    _take(DEPARTURE, DEPARTURE)


func after_each():
    timetable.finish()
    for made:RID in _timetables:
        StationServer.timetable_free(made)
    _timetables.clear()


## The timetable of the tests, its first station arrived at and left as given: the one it only
## passes has no arrival; the last stops
func _take(first_arrival:float, first_departure:float) -> void:
    var made:RID = StationServer.timetable_create()
    _timetables.append(made)
    _entry(made, "Start", FIRST_VELOCITY, first_arrival, first_departure, false)
    _entry(made, "Through", SECOND_VELOCITY, NO_TIME, NO_TIME, false)
    _entry(made, "Turn", SECOND_VELOCITY, DEPARTURE, DEPARTURE, true)
    _entry(made, "End", SECOND_VELOCITY, DEPARTURE, NO_TIME, false)
    timetable.take(made)


func test_it_starts_for_the_first_station():
    assert_eq(timetable.next_stop, "Start")
    assert_eq(timetable.velocity, FIRST_VELOCITY)
    assert_true(timetable.is_stop())


func test_arriving_counts_once_and_leaving_moves_on():
    assert_true(timetable.arrive(DEPARTURE), "arrived")
    assert_false(timetable.arrive(DEPARTURE), "only once while it stands")
    assert_eq(timetable.next_stop, "Start", "it stops there until it leaves")
    assert_eq(timetable.velocity, SECOND_VELOCITY, "the speed on to the next station")
    timetable.advance()
    assert_eq(timetable.next_stop, "Through")
    assert_false(timetable.is_stop(), "it only passes the next one")


func test_latency_is_early_positive_late_negative():
    # LastStationLatency is the departure less the arrival (mtable.cpp:122)
    timetable.arrive(DEPARTURE - MINUTE)
    assert_almost_eq(timetable.latency, 1.0, EPSILON, "a minute early")
    timetable.take(timetable.timetable)
    timetable.arrive(DEPARTURE + MINUTE)
    assert_almost_eq(timetable.latency, -1.0, EPSILON, "a minute late")


func test_the_station_left_stays_shown_until_the_train_is_clear_of_it():
    # StationStart (driveruipanels.cpp:392) follows StationIndex only in UpdateNextStop()
    timetable.arrive(DEPARTURE)
    timetable.advance()
    assert_eq(timetable.station_start, 0, "still at the station it has left")
    timetable.show_next_station(DEPARTURE)
    assert_eq(timetable.station_start, 1, "clear of it")
    timetable.rewind("End")
    assert_eq(timetable.station_start, 3, "shown from the station it rewound to (Driver.cpp:1092)")


func test_the_delay_is_the_arrival_then_the_departure():
    _take(DEPARTURE - DWELL_MINUTES * MINUTE, DEPARTURE)
    timetable.arrive(DEPARTURE - (DWELL_MINUTES - ARRIVAL_LATE_MINUTES) * MINUTE)
    assert_almost_eq(timetable.delay, float(ARRIVAL_LATE_MINUTES), EPSILON, "late at the arrival")
    assert_true(timetable.arrived)
    timetable.advance()
    assert_false(timetable.arrived)
    timetable.show_next_station(DEPARTURE + DEPARTURE_LATE_MINUTES * MINUTE)
    assert_almost_eq(timetable.delay, float(DEPARTURE_LATE_MINUTES), EPSILON, "late at the departure")


func test_the_delay_counts_on_while_the_train_stands_past_its_departure():
    _take(DEPARTURE - DWELL_MINUTES * MINUTE, DEPARTURE)
    timetable.arrive(DEPARTURE - (DWELL_MINUTES + ARRIVAL_EARLY_MINUTES) * MINUTE)
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), DEPARTURE - MINUTE), 0, "early, it waits for the departure")
    var standing:float = DEPARTURE + (STANDING_LATE_MINUTES + PART_OF_A_MINUTE) * MINUTE
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), standing), STANDING_LATE_MINUTES, "whole minutes past it")
    timetable.advance()
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), standing), STANDING_LATE_MINUTES, "still at the station it has left")
    timetable.show_next_station(DEPARTURE + DEPARTURE_LATE_MINUTES * MINUTE)
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), DEPARTURE + LATER_ON_MINUTES * MINUTE), DEPARTURE_LATE_MINUTES,
            "on the way: as it left")


func test_the_delay_of_a_late_arrival_stays_until_the_departure():
    _take(DEPARTURE - DWELL_MINUTES * MINUTE, DEPARTURE)
    timetable.arrive(DEPARTURE - (DWELL_MINUTES - ARRIVAL_LATE_MINUTES) * MINUTE)
    assert_eq(MaszynaDriverTimetable.state_delay_minutes(_state(), DEPARTURE - MINUTE), ARRIVAL_LATE_MINUTES,
            "late at the arrival, before the departure")


func test_it_leaves_at_the_departure_time():
    assert_false(timetable.is_time_to_go(DEPARTURE - MINUTE))
    assert_true(timetable.is_time_to_go(DEPARTURE))
    assert_true(timetable.is_time_to_go(DEPARTURE + MINUTE))


func test_midnight_is_not_twelve_hours_away():
    _take(DEPARTURE, MINUTE)
    assert_false(timetable.is_time_to_go(24.0 - MINUTE), "a minute before midnight is early")


func test_it_turns_where_the_table_says_and_not_at_the_last():
    timetable.rewind("turn")
    assert_eq(timetable.next_stop, "Turn", "the name is matched regardless of case")
    assert_true(timetable.turns_here())
    timetable.arrive(DEPARTURE)
    timetable.advance()
    assert_true(timetable.is_last_station())
    assert_false(timetable.is_time_to_go(DEPARTURE), "never from the last")
    assert_false(timetable.turns_here())


func test_without_a_timetable_there_is_no_limit():
    timetable.finish()
    assert_eq(timetable.velocity, MaszynaDriverSpeed.NO_LIMIT)
    assert_eq(timetable.next_stop, "")
    assert_true(timetable.is_stop(), "past the end it always stops")


func test_every_step_through_it_is_announced():
    watch_signals(timetable)
    timetable.arrive(DEPARTURE)
    timetable.arrive(DEPARTURE)
    timetable.advance()
    timetable.rewind("End")
    timetable.rewind("Nowhere")
    timetable.finish()
    assert_signal_emit_count(timetable, "changed", 4, "arrival, leaving, a rewind and the end - not a repeated arrival nor an unknown station")


## The timetable's state (MaszynaDriverTimetable.state_is_standing())
func _state() -> Dictionary:
    return {
        "timetable": timetable.timetable,
        "offset": timetable.offset,
        "station_index": timetable.station_index,
        "station_start": timetable.station_start,
        "latency": timetable.latency,
        "delay": timetable.delay,
        "arrived": timetable.arrived,
    }


func _entry(made:RID, station:String, velocity:float, arrival:float, departure:float, direction_change:bool) -> void:
    var entry:int = StationServer.timetable_add_entry(made, MaszynaStations.station_of(station), arrival, departure)
    StationServer.timetable_entry_set_speed_limit(made, entry, velocity)
    StationServer.timetable_entry_set_direction_change(made, entry, direction_change)
