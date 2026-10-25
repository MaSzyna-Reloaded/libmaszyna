extends RefCounted
class_name MaszynaRoadNetwork

## A MaSzyna scenery's roads as RoadServer's network: a road along every `road` track, laid along
## it; the six ways across every `cross` (TTrack's crossroad segments, Track.cpp:698-782), one of
## which a car takes at random (Driver.cpp:657-697); every road's end joined to what its track's end
## meets; and at every road's end that meets nothing the invisible loop the original lays there for
## a car to turn back by (TTrack::NullCreate(), Track.cpp:271-311).

## Original engine: the loop at a road's end - two curves, each 20 m out along the road and 20 m
## aside, taken at 20 km/h (Track.cpp:282-306)
const LOOP_REACH:float = 20.0
const LOOP_VELOCITY:float = 20.0
## Two corners of a crossroad this close [m] are one: a crossroad of three roads (Track.cpp:773,
## length2 < 0.01)
const CORNER_MERGE_DISTANCE_SQUARED:float = 0.01
## How far back from a road's end its way out is taken [m]
const END_HEADING_DISTANCE:float = 1.0
## A track's ends, as a road's
const ROAD_ENDPOINTS:Dictionary[RoadServer.RoadEnd, TrackServer.EndpointIndex] = {
    RoadServer.ROAD_END_START: TrackServer.CURVE1_P1,
    RoadServer.ROAD_END_END: TrackServer.CURVE1_P2,
}


## The roads of `tracks`, registered as `track_rids` (SceneryServer, in their order), joined into a
## network - their handles, for the scenery to free (SceneryContentNode.roads_own())
static func build(tracks:Array[TrackData], track_rids:Array[RID]) -> Array[RID]:
    var built:Array[RID] = []
    # what stands at each end of a track: [road, its end] of the road along it, or of a crossroad's
    # ways meeting at that corner
    var ends:Dictionary[RID, Dictionary] = {}
    var road_tracks:Array[RID] = []
    var crosses:Array[RID] = []
    for index:int in tracks.size():
        var data:TrackData = tracks[index]
        var track:RID = track_rids[index]
        if not track.is_valid():
            continue
        if data.kind == TrackServer.TRACK_ROAD:
            var road:RID = _road(TrackServer.track_get_domain_curve(track), data.width, built)
            RoadServer.road_set_track(road, track)
            ends[track] = {
                TrackServer.CURVE1_P1: [[road, RoadServer.ROAD_END_START]],
                TrackServer.CURVE1_P2: [[road, RoadServer.ROAD_END_END]],
            }
            road_tracks.append(track)
        elif data.kind == TrackServer.TRACK_CROSS:
            ends[track] = _cross(data, TrackServer.track_get_velocity(track), built)
            crosses.append(track)
    # every end joined to what its track's end meets (TTrack::Connect*(), Track.cpp:3436-3497)
    for track:RID in road_tracks + crosses:
        for endpoint:int in ends[track]:
            for connection:TrackEndpointRef in TrackServer.track_get_endpoint_connections(track, endpoint):
                var met:Array = ends.get(connection.track_rid, {}).get(connection.endpoint_index, [])
                for own:Array in ends[track][endpoint]:
                    for other:Array in met:
                        RoadServer.road_link(own[0], own[1], other[0], other[1])
    # a road's end that leads nowhere turns back by a loop; a crossroad's corner stays a dead end, as
    # its loop in the original is never found again (Track.cpp:2579-2585)
    for track:RID in road_tracks:
        for end:RoadServer.RoadEnd in ROAD_ENDPOINTS:
            var road:RID = ends[track][ROAD_ENDPOINTS[end]][0][0]
            if not RoadServer.road_get_links(road, end):
                _loop(road, end, built)
    return built


static func _road(curve:Curve3D, width:float, built:Array[RID]) -> RID:
    var road:RID = RoadServer.road_create()
    RoadServer.road_set_curve(road, curve)
    RoadServer.road_set_width(road, width)
    built.append(road)
    return road


## A path from `from` to `to`, leaving and entering by the handles - each relative to its end and
## pointing into the path
static func _path(from:Vector3, from_handle:Vector3, to:Vector3, to_handle:Vector3) -> Curve3D:
    var curve:Curve3D = Curve3D.new()
    curve.add_point(from, Vector3.ZERO, from_handle)
    curve.add_point(to, to_handle, Vector3.ZERO)
    return curve


## The ways across a crossroad, one between every two of its corners - the straight ones its two
## curves and the turns between their ends (Track.cpp:698, 767-782), its corners' handles pointing in;
## at each corner [road, its end] of the ways meeting there, by the track's endpoints
static func _cross(data:TrackData, velocity:float, built:Array[RID]) -> Dictionary:
    var curves:Array[TrackCurve] = [data.curve, data.diverging_curve]
    var corners:Array[Dictionary] = []
    var corner_of:Dictionary[int, int] = {}
    for endpoint:int in [TrackServer.CURVE1_P1, TrackServer.CURVE1_P2, TrackServer.CURVE2_P1, TrackServer.CURVE2_P2]:
        var curve:TrackCurve = curves[1 if endpoint >= TrackServer.CURVE2_P1 else 0]
        var at_start:bool = endpoint == TrackServer.CURVE1_P1 or endpoint == TrackServer.CURVE2_P1
        var point:Vector3 = curve.p1 if at_start else curve.p2
        var handle:Vector3 = curve.c1 if at_start else curve.c2
        var merged:int = corners.find_custom(func(corner:Dictionary) -> bool:
                return (corner["point"] as Vector3).distance_squared_to(point) < CORNER_MERGE_DISTANCE_SQUARED)
        if merged >= 0:
            corner_of[endpoint] = merged
            continue
        corner_of[endpoint] = corners.size()
        corners.append({"point": point, "handle": handle, "ways": []})
    for first:int in corners.size():
        for second:int in range(first + 1, corners.size()):
            var road:RID = _road(_path(corners[first]["point"], corners[first]["handle"],
                    corners[second]["point"], corners[second]["handle"]), data.width, built)
            RoadServer.road_set_velocity(road, velocity)
            corners[first]["ways"].append([road, RoadServer.ROAD_END_START])
            corners[second]["ways"].append([road, RoadServer.ROAD_END_END])
    var ends:Dictionary = {}
    for endpoint:int in corner_of:
        ends[endpoint] = corners[corner_of[endpoint]]["ways"]
    return ends


## Track.cpp:271-311: from the road's end two curves 40 m out, 20 m to either side between, meeting
## at their far ends - out along the one, back along the other, onto the same road the other way
static func _loop(road:RID, end:RoadServer.RoadEnd, built:Array[RID]) -> void:
    var length:float = RoadServer.road_get_length(road)
    var at:float = length if end == RoadServer.ROAD_END_END else 0.0
    var inside:float = length - END_HEADING_DISTANCE if end == RoadServer.ROAD_END_END else END_HEADING_DISTANCE
    var start:Vector3 = RoadServer.road_get_position(road, at)
    var way_out:Vector3 = start - RoadServer.road_get_position(road, clampf(inside, 0.0, length))
    # a road of no length has no way out to turn back by
    if way_out.is_zero_approx():
        return
    var reach:Vector3 = LOOP_REACH * way_out.normalized()
    var far:Vector3 = start + reach + reach
    var width:float = RoadServer.road_get_width(road)
    var out:RID = _road(_path(start, reach, far, Vector3(-reach.z, reach.y, reach.x)), width, built)
    var back:RID = _road(_path(start, reach, far, Vector3(reach.z, reach.y, -reach.x)), width, built)
    for loop:RID in [out, back]:
        RoadServer.road_set_velocity(loop, LOOP_VELOCITY)
    RoadServer.road_link(road, end, out, RoadServer.ROAD_END_START)
    RoadServer.road_link(out, RoadServer.ROAD_END_END, back, RoadServer.ROAD_END_END)
    RoadServer.road_link(back, RoadServer.ROAD_END_START, road, end)
