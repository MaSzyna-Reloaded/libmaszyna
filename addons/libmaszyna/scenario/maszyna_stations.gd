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
