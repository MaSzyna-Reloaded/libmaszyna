extends MaszynaGutTest

## MaszynaDriverRoute: how far the limit the trainset is under lasts (VelLimitLastDist,
## TableCheck(), Driver.cpp:945-1057), read from tracks built here.

const Order = MaszynaAIDriver.Order
const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
## Longer than the reading of a standing driver both ways, so no line ends in it [m]
const APPROACH_LENGTH:float = 4000.0
const RESTRICTED_LENGTH:float = 100.0
const RESTRICTED_VELOCITY:float = 40.0
const LINE_VELOCITY:float = 100.0
const SHUNT_SPEED:float = 25.0

var _tracks:Array[RID] = []
## Freed before the nodes they are driven by (autofree), as test_rail_vehicle_track_movement.gd does
var _vehicles:Array[RailVehicle3D] = []


func after_each() -> void:
    for vehicle:RailVehicle3D in _vehicles:
        remove_child(vehicle)
        vehicle.queue_free()
    _vehicles.clear()
    for track:RID in _tracks:
        TrackServer.track_free(track)
    _tracks.clear()
    TrackServer.topology_rebuild()


func test_a_limit_under_the_trainset_lasts_until_it_has_left_it() -> void:
    _track(Vector3(-APPROACH_LENGTH, 0.0, 0.0), Vector3.ZERO, LINE_VELOCITY, "")
    _track(Vector3.ZERO, Vector3(RESTRICTED_LENGTH, 0.0, 0.0), RESTRICTED_VELOCITY, "restricted")
    _track(Vector3(RESTRICTED_LENGTH, 0.0, 0.0), Vector3(RESTRICTED_LENGTH + APPROACH_LENGTH, 0.0, 0.0),
            LINE_VELOCITY, "")
    TrackServer.topology_rebuild()
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()
    var trainset:MaszynaDriverTrainset = await _read(route, "restricted", RESTRICTED_LENGTH / 2.0)

    assert_eq(route.velocity_limit_last, LINE_VELOCITY, "measured against the speed wanted")
    # the middle of the trainset stands in the middle of the section: its front is half the section
    # less half the trainset from the end, and the rear leaves it a trainset later
    assert_almost_eq(route.velocity_limit_last_distance, RESTRICTED_LENGTH / 2.0 + trainset.length / 2.0, 1.0)


func test_a_limit_unbroken_through_the_reading_lasts_beyond_it() -> void:
    _track(Vector3(-APPROACH_LENGTH, 0.0, 0.0), Vector3(APPROACH_LENGTH, 0.0, 0.0), RESTRICTED_VELOCITY, "restricted")
    TrackServer.topology_rebuild()
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()
    await _read(route, "restricted", APPROACH_LENGTH)

    assert_eq(route.velocity_limit_last_distance, MaszynaDriverRoute.LIMIT_BEYOND_RANGE)


func test_no_limit_under_the_trainset_is_none() -> void:
    _track(Vector3(-APPROACH_LENGTH, 0.0, 0.0), Vector3(APPROACH_LENGTH, 0.0, 0.0), LINE_VELOCITY, "line")
    TrackServer.topology_rebuild()
    var route:MaszynaDriverRoute = MaszynaDriverRoute.new()
    await _read(route, "line", APPROACH_LENGTH)

    assert_eq(route.velocity_limit_last_distance, MaszynaDriverRoute.NO_DISTANCE)


func _track(from:Vector3, to:Vector3, velocity:float, name:String) -> void:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = from
    curve.p2 = to
    var track:RID = TrackServer.track_create()
    _tracks.append(track)
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, name, 1.435)
    TrackServer.track_set_velocity(track, velocity)


## A standing SM42 on the named track, and the route read ahead of it wanting LINE_VELOCITY
func _read(route:MaszynaDriverRoute, track_name:String, offset:float) -> MaszynaDriverTrainset:
    var physics_node:VehiclePhysicsNode = build_vehicle_node("RouteLimitTest", SM42)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    add_child(vehicle)
    _vehicles.append(vehicle)
    vehicle.controller_path = vehicle.get_path_to(physics_node)
    await wait_idle_frames(2)
    var trainset:MaszynaDriverTrainset = MaszynaDriverTrainset.new()
    trainset.update(vehicle.get_rid(), 1, true)
    route.update(vehicle.get_rid(), Order.SHUNT, false, SHUNT_SPEED, 0.0, MaszynaDriverSpeed.EASY_ACCELERATION,
            MaszynaDriverSpeed.NO_LIMIT, trainset, MaszynaDriverTimetable.new(), 0.0, SHUNT_SPEED,
            LINE_VELOCITY, false, MaszynaDriverBraking.new(), false)
    return trainset
