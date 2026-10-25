@tool
extends RefCounted
class_name MaszynaStations

## MaSzyna's stations in StationServer: a timetable names its stations and a scenery its passenger
## stops (`PassengerStopPoint:<name>`), both by a name - `_` for a space, as the original compares
## them without the case (Driver.cpp:1101, ToLower()). A station is made of the first name met, a
## space for each `_`; the tracks of a scenery's stops are its stop points while the scenery is
## loaded.

## The station of each name, by its key
static var _stations:Dictionary[String, RID] = {}
## The stations a loaded scenery gave stop points to, by the scenery's node's ObjectID
static var _scenery_stations:Dictionary[int, Array] = {}


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


## A passenger stop of the station on the track, of the scenery `root` is the node of
static func add_stop_point(root:SceneryContentNode, station_name:String, track:RID) -> void:
    var station:RID = station_of(station_name)
    StationServer.station_add_stop_point(station, track)
    if not _scenery_stations.has(root.get_instance_id()):
        root.unloading.connect(release_scenery.bind(root.get_instance_id()), CONNECT_ONE_SHOT)
    var given:Array = _scenery_stations.get_or_add(root.get_instance_id(), [])
    if not station in given:
        given.append(station)


## The scenery is unloaded: the stations have no stop points of its tracks any more
static func release_scenery(root_id:int) -> void:
    for station:RID in _scenery_stations.get(root_id, []):
        StationServer.station_clear_stop_points(station)
    _scenery_stations.erase(root_id)
