extends MaszynaGutTest

## The track under a vehicle reaches its physics, as TDynamicObject::Update() hands it to the Mover
## (DynObj.cpp:3145-3240): a vehicle running onto a track without rails or off an open track end
## derails (dtrack_norail, Mover.cpp:1346-1356; TTrack::NullCreate(), Track.cpp:226-262), is told
## once (RailVehicleServer.vehicle_derailed) and drawn below its track; a grade pulls it
## (FTrain += TotalMassxg * dHtrack, Mover.cpp:4601); a curve's cant leans against its radius;
## the rails' friction is the track's times the scenery's and the weather's (DynObj.cpp:3176).

## Long enough to run on for the test's seconds [m]
const LONG_TRACK:float = 400.0
## Short enough to run off within the test's seconds [m]
const SHORT_TRACK:float = 40.0
## The speed the cars are set rolling at [km/h]
const ROLLING_SPEED:float = 30.0
## Simulated seconds a car has to derail in
const DERAIL_TIMEOUT:float = 20.0
## Simulated seconds the cars roll on the flat and the grade before they are compared
const ROLLING_SECONDS:float = 5.0
## The grade's rise over LONG_TRACK [m] - 2 %
const GRADE_RISE:float = 8.0
## The grade changes the speed after ROLLING_SECONDS by this much at least [km/h] (g * 0.02 * 5 s is
## about 3.5 km/h)
const GRADE_EFFECT:float = 2.0
## A derailed body sits this far below its track (OffsetTrackV, Mover.cpp:1290)
const DERAILED_DROP:float = 0.2
const POSITION_TOLERANCE:float = 0.05
## Simulated seconds the derailed car is given to come to rest
const SETTLE_SECONDS:float = 2.0
## A canted curve of the game's data (baltyk/mod/tory_skm.scm: R -350, roll 2 degrees), moved to the
## origin - the data's radius and roll have opposite signs, as a cant against the curve needs
const CANTED_END:Vector3 = Vector3(-44.72, 0.0, -78.82)
const CANTED_CONTROL1:Vector3 = Vector3(-18.1848, 0.0, -24.1313)
const CANTED_CONTROL2:Vector3 = Vector3(11.4707, 0.0, 28.2051)
const CANTED_ROLL:float = 2.0
## Bogie pivots this far apart [m] - the radius is measured over them (ts.R, DynObj.cpp:3158) -
## on a car this long [m], which leaves room for them (fAxleDist up to L - 0.2, DynObj.cpp:2502)
const BOGIE_SPACING:float = 15.0
const CAR_LENGTH:float = 20.0
const TRACK_FRICTION:float = 0.2
const SCENERY_FRICTION:float = 0.5
## A memory cell's values on a track's friction: friction * 2 + 0.1, then * 0.5 + 0
const MEMORY_MULTIPLIER:float = 2.0
const MEMORY_OFFSET:float = 0.1
const MEMORY_SECOND_MULTIPLIER:float = 0.5
const FRICTION_TOLERANCE:float = 0.0001

var _derailed:Array[Dictionary] = []
var _tracks:Array[RID] = []
var _cars:Array[RailVehicle3D] = []
## The event system the test's memories, launchers and events are of, the original's
var _event_system:RID


func before_each() -> void:
    _derailed.clear()
    _event_system = ScenarioEventServer.system_create()
    ScenarioEventServer.system_attach_implementation(_event_system, MaszynaScenarioEventServer.implementation_get())
    RailVehicleServer.vehicle_derailed.connect(_on_vehicle_derailed)


func after_each() -> void:
    RailVehicleServer.vehicle_derailed.disconnect(_on_vehicle_derailed)
    for car:RailVehicle3D in _cars:
        free_rail_vehicle(car)
    _cars.clear()
    for track:RID in _tracks:
        TrackServer.track_free(track)
    _tracks.clear()
    TrackServer.topology_rebuild()
    ScenarioEventServer.system_free(_event_system)


func test_a_car_running_off_an_open_track_end_derails() -> void:
    _tracks.append(build_track("short", SHORT_TRACK))
    var vehicle:RID = await _rolling_car("short", 0.5 * SHORT_TRACK)
    var standing_height:float = RailVehicleServer.vehicle_get_transform(vehicle).origin.y
    assert_true(await wait_simulated_until(func() -> bool: return _derailed.size() > 0, DERAIL_TIMEOUT,
            "derailed at the track end"), "the car runs off the end of its track and derails")
    if not is_passing():
        return
    assert_eq(_derailed.size(), 1, "the derailment is told once")
    assert_eq(_derailed[0]["vehicle"], vehicle)
    assert_eq(_derailed[0]["cause"], RailVehicleController.DERAIL_CAUSE_NO_RAIL, "no rails past the end")
    var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
    assert_eq(controller.get_derail_cause(), RailVehicleController.DERAIL_CAUSE_NO_RAIL)
    # the Mover moves the body off its track on its next step, and the body follows as it moves
    # (ComputeMovement(), Mover.cpp:1287-1293; TDynamicObject::Move(), DynObj.cpp:2706-2709)
    await step(ticks(SETTLE_SECONDS))
    assert_almost_eq(RailVehicleServer.vehicle_get_transform(vehicle).origin.y, standing_height - DERAILED_DROP,
            POSITION_TOLERANCE, "the derailed body sits below its track")


func test_a_car_running_onto_a_track_without_rails_derails() -> void:
    var track:RID = build_track("broken", LONG_TRACK)
    _tracks.append(track)
    TrackServer.track_update_properties(track, TrackServer.TRACK_DAMAGE_NO_RAIL, TrackServer.track_get_friction(track), -1.0)
    var vehicle:RID = await _rolling_car("broken", 0.5 * LONG_TRACK)
    assert_true(await wait_simulated_until(func() -> bool: return _derailed.size() > 0, DERAIL_TIMEOUT,
            "derailed on a track without rails"), "the car derails where the rails are gone")
    if not is_passing():
        return
    assert_eq(_derailed[0]["vehicle"], vehicle)
    assert_eq(_derailed[0]["cause"], RailVehicleController.DERAIL_CAUSE_NO_RAIL)


func test_a_grade_pulls_a_rolling_car() -> void:
    _tracks.append(build_track("flat", LONG_TRACK))
    var curve:TrackCurve = TrackCurve.new()
    curve.p2 = Vector3(LONG_TRACK, GRADE_RISE, 0.0)
    var grade:RID = TrackServer.track_create()
    _tracks.append(grade)
    TrackServer.track_update_curves(grade, curve, null)
    TrackServer.track_update(grade, TrackServer.TRACK_NORMAL, "grade", BUILT_TRACK_GAUGE)
    TrackServer.topology_rebuild()
    var on_flat:RID = await _rolling_car("flat", 0.5 * LONG_TRACK)
    var on_grade:RID = await _rolling_car("grade", 0.5 * LONG_TRACK)
    await step(ticks(ROLLING_SECONDS))
    var flat_speed:float = VehicleServer.vehicle_get_speed(on_flat)
    var grade_speed:float = VehicleServer.vehicle_get_speed(on_grade)
    assert_gt(absf(grade_speed - flat_speed), GRADE_EFFECT,
            "the grade changes the car's speed (flat %.2f km/h, grade %.2f km/h)" % [flat_speed, grade_speed])
    assert_eq(_derailed.size(), 0, "nothing derails on a sound track")


## A scenario's switch event on a plain rail track works a derailer: the second position takes the
## rails away, the first puts them back (TTrack::Switch(), Track.cpp:1846-1847)
func test_a_switch_event_on_a_plain_track_works_a_derailer() -> void:
    var track:RID = build_track("derailer", LONG_TRACK)
    _tracks.append(track)
    var action:MaszynaSwitchAction = MaszynaSwitchAction.new()
    var tracks:Array[RID] = [track]
    action.tracks = tracks
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.event_attach_action(event, action)
    action.active_track = TrackServer.TRACK_DIVERGING
    ScenarioEventServer.event_queue(event)
    await wait_simulated_until(func() -> bool: return not TrackServer.track_get_damage_flag(track) & TrackServer.TRACK_DAMAGE_NO_RAIL == 0,
            TICK * 2.0, "the derailer thrown")
    assert_ne(TrackServer.track_get_damage_flag(track) & TrackServer.TRACK_DAMAGE_NO_RAIL, 0, "the rails taken away")
    action.active_track = TrackServer.TRACK_COMMON
    ScenarioEventServer.event_queue(event)
    await wait_simulated_until(func() -> bool: return TrackServer.track_get_damage_flag(track) & TrackServer.TRACK_DAMAGE_NO_RAIL == 0,
            TICK * 2.0, "the derailer put back")
    assert_eq(TrackServer.track_get_damage_flag(track) & TrackServer.TRACK_DAMAGE_NO_RAIL, 0, "the rails back")
    ScenarioEventServer.event_free(event)


## The curve's radius and its cant have opposite signs whichever way the vehicle stands on it - the
## cant compensates the centrifugal acceleration (AccN = V^2/R + g * dHrail / TrackW, Mover.cpp:1311);
## a track run backwards has its roll the other way (TrkFoll.cpp:273-278)
func test_a_canted_curve_leans_against_its_radius() -> void:
    var curve:TrackCurve = TrackCurve.new()
    curve.p2 = CANTED_END
    curve.c1 = CANTED_CONTROL1
    curve.c2 = CANTED_CONTROL2
    curve.roll1 = CANTED_ROLL
    curve.roll2 = CANTED_ROLL
    var track:RID = TrackServer.track_create()
    _tracks.append(track)
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, "canted", BUILT_TRACK_GAUGE)
    TrackServer.topology_rebuild()
    for direction:TrackServer.Direction in [TrackServer.DIRECTION_NORMAL, TrackServer.DIRECTION_REVERSED]:
        var car:RailVehicle3D = build_rail_vehicle("canted_%d" % direction, "canted", 0.5 * TrackServer.track_get_length(track, TrackServer.TRACK_COMMON))
        car.start_direction = direction
        _cars.append(car)
        await step(ticks(TICK) + 1)
        var vehicle:RID = (car.get_node(car.controller_path) as VehiclePhysicsNode).get_vehicle_rid()
        # bogies to measure the curve over; the axles stand at them once the car is placed again
        var wheels:RailVehicleWheels = MaszynaMoverRailVehicleWheels.new()
        wheels.bogie_pivot_spacing = BOGIE_SPACING
        var controller:VehicleController = VehicleServer.vehicle_get_controller(vehicle)
        controller.add_component(wheels)
        controller.dimensions_length = CAR_LENGTH
        RailVehicleServer.vehicle_set_track(vehicle, track,
                0.5 * TrackServer.track_get_length(track, TrackServer.TRACK_COMMON), direction)
        var shape:Dictionary = RailVehicleServer.vehicle_get_curve(vehicle)
        assert_false(is_zero_approx(float(shape["radius"])), "the curve is found (direction %d)" % direction)
        assert_lt(float(shape["radius"]) * float(shape["cant"]), 0.0,
                "the cant leans against the radius (direction %d): %s" % [direction, shape])


## The friction the wheels have: the track's, times the scenery's (Global.fFriction) and the
## weather's (Global.FrictionWeatherFactor) - and a memory cell bound to the track modifies the track's
func test_the_rails_friction_is_the_tracks_times_the_scenerys_and_the_weathers() -> void:
    var track:RID = build_track("rails", LONG_TRACK)
    _tracks.append(track)
    TrackServer.track_update_properties(track, TrackServer.TRACK_DAMAGE_NONE, TRACK_FRICTION, -1.0)
    SimulationServer.friction = SCENERY_FRICTION
    SimulationServer.weather = SimulationServer.WEATHER_RAIN
    var vehicle:RID = await _rolling_car("rails", 0.5 * LONG_TRACK)
    await step(2)
    var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
    assert_almost_eq(controller.get_track_friction(),
            TRACK_FRICTION * SCENERY_FRICTION * SimulationServer.simulation_get_weather_friction_factor(), FRICTION_TOLERANCE)
    SimulationServer.friction = 1.0
    SimulationServer.weather = SimulationServer.WEATHER_CLEAR


func test_rain_and_snow_take_the_rails_grip() -> void:
    SimulationServer.weather = SimulationServer.WEATHER_RAIN
    assert_almost_eq(SimulationServer.simulation_get_weather_friction_factor(), 0.85, FRICTION_TOLERANCE, "rain")
    SimulationServer.weather = SimulationServer.WEATHER_SNOW
    assert_almost_eq(SimulationServer.simulation_get_weather_friction_factor(), 0.75, FRICTION_TOLERANCE, "snow")
    SimulationServer.weather = SimulationServer.WEATHER_CLOUDY
    assert_eq(SimulationServer.simulation_get_weather_friction_factor(), 1.0, "clouds")
    SimulationServer.friction = SCENERY_FRICTION
    SimulationServer.environment_reset()
    assert_eq(SimulationServer.weather, SimulationServer.WEATHER_CLEAR, "a reset clears the weather")
    assert_eq(SimulationServer.friction, 1.0, "and the scenery's friction")


## The scenery's `friction` event sets its multiplier (friction_event, Event.cpp:2090-2104)
func test_a_friction_event_sets_the_scenerys_friction() -> void:
    var action:MaszynaFrictionAction = MaszynaFrictionAction.new()
    action.friction = SCENERY_FRICTION
    var event:RID = ScenarioEventServer.event_create()
    ScenarioEventServer.event_attach_action(event, action)
    ScenarioEventServer.event_queue(event)
    await wait_simulated_until(func() -> bool: return SimulationServer.friction == SCENERY_FRICTION, TICK * 2.0, "the event run")
    assert_eq(SimulationServer.friction, SCENERY_FRICTION)
    ScenarioEventServer.event_free(event)
    SimulationServer.friction = 1.0


## A track's `friction <memcell>`: the cell's values modify the track's friction, and again as they
## change (TTrack::Friction(), Track.cpp:2527-2533)
func test_a_memory_cell_modifies_a_tracks_friction() -> void:
    var track:RID = build_track("memory_rails", LONG_TRACK)
    _tracks.append(track)
    TrackServer.track_update_properties(track, TrackServer.TRACK_DAMAGE_NONE, TRACK_FRICTION, -1.0)
    var memory:RID = MaszynaScenarioEventServer.memory_create(_event_system)
    MaszynaScenarioEventServer.memory_set_values(memory, "", MEMORY_MULTIPLIER, MEMORY_OFFSET)
    MaszynaScenarioEventServer.memory_add_friction_track(memory, track)
    assert_almost_eq(TrackServer.track_get_friction(track), TRACK_FRICTION * MEMORY_MULTIPLIER + MEMORY_OFFSET, FRICTION_TOLERANCE)
    MaszynaScenarioEventServer.memory_set_values(memory, "", MEMORY_SECOND_MULTIPLIER, 0.0)
    assert_almost_eq(TrackServer.track_get_friction(track), TRACK_FRICTION * MEMORY_SECOND_MULTIPLIER, FRICTION_TOLERANCE, "followed as it changes")
    MaszynaScenarioEventServer.memory_free(memory)


## A car on the track at the offset, rolling at ROLLING_SPEED; its RID
func _rolling_car(track_name:String, offset:float) -> RID:
    var car:RailVehicle3D = build_passenger_car("car_%d" % _cars.size(), track_name, offset, 0.0, 0.0, ROLLING_SPEED)
    _cars.append(car)
    await step(ticks(TICK))
    return (car.get_node(car.controller_path) as VehiclePhysicsNode).get_vehicle_rid()


func _on_vehicle_derailed(vehicle:RID, cause:RailVehicleController.DerailCause) -> void:
    _derailed.append({"vehicle": vehicle, "cause": cause})
