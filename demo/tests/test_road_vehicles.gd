extends MaszynaGutTest

## MaSzyna's road cars: a FIZ of the road category read as a RoadVehicleController driven as the
## original drives a car, the scenery's roads joined into RoadServer's network - crossroads and the
## loops at their ends - and the cars' driver slowing for the memories its roads' events read

const ROAD_CAR_FIZ:String = "res://tests/fixtures/test_road_car.fiz"
const RAIL_VEHICLE_FIZ:String = "res://tests/fixtures/test_vehicle.fiz"
## The roads' width [m]
const ROAD_WIDTH:float = 7.0
## The scenery's velocity of the car [km/h]
const CAR_VELOCITY:float = 50.0
## Where the car is told to stop, along the road [m]
const STOP_AT:float = 300.0
## How close a place is taken as reached [m]
const PLACE_TOLERANCE:float = 0.5
## Simulated seconds long enough for the car to have stopped, and to move off again
const STOPPING_TIME:float = 40.0
## Where a car stands along its road [m], and how near it the camera showing it has to be - the
## show distance and the eye height of PlayerCameraServer and some [m]
const SHOWN_AT:float = 30.0
const SHOW_REACH:float = 50.0
## Frames for the follow view to come to its view of a car, and how near it ends up [m] - the
## drive-by view stands 50 m ahead of the cab (driver_mode::DistantView(), drivermode.cpp:904-907)
const FOLLOW_FRAMES:int = 120
const FOLLOW_REACH:float = 80.0
const MOVING_OFF_TIME:float = 10.0

var _tracks:Array[RID] = []
var _roads:Array[RID] = []
var _vehicles:Array[RID] = []
var _controllers:Array[RID] = []
var _event_system:RID


func before_each() -> void:
    _event_system = ScenarioEventServer.system_create()
    ScenarioEventServer.system_attach_implementation(_event_system, MaszynaScenarioEventServer.implementation_get())


func after_each() -> void:
    for vehicle:RID in _vehicles:
        VehicleServer.vehicle_free(vehicle)
    for controller:RID in _controllers:
        VehicleServer.controller_free(controller)
    RoadServer.road_free_many(_roads)
    ScenarioEventServer.system_free(_event_system)
    TrackServer.track_free_many(_tracks)
    TrackServer.topology_rebuild()
    _vehicles.clear()
    _controllers.clear()
    _roads.clear()
    _tracks.clear()


func _curve(from:Vector3, to:Vector3, from_handle:Vector3 = Vector3.ZERO, to_handle:Vector3 = Vector3.ZERO) -> TrackCurve:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = from
    curve.c1 = from_handle
    curve.c2 = to_handle
    curve.p2 = to
    return curve


## The tracks registered as a scenery registers them, and the road network built of them
func _network(tracks:Array[TrackData]) -> Array[RID]:
    var rids:Array[RID] = []
    for data:TrackData in tracks:
        var track:RID = TrackServer.track_create()
        TrackServer.track_update_curves(track, data.curve, data.diverging_curve)
        TrackServer.track_update(track, data.kind, "", data.width)
        rids.append(track)
        _tracks.append(track)
    TrackServer.topology_rebuild()
    _roads.append_array(MaszynaRoadNetwork.build(tracks, rids))
    return rids


func _road_track(from:Vector3, to:Vector3) -> TrackData:
    var data:TrackData = TrackData.new()
    data.kind = TrackServer.TRACK_ROAD
    data.curve = _curve(from, to)
    data.width = ROAD_WIDTH
    return data


## The roads a road's end leads on to
func _next_roads(road:RID, end:RoadServer.RoadEnd) -> Array[RID]:
    var roads:Array[RID] = []
    for link:Dictionary in RoadServer.road_get_links(road, end):
        roads.append(link["road"])
    return roads


## A car of the fixture's FIZ standing `offset` along `road`, facing its end, driven when `driven`
func _car(road:RID, offset:float, driven:bool) -> RID:
    var vehicle:RID = VehicleServer.vehicle_create()
    var controller:RID = VehicleServer.controller_create()
    _vehicles.append(vehicle)
    _controllers.append(controller)
    RoadVehicleServer.vehicle_attach(vehicle)
    VehicleServer.controller_configure(controller, FizVehicleBuilder.build_description_at(ROAD_CAR_FIZ))
    VehicleServer.vehicle_bind_controller(vehicle, controller)
    RoadVehicleServer.vehicle_set_road(vehicle, road, offset, RoadServer.ROAD_END_END)
    if driven:
        MaszynaCarDriver.vehicle_attach(vehicle, CAR_VELOCITY)
    return vehicle


func _x(vehicle:RID) -> float:
    return RoadVehicleServer.vehicle_get_transform(vehicle).origin.x


## A car is shown where it stands: VehicleServer knows where every vehicle is (libreloaded#7 - the
## show camera asked RailVehicleServer and went to the world's origin)
func test_a_car_is_shown_where_it_stands() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(100, 0, 0))]
    var car:RID = _car(RoadServer.track_get_road(_network(tracks)[0]), SHOWN_AT, false)

    var stands:Transform3D = VehicleServer.vehicle_get_transform(car)
    assert_true(stands.is_equal_approx(RoadVehicleServer.vehicle_get_transform(car)), "as the road places it")
    assert_almost_eq(stands.origin.x, SHOWN_AT, PLACE_TOLERANCE)
    assert_lt(PlayerCameraServer.camera_get_show_transform(car).origin.distance_to(stands.origin), SHOW_REACH,
            "the camera beside the car")


## A car is followed in every view: it is a trainset of its own (libreloaded#7 - the follow view
## took a car's controller for a rail one's and stopped on a null)
func test_a_car_is_followed_in_every_view() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(100, 0, 0))]
    var car:RID = _car(RoadServer.track_get_road(_network(tracks)[0]), SHOWN_AT, false)
    var follow:FollowView = FollowView.new()
    add_child_autofree(follow)
    var stands:Vector3 = VehicleServer.vehicle_get_transform(car).origin

    for view:FollowView.View in [FollowView.View.TRAINSET_FRONT, FollowView.View.TRAINSET_REAR,
            FollowView.View.BOGIE, FollowView.View.DRIVEBY]:
        follow.view = view
        follow.activate(car, PlayerCameraServer.camera_get_show_transform(car))
        await wait_idle_frames(FOLLOW_FRAMES)
        assert_lt(follow.global_position.distance_to(stands), FOLLOW_REACH, "view %d by the car" % view)


func test_a_road_category_fiz_is_a_road_vehicle_driven_as_a_car() -> void:
    var car:RoadVehicleController = FizVehicleBuilder.build_description_at(ROAD_CAR_FIZ) as RoadVehicleController

    assert_not_null(car, "Category=road is a road vehicle")
    assert_eq(car.mass, 1200.0)
    assert_eq(car.max_velocity, 140.0)
    assert_eq(car.power, 60.0)
    assert_eq(car.dimensions_length, 4.2)
    assert_eq(car.dimensions_width, 1.7)
    assert_eq(car.wheel_diameter, 0.6)
    assert_eq(car.transmission_ratio, 5.0, "Trans=1:5, the wheel's teeth over the motor's")
    assert_eq(car.following_gap, FizVehicleBuilder.CAR_FOLLOWING_GAP)
    assert_eq(car.acceleration_limit, FizVehicleBuilder.CAR_ACCELERATION_LIMIT)
    assert_eq(car.obstacle_range, FizVehicleBuilder.CAR_OBSTACLE_RANGE)
    assert_eq(car.components.size(), 0, "no part of a rail vehicle")
    assert_true(FizVehicleBuilder.build_description_at(RAIL_VEHICLE_FIZ) is MaszynaMoverRailVehicleController,
            "a train stays on the Mover")


func test_roads_joined_end_to_end_lead_on_to_each_other() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(50, 0, 0)),
            _road_track(Vector3(50, 0, 0), Vector3(100, 0, 0))]
    var rids:Array[RID] = _network(tracks)
    var first:RID = RoadServer.track_get_road(rids[0])
    var second:RID = RoadServer.track_get_road(rids[1])

    assert_eq(_next_roads(first, RoadServer.ROAD_END_END), [second] as Array[RID])
    assert_eq(_next_roads(second, RoadServer.ROAD_END_START), [first] as Array[RID])
    assert_eq(RoadServer.road_get_track(first), rids[0], "a road is laid along its track")


func test_a_road_s_open_end_turns_back_by_a_loop() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(50, 0, 0))]
    var road:RID = RoadServer.track_get_road(_network(tracks)[0])

    var out:Array[RID] = _next_roads(road, RoadServer.ROAD_END_END)
    assert_eq(out.size(), 1, "one way out at the end")
    assert_eq(RoadServer.road_get_velocity(out[0]), MaszynaRoadNetwork.LOOP_VELOCITY)
    var back:Array[Dictionary] = RoadServer.road_get_links(out[0], RoadServer.ROAD_END_END)
    assert_eq(back.size(), 1)
    var returning:Array[Dictionary] = RoadServer.road_get_links(back[0]["road"], RoadServer.ROAD_END_START)
    assert_eq(returning.size(), 1)
    assert_eq(returning[0]["road"], road, "back onto its own road")
    assert_eq(returning[0]["end"], RoadServer.ROAD_END_END, "entered by the end it left by")
    assert_almost_eq(RoadServer.road_get_position(out[0], RoadServer.road_get_length(out[0])).x,
            50.0 + 2.0 * MaszynaRoadNetwork.LOOP_REACH, PLACE_TOLERANCE, "it reaches 40 m out")


func test_a_crossroad_leads_from_each_arm_to_the_other_three() -> void:
    var cross:TrackData = TrackData.new()
    cross.kind = TrackServer.TRACK_CROSS
    cross.width = ROAD_WIDTH
    cross.curve = _curve(Vector3(-10, 0, 0), Vector3(10, 0, 0), Vector3(5, 0, 0), Vector3(-5, 0, 0))
    cross.diverging_curve = _curve(Vector3(0, 0, -10), Vector3(0, 0, 10), Vector3(0, 0, 5), Vector3(0, 0, -5))
    var tracks:Array[TrackData] = [cross,
            _road_track(Vector3(-60, 0, 0), Vector3(-10, 0, 0)),
            _road_track(Vector3(10, 0, 0), Vector3(60, 0, 0)),
            _road_track(Vector3(0, 0, -60), Vector3(0, 0, -10)),
            _road_track(Vector3(0, 0, 10), Vector3(0, 0, 60))]
    var rids:Array[RID] = _network(tracks)
    var west:RID = RoadServer.track_get_road(rids[1])
    var east:RID = RoadServer.track_get_road(rids[2])

    var ways:Array[RID] = _next_roads(west, RoadServer.ROAD_END_END)
    assert_eq(ways.size(), 3, "left, straight on and right")
    var leading_east:int = 0
    for way:RID in ways:
        for end:RoadServer.RoadEnd in [RoadServer.ROAD_END_START, RoadServer.ROAD_END_END]:
            if _next_roads(way, end).has(east):
                leading_east += 1
    assert_eq(leading_east, 1, "one of them comes out at the east arm")


func test_the_driver_stops_for_a_memory_saying_stop_and_moves_off_when_it_clears() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(STOP_AT, 0, 0)),
            _road_track(Vector3(STOP_AT, 0, 0), Vector3(1000, 0, 0))]
    var rids:Array[RID] = _network(tracks)
    var memory:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    MaszynaScenarioEventServer.memory_set_values(memory, "SetVelocity", 0.0, 0.0)
    var action:MaszynaVehicleCommandAction = MaszynaVehicleCommandAction.new()
    action.source = memory
    action.position = Vector3(STOP_AT, 0, 0)
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.system_add_event(_event_system, event)
    ScenarioEventServer.event_attach_action(event, action)
    MaszynaScenarioEventServer.event_set_passive(event, true)
    MaszynaScenarioEventServer.track_add_event(rids[1], MaszynaScenarioEventServer.TRACK_EVENT2, event)
    var car:RID = _car(RoadServer.track_get_road(rids[0]), 50.0, true)
    var controller:RoadVehicleController = VehicleServer.vehicle_get_controller(car)

    assert_true(controller.is_engine_running(), "the driver started the engine")
    assert_eq(int(controller.get_lights()), RoadVehicleController.LIGHTS_HEAD | RoadVehicleController.LIGHTS_TAIL,
            "head lights and red markers on")
    await step(ticks(STOPPING_TIME))
    assert_almost_eq(VehicleServer.vehicle_get_velocity(car), 0.0, 0.01, "it stopped")
    var front:float = _x(car) + controller.dimensions_length / 2.0
    assert_true(front < STOP_AT and front > STOP_AT - 15.0, "short of the memory: front at %.2f m" % front)

    MaszynaScenarioEventServer.memory_set_values(memory, "SetVelocity", -1.0, -1.0)
    var moved:bool = await wait_simulated_until(func() -> bool: return _x(car) > STOP_AT, MOVING_OFF_TIME,
            "the car past the memory")
    assert_true(moved)


func test_the_driver_sounds_the_horn_for_a_warning_signal() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(1000, 0, 0))]
    var car:RID = _car(RoadServer.track_get_road(_network(tracks)[0]), 50.0, true)
    var controller:RoadVehicleController = VehicleServer.vehicle_get_controller(car)

    MaszynaCarDriver.vehicle_handle_command(car, "Warning_signal", 1.0, RoadVehicleController.HORNS_LOW)

    assert_eq(int(controller.get_horns()), RoadVehicleController.HORNS_LOW)
    await step(ticks(2.0))
    assert_eq(int(controller.get_horns()), RoadVehicleController.HORNS_NONE, "silent after its second")


func test_a_car_s_engine_idles_started_and_sounds_higher_as_it_runs() -> void:
    var tracks:Array[TrackData] = [_road_track(Vector3(0, 0, 0), Vector3(1000, 0, 0))]
    var car:RID = _car(RoadServer.track_get_road(_network(tracks)[0]), 50.0, false)
    var engine:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    engine.label = "engine"
    engine.amplitude_factor = 0.3
    engine.amplitude_offset = 0.85
    engine.frequency_factor = 0.07
    engine.frequency_offset = 0.7
    var formulas:MmdSoundFormulas = MmdSoundFormulas.new(car)
    var events:Array[SfxEvent] = []
    formulas.running_sound(engine, &"engine", {}, events)
    var bank:SfxBank = SfxBank.new()
    bank.events = events
    formulas.apply(bank)
    var sound:SfxPlaybackRuntime = SfxPlaybackRuntime.new()
    sound.set_bank(bank)
    var controller:RoadVehicleController = VehicleServer.vehicle_get_controller(car) as RoadVehicleController
    # the car's engine as MaszynaVehicleSoundServer feeds it
    var feed:Callable = func() -> void:
        sound.set_parameter(MaszynaVehicleSoundServer.quantity_get_name(MaszynaVehicleSoundServer.QUANTITY_CAR_ENGINE_RUNNING),
                1.0 if controller.is_engine_running() else 0.0)
        sound.set_parameter(MaszynaVehicleSoundServer.quantity_get_name(MaszynaVehicleSoundServer.QUANTITY_CAR_ENGINE_REVOLUTIONS),
                controller.get_engine_revolutions())
        sound.set_parameter(MaszynaVehicleSoundServer.quantity_get_name(MaszynaVehicleSoundServer.QUANTITY_CAR_THROTTLE),
                controller.get_throttle())
        sound.update(TICK)

    feed.call()
    assert_eq(sound.get_parameter(&"engine_active"), 0.0, "silent with its engine stopped")
    VehicleServer.vehicle_send_command(car, "engine_start")
    for _tick:int in range(10):
        feed.call()
    assert_eq(sound.get_parameter(&"engine_active"), 1.0, "a started engine idles")
    assert_almost_eq(sound.get_parameter(&"engine_pitch"), engine.frequency_offset, 0.001)
    VehicleServer.vehicle_send_command(car, "wanted_velocity_set", CAR_VELOCITY)
    await step(ticks(5.0))
    feed.call()
    assert_gt(sound.get_parameter(&"engine_pitch"), engine.frequency_offset, "higher as it runs")
