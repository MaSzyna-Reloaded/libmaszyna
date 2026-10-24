@tool
extends RefCounted
class_name MaszynaTimetableFactory

## Timetables out of the original's text tables (TTrainParameters::LoadTTfile(), mtable.cpp:276-642):
## a header with the train, its relation, the braked weight and the locomotive, then two lines a
## station - arrival and departure. The table is read as the original reads it, token by token, and
## what it works out while reading is worked out here: the line speed filled in backwards, an hour
## left out taken from the line above, the first station a stop, the radio channel of each station,
## a shift of every time. Names are made plain ASCII, as the original compares them.
##
## A table becomes a timetable of StationServer, its stations MaszynaStations' of their names; an
## entry's facilities as the table writes them go to its extra (EXTRA_FACILITIES). A file is read once: every run by it is the same timetable, shifted by the run's
## offset (StationServer.run_create()).
##
## Not read: the station announcements (load_sounds(), mtable.cpp:644-671) - see TODO.md.

const EXTENSION:String = ".txt"
## A timetable that is only a number is the train's speed limit, within these (mtable.cpp:283-293)
const NUMERIC_VELOCITY_MIN:float = 10.0
const NUMERIC_VELOCITY_MAX:float = 200.0
## The marks the table is read by
const TOP_BORDER:String = "___________________"
const HEADER_END:String = "_______|"
const TABLE_START:String = "[______________"
const VELOCITY_CHANGE:String = "|_____|"
const TABLE_END:String = "_|_"
const COLUMN:String = "|"
const ROW_START:String = "["
const ROW_END:String = "]"
const HOUR_SEPARATOR:String = "."
const PERCENT:String = "%"
## The track count closes a station's name and its facilities
const TRACK_COUNTS:Array[String] = ["1", "2"]
## A time left out of the table (StationServer.NO_TIME)
const NO_TIME:float = StationServer.NO_TIME
## An entry's extra: its facilities as the table writes them (`R4,RT,H,PP`; TMTableLine,
## mtable.h:23-39). What the original reads of them is the entry's own - its radio channel, a
## technical stop, a change of direction - and the station's type
const EXTRA_FACILITIES:String = "facilities"
## A small passenger stop has "po" at the end of its name or among its facilities, a technical stop
## "pt" there (station.cpp:31-34, mtable.cpp:486); a change of direction is `@` (DirectionChange(),
## mtable.cpp:632-638)
const PASSENGER_STOP_MARK:String = "po"
const TECHNICAL_STOP_MARK:String = "pt"
const DIRECTION_CHANGE_MARK:String = "@"

## The timetables read, by their file's path - a number's by the number
static var _timetables:Dictionary[String, RID] = {}
## A station's facilities name its radio channel (`R4`); a larger number is something else
## (`R307`, mtable.cpp:540-553)
const RADIO_PREFIX:String = "R"
const MAX_RADIO_CHANNEL:int = 10


## The timetable `name` in the scenery's directory, RID() when there is none - read once
static func load_timetable(directory:String, name:String) -> RID:
    var path:String = directory.path_join(MaszynaDataPath.resolve(directory, name + EXTENSION))
    var read:RID = _timetables.get(path if FileAccess.file_exists(path) else name, RID())
    if read.is_valid():
        return read
    if not FileAccess.file_exists(path):
        # no file, but a number: the train's speed
        if not name.is_valid_int():
            return RID()
        var velocity:float = float(name)
        if velocity <= NUMERIC_VELOCITY_MIN or velocity >= NUMERIC_VELOCITY_MAX:
            return RID()
        var numeric:RID = StationServer.timetable_create()
        StationServer.timetable_set_train_name(numeric, name)
        StationServer.timetable_set_speed_limit(numeric, velocity)
        _timetables[name] = numeric
        return numeric
    var timetable:RID = create_timetable(Windows1250.decode(FileAccess.get_file_as_bytes(path)))
    if timetable.is_valid():
        _timetables[path] = timetable
    return timetable


## The timetable the table text describes, RID() when it is no table
static func create_timetable(text:String) -> RID:
    var first_line_end:int = text.find("\n")
    if first_line_end < 0 or not text.substr(0, first_line_end).contains(TOP_BORDER):
        return RID()
    var tokens:PackedStringArray = text.substr(first_line_end).replace("\t", " ").replace("\r", " ").replace("\n", " ").split(" ", false)
    var reader:TableReader = TableReader.new(tokens)
    var timetable:Header = Header.new()

    var token:String = reader.next()
    if token == ROW_START and reader.next() == "Rodzaj":
        reader.skip_to(COLUMN)
    timetable.train_name = reader.next()
    while not reader.at_end():
        token = reader.next()
        if token.contains(HEADER_END):
            break
        if token == "Kategoria":
            reader.skip_to(COLUMN)
            timetable.train_category = reader.next()
        elif token == "Nazwa":
            reader.skip_to(COLUMN)
            timetable.train_label = reader.next()
    reader.skip_to(ROW_START)
    token = reader.next()
    while token == COLUMN:
        token = reader.next()
    timetable.relation_from = Windows1250.to_ascii(token)
    reader.skip_to("Relacja")
    reader.skip_to(COLUMN)
    timetable.relation_to = Windows1250.to_ascii(reader.next())
    reader.skip_to("Wymagany")
    reader.skip_to(COLUMN)
    timetable.brake_ratio = float(reader.next().get_slice(PERCENT, 0))
    reader.skip_to("Seria")
    reader.skip_to(COLUMN)
    timetable.locomotive_series = reader.next()
    timetable.locomotive_load = float(reader.next())
    while not reader.at_end() and not reader.next().contains(TABLE_START):
        pass

    var entries:Array[Line] = []
    var velocity:float = -1.0
    var radio_channel:int = -1
    var table_end:bool = false
    while not table_end and not reader.at_end():
        reader.skip_to(ROW_START)
        if reader.at_end():
            break
        var entry:Line = Line.new()
        entries.append(entry)
        var previous:Line = entries[-2] if entries.size() > 1 else null

        # the arrival line: km | speed | station tracks arrival | running time
        token = reader.next()
        if not token.contains(COLUMN):
            entry.kilometre = float(token)
            token = reader.next()
        velocity = _read_velocity(reader, token, entries, velocity)
        token = reader.token()
        while not token.contains(COLUMN) and not reader.at_end():
            token = reader.next()
        entry.station_name = Windows1250.to_ascii(reader.next())
        token = reader.next()
        while not token in TRACK_COUNTS and not reader.at_end():
            token = reader.next()
        entry.track_count = int(token)
        token = reader.next()
        if not token == COLUMN:
            entry.arrival = _read_time(token, _hour(previous.arrival) if previous else -1)
        token = reader.next()
        while token == COLUMN:
            token = reader.next()
        if not token == ROW_END:
            entry.travel_minutes = float(token)

        # the departure line: | speed | facilities tracks departure | running time
        reader.skip_to(ROW_START)
        token = reader.next()
        if not token.contains(COLUMN):
            token = reader.next()
        velocity = _read_velocity(reader, token, entries, velocity)
        token = reader.token()
        while not token.contains(COLUMN) and not reader.at_end():
            token = reader.next()
        token = reader.next()
        var facilities:String = ""
        while not token in TRACK_COUNTS and not reader.at_end():
            facilities += token
            token = reader.next()
        entry.facilities = facilities
        entry.track_count = int(token)
        radio_channel = _read_radio_channel(entry, radio_channel)
        token = reader.next()
        if token == COLUMN:
            # it leaves when it arrives - the last station too (mtable.cpp:578-582)
            entry.departure = entry.arrival
        else:
            entry.departure = _read_time(token, _hour(previous.departure) if previous else -1)
        token = reader.next()
        while token == COLUMN:
            token = reader.next()
        if not token == ROW_END:
            entry.travel_minutes = float(token)

        # the line under the station: a change of the line speed, or the end of the table
        token = reader.next()
        while not token.contains(ROW_START) and not reader.at_end():
            token = reader.next()
        if not token.contains(TABLE_END):
            token = reader.next()
        if not token.contains(COLUMN):
            token = reader.next()
        velocity = _read_velocity(reader, token, entries, velocity)
        token = reader.token()
        while not token.contains(COLUMN) and not reader.at_end():
            token = reader.next()
        while not token.contains(ROW_END) and not reader.at_end():
            token = reader.next()
        table_end = token.contains(TABLE_END)

    # a train that starts at the first station stops there (mtable.cpp:612-619)
    if entries and entries[0].station_name == timetable.relation_from and entries[0].arrival < 0.0:
        entries[0].arrival = entries[0].departure
    return _create(timetable, entries)


## The timetable of StationServer the table read makes
static func _create(header:Header, lines:Array[Line]) -> RID:
    var timetable:RID = StationServer.timetable_create()
    StationServer.timetable_set_train_name(timetable, header.train_name)
    StationServer.timetable_set_train_category(timetable, header.train_category)
    StationServer.timetable_set_train_label(timetable, header.train_label)
    if header.relation_from:
        StationServer.timetable_set_origin(timetable, MaszynaStations.station_of(header.relation_from))
    if header.relation_to:
        StationServer.timetable_set_destination(timetable, MaszynaStations.station_of(header.relation_to))
    # what the train is to be (TTrainParameters, mtable.h:58-60)
    StationServer.timetable_set_required_locomotive(timetable, header.locomotive_series)
    StationServer.timetable_set_required_mass(timetable, header.locomotive_load)
    StationServer.timetable_set_required_brake_ratio(timetable, header.brake_ratio)
    for line:Line in lines:
        var station:RID = MaszynaStations.station_of(line.station_name)
        if line.station_name.ends_with(PASSENGER_STOP_MARK) or line.facilities.contains(PASSENGER_STOP_MARK):
            StationServer.station_set_type(station, StationServer.STATION_TYPE_PASSENGER_STOP)
        var entry:int = StationServer.timetable_add_entry(timetable, station, line.arrival, line.departure)
        StationServer.timetable_entry_set_technical_stop(timetable, entry, line.facilities.contains(TECHNICAL_STOP_MARK))
        StationServer.timetable_entry_set_direction_change(
                timetable, entry, line.facilities.contains(DIRECTION_CHANGE_MARK))
        StationServer.timetable_entry_set_speed_limit(timetable, entry, line.velocity)
        StationServer.timetable_entry_set_radio_channel(timetable, entry, line.radio_channel)
        StationServer.timetable_entry_set_kilometre(timetable, entry, line.kilometre)
        StationServer.timetable_entry_set_running_time(timetable, entry, line.travel_minutes)
        StationServer.timetable_entry_set_track_count(timetable, entry, line.track_count)
        StationServer.timetable_entry_set_extra(timetable, entry, {EXTRA_FACILITIES: line.facilities})
    return timetable


## The speed column, the token the reader stands on being the one before it: a velocity change mark
## gives the speed read so far to every station without one yet (UpdateVelocity(), mtable.cpp:232-243),
## a number is the new speed
static func _read_velocity(reader:TableReader, token:String, entries:Array[Line], velocity:float) -> float:
    if token.contains(VELOCITY_CHANGE):
        var index:int = entries.size() - 1
        while index >= 0 and entries[index].velocity < 0.0:
            entries[index].velocity = velocity
            index -= 1
        return velocity
    var speed:String = reader.next()
    return velocity if speed.contains(COLUMN) else float(speed)


## `h.mm`, or `mm` with the hour of the line above
static func _read_time(token:String, previous_hour:int) -> float:
    var hour:int = int(token.get_slice(HOUR_SEPARATOR, 0)) if token.contains(HOUR_SEPARATOR) else previous_hour
    var minute:float = float(token.get_slice(HOUR_SEPARATOR, 1)) if token.contains(HOUR_SEPARATOR) else float(token)
    return NO_TIME if hour < 0 else hour + minute / Units.MINUTES_PER_HOUR


static func _hour(time:float) -> int:
    return floori(time) if time >= 0.0 else -1


## The radio channel from the station on (mtable.cpp:535-556): an `R<n>` of its facilities; of two,
## the one not used so far
static func _read_radio_channel(entry:Line, active_channel:int) -> int:
    for facility:String in entry.facilities.split(",", false):
        var number:String = facility.trim_prefix(RADIO_PREFIX)
        if not facility.begins_with(RADIO_PREFIX) or not number.is_valid_int():
            continue
        var channel:int = int(number)
        if channel <= MAX_RADIO_CHANNEL and (entry.radio_channel == -1 or not channel == active_channel):
            entry.radio_channel = channel
    return entry.radio_channel if not entry.radio_channel == -1 else active_channel


## The table's header as it is read (TTrainParameters)
class Header:
    var train_name:String = ""
    var train_category:String = ""
    var train_label:String = ""
    var relation_from:String = ""
    var relation_to:String = ""
    var brake_ratio:float = 0.0
    var locomotive_series:String = ""
    var locomotive_load:float = 0.0


## A station of the table as it is read (TMTableLine): hours since midnight, NO_TIME for none
class Line:
    ## Not read where the table joins it to the column mark ("8.1|"), as the original does not
    ## (mtable.cpp:413-417)
    var kilometre:float = StationServer.NO_KILOMETRE
    var velocity:float = StationServer.NO_SPEED_LIMIT
    var station_name:String = ""
    var facilities:String = ""
    var track_count:int = 1
    var arrival:float = NO_TIME
    var departure:float = NO_TIME
    var travel_minutes:float = 0.0
    var radio_channel:int = StationServer.NO_RADIO_CHANNEL


## The table's tokens, read one after another
class TableReader:
    var _tokens:PackedStringArray
    var _index:int = -1

    func _init(tokens:PackedStringArray) -> void:
        _tokens = tokens

    func next() -> String:
        _index += 1
        return token()

    ## The token last read
    func token() -> String:
        return _tokens[_index] if _index >= 0 and _index < _tokens.size() else ""

    func at_end() -> bool:
        return _index >= _tokens.size() - 1

    func skip_to(wanted:String) -> void:
        while not at_end() and not next() == wanted:
            pass
