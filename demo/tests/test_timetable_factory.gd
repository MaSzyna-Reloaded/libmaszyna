extends MaszynaGutTest

const FIXTURES:String = "res://tests/fixtures/timetables"
const MINUTE:float = 1.0 / 60.0


func test_the_header_is_read() -> void:
    var timetable:Timetable = MaszynaLegacyTimetableFactory.load_timetable(FIXTURES, "TEST_TIMETABLE")

    assert_eq(timetable.train_name, "TST12345")
    assert_eq(timetable.train_category, "OS")
    assert_eq(timetable.train_label, "Probny")
    assert_eq(timetable.relation_from, "Poczatkowa")
    assert_eq(timetable.relation_to, "Koncowa")
    assert_eq(timetable.brake_ratio, 55.0)
    assert_eq(timetable.locomotive_series, "SN61")
    assert_eq(timetable.locomotive_load, 120.0)
    assert_eq(timetable.entries.size(), 4)


func test_the_stations_are_read_as_the_original_reads_them() -> void:
    var entries:Array = MaszynaLegacyTimetableFactory.load_timetable(FIXTURES, "test_timetable").entries

    var first:TimetableEntry = entries[0]
    assert_eq(first.station_name, "Poczatkowa")
    assert_eq(first.kilometre, 10.0)
    assert_almost_eq(first.arrival, 11.0 + 50.0 * MINUTE, 0.0001)
    assert_almost_eq(first.departure, 11.0 + 52.0 * MINUTE, 0.0001)
    assert_eq(first.facilities, "R3,H,PP")
    assert_eq(first.radio_channel, 3)

    var second:TimetableEntry = entries[1]
    assert_almost_eq(second.arrival, 11.0 + 58.0 * MINUTE, 0.0001, "only minutes: the hour of the line above")
    assert_almost_eq(second.departure, 11.0 + 59.0 * MINUTE, 0.0001)
    assert_eq(second.track_count, 2)
    assert_eq(second.travel_minutes, 6.0)
    assert_eq(second.radio_channel, 5)

    var passing:TimetableEntry = entries[2]
    assert_false(passing.is_stop(), "no arrival time: the train passes")
    assert_almost_eq(passing.departure, 12.0 + 6.0 * MINUTE, 0.0001)
    assert_true(passing.facilities.contains("@"))
    assert_eq(passing.radio_channel, -1)

    var last:TimetableEntry = entries[3]
    assert_almost_eq(last.departure, last.arrival, 0.0001, "no departure: it leaves when it arrives")
    assert_eq(last.radio_channel, 3, "of two channels, the one not used so far")


func test_the_line_speed_is_filled_in_backwards() -> void:
    var entries:Array = MaszynaLegacyTimetableFactory.load_timetable(FIXTURES, "test_timetable").entries

    var speeds:Array[float] = []
    for entry:TimetableEntry in entries:
        speeds.append(entry.velocity)
    var expected:Array[float] = [80.0, 80.0, 60.0, 60.0]
    assert_eq(speeds, expected)


func test_times_are_shifted_across_midnight() -> void:
    var entries:Array = MaszynaLegacyTimetableFactory.load_timetable(FIXTURES, "test_timetable", 12 * 60).entries

    var first:TimetableEntry = entries[0]
    assert_almost_eq(first.arrival, 23.0 + 50.0 * MINUTE, 0.0001)
    var last:TimetableEntry = entries[3]
    assert_almost_eq(last.arrival, 0.0 + 15.0 * MINUTE, 0.0001, "past midnight")
    var passing:TimetableEntry = entries[2]
    assert_false(passing.is_stop(), "a time left out stays left out")


func test_a_number_instead_of_a_timetable_is_the_speed() -> void:
    var timetable:Timetable = MaszynaLegacyTimetableFactory.load_timetable(FIXTURES, "90")

    assert_eq(timetable.velocity, 90.0)
    assert_eq(timetable.entries.size(), 0)
    assert_null(MaszynaLegacyTimetableFactory.load_timetable(FIXTURES, "none"), "no file and no number")
