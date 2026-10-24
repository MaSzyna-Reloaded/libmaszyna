extends MaszynaGutTest

## The original's tables read into StationServer's timetables: the header to the timetable's name
## and extras, the stations to its entries, MaSzyna's own of them to the entries' extras

const FIXTURES:String = "res://tests/fixtures/timetables"
const MINUTE:float = 1.0 / 60.0


func _extra(timetable:RID, entry:int, key:String) -> Variant:
    return StationServer.timetable_entry_get_extra(timetable, entry).get(key)


func test_the_header_is_read() -> void:
    var timetable:RID = MaszynaTimetableFactory.load_timetable(FIXTURES, "TEST_TIMETABLE")

    assert_eq(StationServer.timetable_get_train_name(timetable), "TST12345")
    assert_eq(StationServer.timetable_get_train_category(timetable), "OS")
    assert_eq(StationServer.timetable_get_train_label(timetable), "Probny")
    assert_eq(StationServer.station_get_name(StationServer.timetable_get_origin(timetable)), "Poczatkowa")
    assert_eq(StationServer.station_get_name(StationServer.timetable_get_destination(timetable)), "Koncowa")
    assert_eq(StationServer.timetable_get_required_brake_ratio(timetable), 55.0)
    assert_eq(StationServer.timetable_get_required_locomotive(timetable), "SN61")
    assert_eq(StationServer.timetable_get_required_mass(timetable), 120.0)
    assert_eq(StationServer.timetable_get_entry_count(timetable), 4)


func test_a_file_is_one_timetable_however_often_it_is_read() -> void:
    assert_eq(MaszynaTimetableFactory.load_timetable(FIXTURES, "test_timetable"),
            MaszynaTimetableFactory.load_timetable(FIXTURES, "TEST_TIMETABLE"))


func test_the_stations_are_read_as_the_original_reads_them() -> void:
    var timetable:RID = MaszynaTimetableFactory.load_timetable(FIXTURES, "test_timetable")

    assert_eq(StationServer.station_get_name(StationServer.timetable_entry_get_station(timetable, 0)), "Poczatkowa")
    assert_eq(StationServer.timetable_entry_get_station(timetable, 0), MaszynaStations.station_of("poczatkowa"),
            "a station of its name, whatever its case")
    assert_eq(StationServer.timetable_entry_get_kilometre(timetable, 0), 10.0)
    assert_almost_eq(StationServer.timetable_entry_get_arrival(timetable, 0), 11.0 + 50.0 * MINUTE, 0.0001)
    assert_almost_eq(StationServer.timetable_entry_get_departure(timetable, 0), 11.0 + 52.0 * MINUTE, 0.0001)
    assert_eq(_extra(timetable, 0, MaszynaTimetableFactory.EXTRA_FACILITIES), "R3,H,PP")
    assert_eq(StationServer.timetable_entry_get_radio_channel(timetable, 0), 3)

    assert_almost_eq(StationServer.timetable_entry_get_arrival(timetable, 1), 11.0 + 58.0 * MINUTE, 0.0001,
            "only minutes: the hour of the line above")
    assert_almost_eq(StationServer.timetable_entry_get_departure(timetable, 1), 11.0 + 59.0 * MINUTE, 0.0001)
    assert_eq(StationServer.timetable_entry_get_track_count(timetable, 1), 2)
    assert_eq(StationServer.timetable_entry_get_running_time(timetable, 1), 6.0)
    assert_eq(StationServer.timetable_entry_get_radio_channel(timetable, 1), 5)

    assert_false(StationServer.timetable_entry_is_stop(timetable, 2), "no arrival time: the train passes")
    assert_almost_eq(StationServer.timetable_entry_get_departure(timetable, 2), 12.0 + 6.0 * MINUTE, 0.0001)
    assert_true((_extra(timetable, 2, MaszynaTimetableFactory.EXTRA_FACILITIES) as String).contains("@"))
    assert_true(StationServer.timetable_entry_is_direction_change(timetable, 2), "`@`: it changes direction there")
    assert_false(StationServer.timetable_entry_is_direction_change(timetable, 1))
    assert_false(StationServer.timetable_entry_is_technical_stop(timetable, 1), "no `pt`")
    assert_eq(StationServer.timetable_entry_get_radio_channel(timetable, 2), -1)

    assert_almost_eq(StationServer.timetable_entry_get_departure(timetable, 3),
            StationServer.timetable_entry_get_arrival(timetable, 3), 0.0001, "no departure: it leaves when it arrives")
    assert_eq(StationServer.timetable_entry_get_radio_channel(timetable, 3), 3, "of two channels, the one not used so far")


func test_the_line_speed_is_filled_in_backwards() -> void:
    var timetable:RID = MaszynaTimetableFactory.load_timetable(FIXTURES, "test_timetable")

    var speeds:Array[float] = []
    for entry:int in StationServer.timetable_get_entry_count(timetable):
        speeds.append(StationServer.timetable_entry_get_speed_limit(timetable, entry))
    var expected:Array[float] = [80.0, 80.0, 60.0, 60.0]
    assert_eq(speeds, expected)


func test_a_drivers_times_are_shifted_across_midnight() -> void:
    var driver_timetable:MaszynaDriverTimetable = MaszynaDriverTimetable.new()
    driver_timetable.take(MaszynaTimetableFactory.load_timetable(FIXTURES, "test_timetable"), RID(), 12 * 60)

    assert_almost_eq(driver_timetable.arrival(0), 23.0 + 50.0 * MINUTE, 0.0001)
    assert_almost_eq(driver_timetable.arrival(3), 0.0 + 15.0 * MINUTE, 0.0001, "past midnight")
    assert_eq(driver_timetable.arrival(2), StationServer.NO_TIME, "a time left out stays left out")


func test_a_number_instead_of_a_timetable_is_the_speed() -> void:
    var timetable:RID = MaszynaTimetableFactory.load_timetable(FIXTURES, "90")

    assert_eq(StationServer.timetable_get_speed_limit(timetable), 90.0)
    assert_eq(StationServer.timetable_get_entry_count(timetable), 0)
    assert_eq(MaszynaTimetableFactory.load_timetable(FIXTURES, "none"), RID(), "no file and no number")
