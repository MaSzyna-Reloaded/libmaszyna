@tool
extends RefCounted
class_name MaszynaStations

## MaSzyna's stations in StationServer: a timetable names its stations and a scenery its passenger
## stops (`PassengerStopPoint:<name>`), both by a name - `_` for a space, as the original compares
## them without the case (Driver.cpp:1101, ToLower()). A station is made of the first name met, a
## space for each `_`.

## Between the station and the track of a vehicle's destination, "(stacja):(tor)" (DynObj.h:216)
const DESTINATION_TRACK_SEPARATOR:String = ":"

## The station of each name, by its key
static var _stations:Dictionary[String, RID] = {}


## What two names of one station have alike
static func key(station_name:String) -> String:
    return station_name.replace("_", " ").to_lower()


## The station of the name, made the first time it is named
static func station_of(station_name:String) -> RID:
    var station_key:String = key(station_name)
    var station:RID = _stations.get(station_key, RID())
    if station.is_valid():
        return station
    station = StationServer.station_create()
    StationServer.station_set_name(station, station_name.replace("_", " "))
    _stations[station_key] = station
    return station


## A scenery's stations, for the core's list of them (CompiledScenery.stations): MaSzyna has no
## station of its own, only its passenger stop indicators (W4) - an event `putvalues <x> <y> <z>
## PassengerStopPoint:<name>#<indicator>` (pkp/wskazniki/w4.inc) or a memory holding that command
## (MemCell.cpp:74-76). A station is made of each name they give, in the middle of its indicators.
## A station with none (a freight or a technical one) is left out: a timetable names it, but gives
## no place.
static func stations_of_stop_points(
    events:Array[MaszynaEventData], memcells:Array[MaszynaMemcellData]
) -> Array[StationData]:
    var stations:Dictionary[String, StationData] = {}
    var position_sums:Dictionary[String, Vector3] = {}
    var indicator_counts:Dictionary[String, int] = {}
    var stop_points:Array[Array] = []
    for event:MaszynaEventData in events:
        # <x> <y> <z> <command> <value1> <value2>, the origin moving it, not the rotation
        # (Event.cpp:700-767, MaszynaEventFactory)
        if event.type == "putvalues" and event.parameters.size() > 3:
            stop_points.append([event.parameters[3], event.origin + Vector3(
                float(event.parameters[0]), float(event.parameters[1]), float(event.parameters[2])
            )])
    for memcell:MaszynaMemcellData in memcells:
        stop_points.append([memcell.text, memcell.position])
    for stop_point:Array in stop_points:
        var command:String = stop_point[0]
        if not command.begins_with(MaszynaEventFactory.PASSENGER_STOP_POINT):
            continue
        # the name as the timetable gives it, without the indicator's own mark past `#`
        var stop_name:String = command.trim_prefix(MaszynaEventFactory.PASSENGER_STOP_POINT).get_slice(
            MaszynaEventFactory.STOP_POINT_UNIQUE, 0
        )
        if stop_name.is_empty():
            continue
        var station_key:String = key(stop_name)
        if not stations.has(station_key):
            var station:StationData = StationData.new()
            station.station_name = stop_name.replace("_", " ")
            stations[station_key] = station
            position_sums[station_key] = Vector3.ZERO
            indicator_counts[station_key] = 0
        position_sums[station_key] += stop_point[1] as Vector3
        indicator_counts[station_key] += 1
    var result:Array[StationData] = []
    for station_key:String in stations:
        stations[station_key].position = position_sums[station_key] / indicator_counts[station_key]
        result.append(stations[station_key])
    return result
