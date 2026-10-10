extends MaszynaGutTest
## Baseline for the #184 architecture rework: what the vehicle state costs today, so that every
## later stage has a number to be judged against instead of a claim.
##
## It asserts correctness only and PRINTS the timings. A time assertion in CI is a flake - the
## numbers are for a human comparing two runs of the same machine.
##
## Measure on `make compile-profiling`. `compile-debug` builds the vendored Mover.cpp at -O0,
## which makes the physics several times slower than a shipped build and sends any comparison
## after the wrong subsystem (TODO.md, "Physics performance").
##
## The vehicle is the fixture FIZ (Brake/BuffCoupl/Cntrl/Dimensions/Doors/Param/Wheels, plus the
## Horns the instancer always attaches) with an electric engine, lighting and a spring brake
## added the way a hand-authored scene adds them. That is eight parts, not the fourteen a real
## locomotive carries,
## so the absolute key count is lower than a scenery's - what matters is that it is the same
## vehicle on both sides of a change.
##
## The vehicles stand on a real fixture track on purpose: one without a track computes a NaN
## transform, and `NaN > culling_distance` is false, so it is never culled and the bench would
## measure a path the game does not take (FINDINGS.md, 2026-09-22).

const FIXTURE_FIZ: String = "res://tests/fixtures/test_vehicle.fiz"

const VEHICLE_COUNT: int = 100
## Distance between two vehicles along the track (m) - far enough apart that the neighbour scan
## finds a neighbour rather than a collision
const VEHICLE_SPACING: float = 20.0
## Length of one fixture track segment (m). Several short ones laid end to end, rather than one
## long one, because the neighbour scan buckets vehicles by track and a scenery spreads them too.
const TRACK_LENGTH: float = 200.0
## Track laid past the last vehicle, so the scan ahead of it stays on a real track
const TRACK_MARGIN: float = 200.0
const TRACK_GAUGE: float = 1.435

const FRAME_DELTA: float = 1.0 / 60.0
## Frames averaged per measurement - enough to swamp a single scheduling hiccup
const SAMPLE_FRAMES: int = 30

const MICROSECONDS_PER_MILLISECOND: float = 1000.0

## What one TrainSoundSystem update pulls out of a vehicle per tick: what BrakeSoundModel reads,
## the MMD trigger properties of
## mmd_sound_catalog.gd, and what RunningSoundModel reads. Kept as the strings the sound system
## itself uses, because the point of the measurement is what a by-name read costs today.
const SOUND_KEYS: PackedStringArray = [
    "speed", "velocity", "mass_total", "direction",
    "brake_force_ratio", "brake_emergency_valve_flow", "brake_air_pressure",
    "brake_releaser_active", "brake_loco_pressure", "brake_main_valve_flow", "pipe_pressure",
    "brake_control_pressure", "brake_unit_force", "brake_local_aeim_position",
    "brake_edb_cylinder_pressure", "brake_handle_braking_flow", "brake_handle_release_flow",
    "brake_handle_emergency_flow", "brake_handle_control_chamber_flow",
    "brake_handle_timing_reservoir_flow",
    "spring_brake/active", "slipping_wheels", "wheel_rotation_speed_rps",
    "battery_enabled", "compressor_enabled", "engine_rpm_ratio", "engine_power",
    "engine_type", "engine_rpm_count", "dynamic_brake_active", "motor_torque",
    "horn_low_active", "horn_high_active", "tachometer_clock_speed",
]

var _tracks: Array[RID] = []
var _vehicles: Array[RID] = []
var _controllers: Array[VehicleController] = []
var _vehicle_nodes: Array[VehiclePhysicsNode] = []


func before_all() -> void:
    var build_started: int = Time.get_ticks_usec()
    var track_count: int = ceili((VEHICLE_COUNT * VEHICLE_SPACING + TRACK_MARGIN) / TRACK_LENGTH)
    for index: int in track_count:
        var curve: TrackCurve = TrackCurve.new()
        curve.p1 = Vector3(index * TRACK_LENGTH, 0.0, 0.0)
        curve.p2 = Vector3((index + 1) * TRACK_LENGTH, 0.0, 0.0)
        var track_rid: RID = TrackServer.track_create()
        TrackServer.track_update_curves(track_rid, curve, null)
        TrackServer.track_update(track_rid, TrackServer.TRACK_NORMAL, "", TRACK_GAUGE)
        _tracks.append(track_rid)
    TrackServer.topology_rebuild()

    # the vehicle is configured by its model, before the Mover is initialised - applying a FIZ
    # afterwards leaves the backend running on the zeros it started with (mass 0 -> NaN velocity)
    var model: VehicleController = FizVehicleBuilder.build_description_at(FIXTURE_FIZ)
    for index: int in VEHICLE_COUNT:
        var physics_node: RailVehiclePhysicsNode = RailVehiclePhysicsNode.new()
        physics_node.vehicle_id = "bench_vehicle_%d" % index
        physics_node.set_controller(model)
        add_child(physics_node)
        _vehicle_nodes.append(physics_node)
        var controller: VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())

        # the two biggest publishers the fixture has no section for, added as a scene would
        var engine: RailVehicleElectricSeriesEngine = MoverRailVehicleElectricSeriesEngine.new()
        controller.add_component(engine)
        # what the engine used to publish of its power source, now a component of its own
        var power_source: RailVehicleEnginePowerSource = MoverRailVehicleEnginePowerSource.new()
        power_source.source_type = RailVehicleController.POWER_SOURCE_ACCUMULATOR
        controller.add_component(power_source)
        var lighting: RailVehicleLighting = MoverRailVehicleLighting.new()
        controller.add_component(lighting)
        var spring_brake: RailVehicleSpringBrake = MoverRailVehicleSpringBrake.new()
        controller.add_component(spring_brake)

        _controllers.append(controller)

        # the vehicle's own handle - creating a second one here would step the same controller
        # twice and free it twice
        var vehicle_rid: RID = physics_node.get_vehicle_rid()
        _vehicles.append(vehicle_rid)
        var offset: float = index * VEHICLE_SPACING
        var track_rid: RID = _tracks[int(offset / TRACK_LENGTH)]
        RailVehicleServer.vehicle_set_track(
            vehicle_rid, track_rid, fmod(offset, TRACK_LENGTH),
            TrackServer.DIRECTION_NORMAL)

    await wait_idle_frames(2)
    print("[bench] built %d vehicles in %.1f ms" % [
        VEHICLE_COUNT, float(Time.get_ticks_usec() - build_started) / MICROSECONDS_PER_MILLISECOND])


func after_all() -> void:
    # the handles belong to the nodes below and go with them
    _vehicles.clear()
    _controllers.clear()
    for physics_node: VehiclePhysicsNode in _vehicle_nodes:
        remove_child(physics_node)
        physics_node.free()
    _vehicle_nodes.clear()
    for track_rid: RID in _tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    _tracks.clear()
    TrackServer.topology_rebuild()
    super()


## Every vehicle has to be on the track and publishing state, or the three measurements below
## are timing a path the game never takes.
func test_bench_fixture_is_sound() -> void:
    assert_eq(_controllers.size(), VEHICLE_COUNT, "every bench vehicle was built")
    var published: Dictionary = _controllers[0].get_state()
    assert_true(published.has("velocity"), "the controller publishes its own state")
    assert_true(published.has("brake_air_pressure"), "the FIZ parts are attached")
    assert_true(published.has("engine_power"), "the engine is attached")
    var missing: PackedStringArray = PackedStringArray()
    for key: String in SOUND_KEYS:
        if not published.has(key):
            missing.append(key)
    assert_eq(missing.size(), 0, "the sound-shaped read hits real keys, missing: %s" % [missing])

    for vehicle_rid: RID in _vehicles:
        var origin: Vector3 = RailVehicleServer.vehicle_get_transform(vehicle_rid).origin
        assert_false(is_nan(origin.x), "a vehicle without a track is never culled - see the header")
    print("[bench] %d vehicles, %d state keys each" % [VEHICLE_COUNT, published.size()])


func test_bench_physics_server_tick() -> void:
    var started: int = Time.get_ticks_usec()
    for frame: int in SAMPLE_FRAMES:
        VehicleServer.stepping_advance(FRAME_DELTA)
    var elapsed: int = Time.get_ticks_usec() - started
    print("[bench] server tick: %.3f ms/frame for %d vehicles" % [
        float(elapsed) / SAMPLE_FRAMES / MICROSECONDS_PER_MILLISECOND, VEHICLE_COUNT])
    assert_true(true, "timing only - a time assertion in CI is a flake")


## What one vehicle costs to publish, which is the number FINDINGS.md (2026-09-22) puts at ~17 us
## per controller per tick and names as the next thing to look at.
func test_bench_state_build() -> void:
    var started: int = Time.get_ticks_usec()
    for frame: int in SAMPLE_FRAMES:
        for controller: VehicleController in _controllers:
            controller.update_state()
            controller.get_state()
    var elapsed: int = Time.get_ticks_usec() - started
    print("[bench] state build: %.1f us/vehicle, %.3f ms/frame for %d vehicles" % [
        float(elapsed) / SAMPLE_FRAMES / VEHICLE_COUNT,
        float(elapsed) / SAMPLE_FRAMES / MICROSECONDS_PER_MILLISECOND, VEHICLE_COUNT])
    assert_true(true, "timing only - a time assertion in CI is a flake")


## The shape of TrainSoundSystem's own read: the state once per vehicle, then every key it wants
## out of it by name (train_sound_system.gd:203, _update_brake_sounds, _update_triggers).
func test_bench_sound_shaped_read() -> void:
    var sink: float = 0.0
    var started: int = Time.get_ticks_usec()
    for frame: int in SAMPLE_FRAMES:
        for controller: VehicleController in _controllers:
            controller.update_state()
            var state: Dictionary = controller.get_state()
            for key: String in SOUND_KEYS:
                sink += float(state.get(key, 0.0))
    var elapsed: int = Time.get_ticks_usec() - started
    print("[bench] sound-shaped read of %d keys: %.1f us/vehicle, %.3f ms/frame" % [
        SOUND_KEYS.size(),
        float(elapsed) / SAMPLE_FRAMES / VEHICLE_COUNT,
        float(elapsed) / SAMPLE_FRAMES / MICROSECONDS_PER_MILLISECOND])
    if is_nan(sink):
        var offenders: PackedStringArray = PackedStringArray()
        var probe: Dictionary = _controllers[0].get_state()
        for key: String in SOUND_KEYS:
            if is_nan(float(probe.get(key, 0.0))):
                offenders.append(key)
        fail_test("these published state values are NaN: %s" % [offenders])
    assert_true(not is_nan(sink), "the reads were not optimised away")
