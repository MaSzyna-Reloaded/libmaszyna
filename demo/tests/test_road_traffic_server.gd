extends MaszynaGutTest

## RoadTrafficServer: vehicles of its fleet driven along the roads it is given, on their right,
## keeping their distance, going on where a road's end meets another and taken off where it meets
## none; the roads near the camera filled by their length and the density.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## RoadTrafficServer's settings and their defaults (its DENSITY_SETTING, MAX_VEHICLES_SETTING - not
## bound)
const DENSITY_SETTING:String = "maszyna/traffic/density"
const DEFAULT_DENSITY:float = 0.5
const MAX_VEHICLES_SETTING:String = "maszyna/traffic/max_vehicles"
const DEFAULT_MAX_VEHICLES:int = 60
## Any model of the fixtures stands for a car
const DATA_PATH:String = "models/sem/karzelki"
const MODEL:String = "ktmnb"
const VEHICLE_LENGTH:float = 4.0
const VEHICLE_WIDTH:float = 1.8
const ROAD_WIDTH:float = 7.0
const LONG_ROAD:float = 500.0
const DEAD_END_ROAD:float = 30.0
## The side of a square of four roads, each meeting the next at a corner
const LOOP_SIDE:float = 100.0
## How long the vehicles are watched [s]
const DRIVE_TIME:float = 20.0

var _previous_game_dir:String = ""
var _previous_density:float = 0.0
var _previous_max_vehicles:int = 0
var _camera:Camera3D = null
var _roads:Array[RID] = []


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)
    _previous_density = ProjectSettings.get_setting(DENSITY_SETTING, DEFAULT_DENSITY)
    _previous_max_vehicles = ProjectSettings.get_setting(MAX_VEHICLES_SETTING, DEFAULT_MAX_VEHICLES)
    _camera = Camera3D.new()
    add_child_autoqfree(_camera)
    SceneryStreamingServer.streaming_set_camera(_camera.get_instance_id())
    _roads.clear()


func after_each() -> void:
    RoadTrafficServer.road_remove_many(_roads)
    TrackServer.track_free_many(_roads)
    var none:Array[RoadTrafficVehicleType] = []
    RoadTrafficServer.traffic_set_vehicle_types(none)
    ProjectSettings.set_setting(DENSITY_SETTING, _previous_density)
    ProjectSettings.set_setting(MAX_VEHICLES_SETTING, _previous_max_vehicles)
    await Engine.get_main_loop().process_frame
    SceneryStreamingServer.streaming_set_camera(0)
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


## The fleet of `max_vehicles` cars, the roads `density` busy; the settings land a frame after
## they are set
func _start(max_vehicles:int, density:float) -> void:
    ProjectSettings.set_setting(DENSITY_SETTING, density)
    ProjectSettings.set_setting(MAX_VEHICLES_SETTING, max_vehicles)
    await Engine.get_main_loop().process_frame
    var car:RoadTrafficVehicleType = RoadTrafficVehicleType.new()
    car.data_path = DATA_PATH
    car.model_filename = MODEL
    car.length = VEHICLE_LENGTH
    car.width = VEHICLE_WIDTH
    var types:Array[RoadTrafficVehicleType] = [car]
    RoadTrafficServer.traffic_set_vehicle_types(types)


func _road(p1:Vector3, p2:Vector3) -> RID:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    var track:RID = TrackServer.track_create()
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_ROAD, "", ROAD_WIDTH)
    _roads.append(track)
    return track


## Four roads round a square, the camera in its middle
func _loop() -> void:
    for corners:Array in [
            [Vector3.ZERO, Vector3(LOOP_SIDE, 0.0, 0.0)],
            [Vector3(LOOP_SIDE, 0.0, 0.0), Vector3(LOOP_SIDE, 0.0, LOOP_SIDE)],
            [Vector3(LOOP_SIDE, 0.0, LOOP_SIDE), Vector3(0.0, 0.0, LOOP_SIDE)],
            [Vector3(0.0, 0.0, LOOP_SIDE), Vector3.ZERO]]:
        _road(corners[0], corners[1])
    TrackServer.track_connect_many(_roads)
    _camera.global_position = Vector3(LOOP_SIDE, 0.0, LOOP_SIDE) / 2.0
    RoadTrafficServer.road_add_many(_roads)


func test_a_vehicle_drives_along_its_road_on_its_right() -> void:
    await _start(1, 1.0)
    var road:RID = _road(Vector3.ZERO, Vector3(LONG_ROAD, 0.0, 0.0))
    _camera.global_position = Vector3(LONG_ROAD / 2.0, 0.0, 0.0)
    RoadTrafficServer.road_add_many(_roads)
    var vehicles:Array[RID] = RoadTrafficServer.vehicle_get_rids()
    assert_eq(vehicles.size(), 1, "the road near the camera is filled at once")
    if not vehicles:
        return
    var vehicle:RID = vehicles[0]
    assert_eq(RoadTrafficServer.vehicle_get_track(vehicle), road)
    var before:Transform3D = RoadTrafficServer.vehicle_get_transform(vehicle)
    await step(ticks(1.0))
    var after:Transform3D = RoadTrafficServer.vehicle_get_transform(vehicle)
    assert_gt((after.origin - before.origin).dot(after.basis.z), 0.0, "it goes the way it faces")
    assert_gt(RoadTrafficServer.vehicle_get_speed(vehicle), 0.0)
    # the road runs along X; its right is -X of the vehicle's frame
    assert_gt(Vector3(0.0, 0.0, after.origin.z).dot(-after.basis.x), 0.0, "it keeps to the right")


func test_a_vehicle_where_its_road_meets_none_is_taken_off() -> void:
    await _start(1, 1.0)
    _road(Vector3.ZERO, Vector3(DEAD_END_ROAD, 0.0, 0.0))
    RoadTrafficServer.road_add_many(_roads)
    assert_eq(RoadTrafficServer.vehicle_get_rids().size(), 1)
    # the camera stands on the road: no other vehicle comes onto it in sight
    await wait_simulated_until(func() -> bool: return RoadTrafficServer.vehicle_get_rids().is_empty(),
            DRIVE_TIME, "the vehicle taken off at the road's end")


func test_a_vehicle_goes_on_to_the_road_its_end_meets() -> void:
    await _start(1, 1.0)
    _loop()
    var vehicles:Array[RID] = RoadTrafficServer.vehicle_get_rids()
    assert_eq(vehicles.size(), 1)
    if not vehicles:
        return
    var first:RID = RoadTrafficServer.vehicle_get_track(vehicles[0])
    await wait_simulated_until(func() -> bool:
            return not RoadTrafficServer.vehicle_get_track(vehicles[0]) == first,
            DRIVE_TIME, "the vehicle on another road of the square")
    assert_eq(RoadTrafficServer.vehicle_get_rids().size(), 1, "a road of the square leads on")


func test_a_vehicle_keeps_its_distance_to_the_one_ahead() -> void:
    await _start(8, 1.0)
    _loop()
    assert_gt(RoadTrafficServer.vehicle_get_rids().size(), 1)
    for _second:int in int(DRIVE_TIME):
        await step(ticks(1.0))
        var vehicles:Array[RID] = RoadTrafficServer.vehicle_get_rids()
        for first:int in vehicles.size():
            for second:int in range(first + 1, vehicles.size()):
                var a:Transform3D = RoadTrafficServer.vehicle_get_transform(vehicles[first])
                var b:Transform3D = RoadTrafficServer.vehicle_get_transform(vehicles[second])
                # one lane: the same way, the same side of the road
                if a.basis.z.dot(b.basis.z) > 0.9 and a.origin.distance_to(b.origin) < LOOP_SIDE / 2.0:
                    assert_gte(a.origin.distance_to(b.origin), VEHICLE_LENGTH, "the vehicles do not overlap")


func test_roads_removed_take_their_vehicles_off() -> void:
    await _start(4, 1.0)
    _loop()
    assert_gt(RoadTrafficServer.vehicle_get_rids().size(), 0)
    RoadTrafficServer.road_remove_many(_roads)
    assert_eq(RoadTrafficServer.vehicle_get_rids().size(), 0)


func test_no_vehicle_drives_at_no_density() -> void:
    await _start(4, 0.0)
    _loop()
    await step(ticks(2.0))
    assert_eq(RoadTrafficServer.vehicle_get_rids().size(), 0)


func test_the_fleet_bounds_the_traffic() -> void:
    await _start(3, 1.0)
    _road(Vector3.ZERO, Vector3(LONG_ROAD, 0.0, 0.0))
    _camera.global_position = Vector3(LONG_ROAD / 2.0, 0.0, 0.0)
    RoadTrafficServer.road_add_many(_roads)
    assert_eq(RoadTrafficServer.vehicle_get_rids().size(), 3, "the road wants more than the fleet has")


func test_the_traffic_stands_while_the_simulation_does() -> void:
    await _start(1, 1.0)
    _loop()
    var vehicles:Array[RID] = RoadTrafficServer.vehicle_get_rids()
    assert_eq(vehicles.size(), 1)
    if not vehicles:
        return
    var before:Transform3D = RoadTrafficServer.vehicle_get_transform(vehicles[0])
    for _frame:int in ticks(1.0):
        await Engine.get_main_loop().process_frame
    assert_eq(RoadTrafficServer.vehicle_get_transform(vehicles[0]), before)
