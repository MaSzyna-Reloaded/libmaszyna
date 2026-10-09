extends MaszynaGutTest

## The simulation has one clock, SimulationServer's (Timer::UpdateTimers(), Timer.cpp:79-87): a frame
## advances it by its delta, at most MAX_FRAME_DELTA, times the simulation speed; the physics
## integrates exactly that, whole, in steps no longer than PHYSICS_STEP (drivermode.cpp:193-206),
## and the drivers and events get it in slices of at most MAX_SLICE_TIME. The simulation speed is
## honoured whatever the frames cost; a machine under 1 / MAX_FRAME_DELTA fps runs it slower,
## nothing is owed and nothing jumps.

## SimulationServer::MAX_FRAME_DELTA
const MAX_FRAME_DELTA:float = 0.25
## SimulationServer::MAX_SLICE_TIME
const MAX_SLICE_TIME:float = 0.1
## x4: one capped frame is a second of simulation
const SECOND_A_FRAME_SPEED:float = 4.0
## x100: the speed is honoured, not capped at the original's second a frame
const FAST_SPEED:float = 100.0
const VELOCITY_MS:float = 10.0
const TRACK_LENGTH_M:float = 2000.0
const DOUBLE_SPEED:float = 2.0
const SHORT_FRAME:float = 0.1
## What the clock adds between the calls of a test - a frame or two at most [s]
const FRAME_TOLERANCE:float = 0.05
## SimulationServer::SPEED_CHANGE_TIME_SETTING - how long the running speed takes to reach one set
const SPEED_CHANGE_TIME_SETTING:String = "maszyna/simulation/speed_change_time"
const SPEED_CHANGE_TIME:float = 0.4
## Enough frames for the running speed to have reached the one set, many times over
const SETTLE_FRAMES:int = 100

var _speed_change_time:Variant

var _track:RID = RID()
var _controller:VehicleController = null
var _vehicle:RID = RID()


func before_each() -> void:
    # the clock is measured at the speed set, not on its way to it
    _speed_change_time = ProjectSettings.get_setting(SPEED_CHANGE_TIME_SETTING)
    ProjectSettings.set_setting(SPEED_CHANGE_TIME_SETTING, 0.0)
    await ProjectSettings.settings_changed
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = Vector3.ZERO
    curve.p2 = Vector3(TRACK_LENGTH_M, 0.0, 0.0)
    _track = TrackServer.track_create()
    TrackServer.track_update_curves(_track, curve, null)
    TrackServer.track_update(_track, TrackServer.TRACK_NORMAL, "", 1.435)
    TrackServer.topology_rebuild()
    # a real vehicle, because the dynamics need a mass - an empty Mover integrates to NaN
    var model:VehicleController = load("res://tests/fixtures/sm42_vehicle.tres") as VehicleController
    # an unmanned vehicle is not simulated at all (Mover.cpp:4485) - see FINDINGS.md, 2026-09-23
    _controller = build_vehicle("clock_test", model, VELOCITY_MS * 3.6, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    _vehicle = _controller.get_rid()
    RailVehicleServer.vehicle_set_track(_vehicle, _track, 100.0, TrackServer.DIRECTION_NORMAL)
    await wait_idle_frames(1)


func after_each() -> void:
    SimulationServer.simulation_reset_speed()
    ProjectSettings.set_setting(SPEED_CHANGE_TIME_SETTING, _speed_change_time)
    if TrackServer.track_exists(_track):
        TrackServer.track_free(_track)
    TrackServer.topology_rebuild()
    _controller = null


func _travelled() -> float:
    return RailVehicleServer.vehicle_get_transform(_vehicle).origin.x


## A stalled frame counts MAX_FRAME_DELTA of real time and no more - the rest is not owed - times
## the speed, and the physics integrates all of it, not a budget of it.
func test_a_long_frame_is_capped_and_integrated_whole() -> void:
    SimulationServer.simulation_speed = SECOND_A_FRAME_SPEED
    var time_before:float = SimulationServer.simulation_get_time()
    var before:float = _travelled()
    SimulationServer.simulation_advance(5.0)
    var seconds:float = MAX_FRAME_DELTA * SECOND_A_FRAME_SPEED
    assert_almost_eq(SimulationServer.simulation_get_time() - time_before, seconds, 0.0001,
            "the clock takes a quarter of the five real seconds, sped up")
    assert_almost_eq(_travelled() - before, seconds * VELOCITY_MS, VELOCITY_MS * FRAME_TOLERANCE,
            "and the vehicle drove that whole time")


## The simulation speed scales the frame, for the clock and the vehicles alike.
func test_the_speed_scales_the_clock_and_the_physics() -> void:
    SimulationServer.simulation_speed = DOUBLE_SPEED
    var time_before:float = SimulationServer.simulation_get_time()
    var before:float = _travelled()
    SimulationServer.simulation_advance(SHORT_FRAME)
    var seconds:float = SHORT_FRAME * DOUBLE_SPEED
    assert_almost_eq(SimulationServer.simulation_get_time() - time_before, seconds, 0.0001)
    assert_almost_eq(_travelled() - before, seconds * VELOCITY_MS, VELOCITY_MS * FRAME_TOLERANCE,
            "the vehicle drove the simulated seconds, not the real ones")


## A long frame is simulated in slices of at most MAX_SLICE_TIME, each announced: a driver or an
## event reacts in it as it would in a frame that short.
func test_a_long_frame_is_announced_in_short_slices() -> void:
    SimulationServer.simulation_speed = SECOND_A_FRAME_SPEED
    var slices:Array[float] = []
    var record:Callable = func(seconds:float) -> void: slices.append(seconds)
    SimulationServer.simulation_advanced.connect(record)
    SimulationServer.simulation_advance(MAX_FRAME_DELTA)
    SimulationServer.simulation_advanced.disconnect(record)

    assert_eq(slices.size(), roundi(MAX_FRAME_DELTA * SECOND_A_FRAME_SPEED / MAX_SLICE_TIME), "ten slices of a second")
    for seconds:float in slices:
        assert_almost_eq(seconds, MAX_SLICE_TIME, 0.0001)


## The time of day runs with the same seconds.
func test_the_time_of_day_runs_by_the_clock() -> void:
    var hours_before:float = SimulationServer.time_of_day
    SimulationServer.simulation_advance(MAX_FRAME_DELTA)
    assert_almost_eq(fposmod(SimulationServer.time_of_day - hours_before, 24.0), MAX_FRAME_DELTA / 3600.0,
            FRAME_TOLERANCE / 3600.0)


## x100 is x100: a 60 fps frame is well past the original's second a frame.
func test_the_speed_is_honoured_past_a_second_a_frame() -> void:
    SimulationServer.simulation_speed = FAST_SPEED
    var time_before:float = SimulationServer.simulation_get_time()
    var frame:float = 1.0 / 60.0
    SimulationServer.simulation_advance(frame)
    assert_almost_eq(SimulationServer.simulation_get_time() - time_before, frame * FAST_SPEED, 0.0001)


## Paused, the clock stands, and so does everything that reads it.
func test_the_clock_stands_while_paused() -> void:
    SimulationServer.simulation_pause()
    var time_before:float = SimulationServer.simulation_get_time()
    var before:float = _travelled()
    await wait_idle_frames(2)
    assert_eq(SimulationServer.simulation_get_time(), time_before)
    assert_eq(_travelled(), before)
    SimulationServer.simulation_unpause()


## Like a tape's motor, the running speed gets to the one set over a while, not at once
func test_a_speed_set_is_reached_over_the_speed_change_time() -> void:
    ProjectSettings.set_setting(SPEED_CHANGE_TIME_SETTING, SPEED_CHANGE_TIME)
    # SimulationServer takes the setting on settings_changed, which Godot emits deferred
    await ProjectSettings.settings_changed
    SimulationServer.simulation_speed = DOUBLE_SPEED

    SimulationServer.simulation_advance(SHORT_FRAME)
    var running:float = SimulationServer.simulation_get_current_speed()
    assert_true(running > 1.0 and running < DOUBLE_SPEED, "on its way: %s" % running)
    for frame:int in SETTLE_FRAMES:
        SimulationServer.simulation_advance(SHORT_FRAME)

    assert_eq(SimulationServer.simulation_get_current_speed(), DOUBLE_SPEED)
    assert_eq(SimulationServer.simulation_speed, DOUBLE_SPEED, "the speed set is the one set")


## Without a SimulationRuntime in the tree - the editor, a scenery loaded without a game, the tests
## (simulation_runtime_hook.gd) - the frames do not move the clock, whoever holds it
func test_the_clock_stands_without_a_runtime() -> void:
    var time_before:float = SimulationServer.simulation_get_time()
    var before:float = _travelled()
    await wait_idle_frames(2)
    assert_eq(SimulationServer.simulation_get_time(), time_before)
    assert_eq(_travelled(), before)
