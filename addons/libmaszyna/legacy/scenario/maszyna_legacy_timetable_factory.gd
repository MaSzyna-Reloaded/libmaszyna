@tool
extends RefCounted
class_name MaszynaLegacyTimetableFactory

## Timetables out of the original's text tables (TTrainParameters::LoadTTfile(), mtable.cpp:276-642):
## a header with the train, its relation, the braked weight and the locomotive, then two lines a
## station - arrival and departure. The table is read as the original reads it, token by token, and
## what it works out while reading is worked out here: the line speed filled in backwards, an hour
## left out taken from the line above, the first station a stop, the radio channel of each station,
## a shift of every time. Names are made plain ASCII, as the original compares them.
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
## A time left out of the table (TimetableEntry::NO_TIME)
const NO_TIME:float = -1.0
## A station's facilities name its radio channel (`R4`); a larger number is something else
## (`R307`, mtable.cpp:540-553)
const RADIO_PREFIX:String = "R"
const MAX_RADIO_CHANNEL:int = 10


## The timetable `name` in the scenery's directory, null when there is none. Times move by
## `minutes_offset`.
static func load_timetable(directory:String, name:String, minutes_offset:float = 0.0) -> Timetable:
    var path:String = directory.path_join(MaszynaDataPath.resolve(directory, name + EXTENSION))
    if not FileAccess.file_exists(path):
        # no file, but a number: the train's speed
        if not name.is_valid_int():
            return null
        var velocity:float = float(name)
        if velocity <= NUMERIC_VELOCITY_MIN or velocity >= NUMERIC_VELOCITY_MAX:
            return null
        var numeric:Timetable = Timetable.new()
        numeric.train_name = name
        numeric.velocity = velocity
        return numeric
    return create_timetable(Windows1250.decode(FileAccess.get_file_as_bytes(path)), minutes_offset)


## The timetable the table text describes, null when it is no table
static func create_timetable(text:String, minutes_offset:float = 0.0) -> Timetable:
    var first_line_end:int = text.find("\n")
    if first_line_end < 0 or not text.substr(0, first_line_end).contains(TOP_BORDER):
        return null
    var tokens:PackedStringArray = text.substr(first_line_end).replace("\t", " ").replace("\r", " ").replace("\n", " ").split(" ", false)
    var reader:TableReader = TableReader.new(tokens)
    var timetable:Timetable = Timetable.new()

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

    var entries:Array[TimetableEntry] = []
    var velocity:float = -1.0
    var radio_channel:int = -1
    var table_end:bool = false
    while not table_end and not reader.at_end():
        reader.skip_to(ROW_START)
        if reader.at_end():
            break
        var entry:TimetableEntry = TimetableEntry.new()
        entries.append(entry)
        var previous:TimetableEntry = entries[-2] if entries.size() > 1 else null

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
    if entries and entries[0].station_name == timetable.relation_from and not entries[0].is_stop():
        entries[0].arrival = entries[0].departure
    if not minutes_offset == 0.0:
        for entry:TimetableEntry in entries:
            entry.arrival = _shift(entry.arrival, minutes_offset)
            entry.departure = _shift(entry.departure, minutes_offset)
    timetable.entries = entries
    return timetable


## The speed column, the token the reader stands on being the one before it: a velocity change mark
## gives the speed read so far to every station without one yet (UpdateVelocity(), mtable.cpp:232-243),
## a number is the new speed
static func _read_velocity(
    reader:TableReader, token:String, entries:Array[TimetableEntry], velocity:float
) -> float:
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
    return NO_TIME if hour < 0 else hour + minute / LibMaszynaUnits.MINUTES_PER_HOUR


static func _hour(time:float) -> int:
    return floori(time) if time >= 0.0 else -1


## The radio channel from the station on (mtable.cpp:535-556): an `R<n>` of its facilities; of two,
## the one not used so far
static func _read_radio_channel(entry:TimetableEntry, active_channel:int) -> int:
    for facility:String in entry.facilities.split(",", false):
        var number:String = facility.trim_prefix(RADIO_PREFIX)
        if not facility.begins_with(RADIO_PREFIX) or not number.is_valid_int():
            continue
        var channel:int = int(number)
        if channel <= MAX_RADIO_CHANNEL and (entry.radio_channel == -1 or not channel == active_channel):
            entry.radio_channel = channel
    return entry.radio_channel if not entry.radio_channel == -1 else active_channel


static func _shift(time:float, minutes:float) -> float:
    if time < 0.0:
        return time
    return fposmod(time * LibMaszynaUnits.MINUTES_PER_HOUR + minutes, LibMaszynaUnits.MINUTES_PER_DAY) / LibMaszynaUnits.MINUTES_PER_HOUR


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
