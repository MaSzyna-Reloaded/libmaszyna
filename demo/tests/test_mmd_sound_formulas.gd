extends MaszynaGutTest

## The original's sound formulas - the brakes' (Train.cpp:8474-8641, DynObj.cpp:4545-4760) and the
## engine's gain - as MmdSoundFormulas writes them into a bank of an SM42, run by gnd-sfx's playback
## runtime with hand-made raw quantities, by the names MaszynaVehicleSoundServer feeds them by. The
## original eases by a share per frame; the bank by the time constant of that share at 60 frames a
## second (MmdSoundFormulas.time_constant()).

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const DELTA:float = 0.1
const TOLERANCE:float = 0.0001
const SLOTS:int = 16
## The fixture's Vmax and its brake cylinders' full pressure
const MAX_SPEED:float = 90.0
const MAX_CYLINDER_PRESSURE:float = 4.0

var _vehicle_rid:RID


func before_each() -> void:
    _vehicle_rid = build_vehicle_node("SoundFormulasTest", SM42).get_vehicle_rid()


## A bank of `sources`' brake sounds, each keyed by its label, in a runtime of its own
func _runtime(sources:Array[MmdSoundSourceDefinition]) -> SfxPlaybackRuntime:
    var formulas:MmdSoundFormulas = MmdSoundFormulas.new(_vehicle_rid)
    var events:Array[SfxEvent] = []
    for source:MmdSoundSourceDefinition in sources:
        formulas.brake_sound(source, StringName(source.label), events)
    return _run(formulas, events)


func _run(formulas:MmdSoundFormulas, events:Array[SfxEvent]) -> SfxPlaybackRuntime:
    var bank:SfxBank = SfxBank.new()
    bank.events = events
    formulas.apply(bank)
    var runtime:SfxPlaybackRuntime = SfxPlaybackRuntime.new()
    runtime.set_slot_capacity(SLOTS)
    runtime.set_bank(bank)
    return runtime


## The quantities fed as MaszynaVehicleSoundServer feeds them, then one update of `delta`
func _update(runtime:SfxPlaybackRuntime, values:Dictionary, delta:float = DELTA) -> void:
    for quantity:MaszynaVehicleSoundServer.Quantity in values:
        runtime.set_parameter(MaszynaVehicleSoundServer.quantity_get_name(quantity), values[quantity])
    runtime.update(delta)


func _source(label:String, amplitude_factor:float = 1.0, amplitude_offset:float = 0.0) -> MmdSoundSourceDefinition:
    var source:MmdSoundSourceDefinition = MmdSoundSourceDefinition.new()
    source.label = label
    source.sound_main = label
    source.amplitude_factor = amplitude_factor
    source.amplitude_offset = amplitude_offset
    return source


func _sources(source:MmdSoundSourceDefinition) -> Array[MmdSoundSourceDefinition]:
    var sources:Array[MmdSoundSourceDefinition] = [source]
    return sources


## The share of the way eased in `delta` for the original's share per frame
func _eased(share:float, delta:float = DELTA) -> float:
    return 1.0 - exp(-delta / MmdSoundFormulas.time_constant(share))


func _fired(runtime:SfxPlaybackRuntime, event_name:StringName) -> int:
    return runtime.get_instance_ids(event_name).size()


func test_local_brake_release_hiss_follows_the_rate_of_pressure_drop_and_fades_out() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("localbrakesound", 2.5, 0.2)))
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_LOCAL_PRESSURE: 3.0})
    assert_eq(runtime.get_parameter(&"localbrakesound_active"), 0.0)

    # rate -1 bar/s, ten times it eased by 0.1 a frame; gain 0.2 + 2.5 * |change| * 0.05
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_LOCAL_PRESSURE: 2.9})
    var change:float = 10.0 * _eased(0.1)
    var gain:float = 0.2 + 2.5 * change * 0.05
    assert_eq(runtime.get_parameter(&"localbrakesound_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"localbrakesound_gain"), gain, TOLERANCE)

    # the cylinders are held by the train brake now - no more release hiss, the gain falls 0.1/s
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 5.0}, 1.0)
    assert_eq(runtime.get_parameter(&"localbrakesound_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"localbrakesound_gain"), gain - 0.1, TOLERANCE)

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 5.0}, 10.0)
    assert_eq(runtime.get_parameter(&"localbrakesound_active"), 0.0)


func test_local_brake_engage_hiss_follows_the_rate_of_pressure_rise() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("localbrakesound2", 2.0)))
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_LOCAL_PRESSURE: 0.0})

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_LOCAL_PRESSURE: 0.1})
    assert_eq(runtime.get_parameter(&"localbrakesound2_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"localbrakesound2_gain"), 2.0 * 10.0 * _eased(0.1) * 0.05, TOLERANCE)
    assert_almost_eq(runtime.get_parameter(&"localbrakesound2_pitch"), 1.0, TOLERANCE)


func test_fv4a_handle_hisses_from_its_own_flows_at_half_volume() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("airsound")))

    # the flow eased by 0.05 a frame; volume a quarter of it, half of that heard
    _update(runtime, {
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_HANDLE_BRAKING_FLOW: 10.0,
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_MAIN_VALVE_FLOW: 1.0,
    })
    assert_eq(runtime.get_parameter(&"airsound_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"airsound_gain"), 10.0 * _eased(0.05) * 0.25 * 0.5, TOLERANCE)


func test_other_handles_hiss_from_the_averaged_main_valve_flow() -> void:
    var brake:MaszynaMoverRailVehicleBrake = RailVehicleServer.vehicle_component_get(
            _vehicle_rid, RailVehicleComponentType.COMPONENT_BRAKES) as MaszynaMoverRailVehicleBrake
    brake.cntrl_brake_handle_type = MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("airsound")))

    # (4 * 0 + 1) / 5 a frame; volume twice the flow, heard at most at 1
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_MAIN_VALVE_FLOW: 1.0})
    assert_eq(runtime.get_parameter(&"airsound_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"airsound_gain"), minf(2.0 * _eased(0.2), 1.0), TOLERANCE)

    # the averaged flow falls under the 0.05 threshold - a hard stop
    for step:int in range(10):
        _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_MAIN_VALVE_FLOW: 0.0})
    assert_eq(runtime.get_parameter(&"airsound_active"), 0.0)


func test_brake_shoes_play_with_force_and_speed_and_stop_without() -> void:
    var source:MmdSoundSourceDefinition = _source("brakesound")
    source.frequency_offset = 1.0
    var runtime:SfxPlaybackRuntime = _runtime(_sources(source))
    var state:Dictionary = {
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE: 20.0,
        MaszynaVehicleSoundServer.QUANTITY_SPEED: 50.0,
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO: 1.0,
    }

    _update(runtime, state)
    assert_eq(runtime.get_parameter(&"brakesound_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"brakesound_gain"),
            sqrt(lerpf(0.4, 1.0, 50.0 / (1.0 + MAX_SPEED))), TOLERANCE)
    assert_almost_eq(runtime.get_parameter(&"brakesound_pitch"), 1.0 + 50.0 / (1.0 + MAX_SPEED), TOLERANCE)

    state[MaszynaVehicleSoundServer.QUANTITY_SPEED] = 0.0
    _update(runtime, state)
    assert_eq(runtime.get_parameter(&"brakesound_active"), 0.0)


func test_squeal_fades_below_its_speed_and_stops_when_quiet() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("brake")))
    var state:Dictionary = {
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_UNIT_FORCE: 20.0,
        MaszynaVehicleSoundServer.QUANTITY_SPEED: 50.0,
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_FORCE_RATIO: 1.0,
    }
    _update(runtime, state)
    assert_eq(runtime.get_parameter(&"brake_active"), 1.0)

    # 2.5 of its gain a second
    state[MaszynaVehicleSoundServer.QUANTITY_SPEED] = 1.0
    _update(runtime, state)
    assert_eq(runtime.get_parameter(&"brake_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"brake_gain"), 0.75, TOLERANCE)

    _update(runtime, state, 1.0)
    assert_eq(runtime.get_parameter(&"brake_active"), 0.0)


func test_cylinder_release_hiss_needs_a_falling_pressure_in_a_filled_cylinder() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("unbrake", 20.0)))
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 3.8})

    # rate 8 bar/s eased by 0.05 a frame; 20 * it * (0.25 + 0.75 * 0.75), heard at most at 1
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 3.0})
    assert_eq(runtime.get_parameter(&"unbrake_active"), 1.0)
    assert_almost_eq(runtime.get_parameter(&"unbrake_gain"),
            minf(20.0 * 8.0 * _eased(0.05) * (0.25 + 0.75 * 3.0 / MAX_CYLINDER_PRESSURE), 1.0), TOLERANCE)


func test_emergency_hiss_starts_and_stops_with_a_hysteresis() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("emergencybrake")))

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW: 0.03})
    assert_eq(runtime.get_parameter(&"emergencybrake_active"), 1.0)
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW: 0.02})
    assert_eq(runtime.get_parameter(&"emergencybrake_active"), 1.0, "between the thresholds it plays on")
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_EMERGENCY_VALVE_FLOW: 0.01})
    assert_eq(runtime.get_parameter(&"emergencybrake_active"), 0.0)


func test_cylinder_clicks_once_per_fifteenth_of_full_pressure() -> void:
    var sources:Array[MmdSoundSourceDefinition] = [_source("brakecylinderinc"), _source("brakecylinderdec")]
    var runtime:SfxPlaybackRuntime = _runtime(sources)
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 0.0})

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: MAX_CYLINDER_PRESSURE * 0.2})
    assert_eq(_fired(runtime, &"brakecylinderinc"), 1)
    assert_eq(_fired(runtime, &"brakecylinderdec"), 0)
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: MAX_CYLINDER_PRESSURE * 0.21})
    assert_eq(_fired(runtime, &"brakecylinderinc"), 1, "the same fifteenth clicks once")

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 0.0})
    assert_eq(_fired(runtime, &"brakecylinderdec"), 1)


func test_spring_brake_plays_once_as_it_holds() -> void:
    var sources:Array[MmdSoundSourceDefinition] = [_source("springbrake"), _source("springbrakeoff")]
    var runtime:SfxPlaybackRuntime = _runtime(sources)
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_SPRING_BRAKE_ACTIVE: 0.0})

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_SPRING_BRAKE_ACTIVE: 1.0})
    assert_eq(_fired(runtime, &"springbrake"), 1)
    assert_eq(_fired(runtime, &"springbrakeoff"), 0)


func test_accelerator_plays_once_per_reported_event() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("brakeacc")))

    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_ACCELERATOR_COUNT: 0.0})
    assert_eq(_fired(runtime, &"brakeacc"), 0)
    _update(runtime, {MaszynaVehicleSoundServer.QUANTITY_BRAKE_ACCELERATOR_COUNT: 1.0})
    assert_eq(_fired(runtime, &"brakeacc"), 1)


func test_releaser_is_louder_with_the_cylinder_pressure() -> void:
    var runtime:SfxPlaybackRuntime = _runtime(_sources(_source("releaser")))

    _update(runtime, {
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_RELEASER_ACTIVE: 1.0,
        MaszynaVehicleSoundServer.QUANTITY_BRAKE_CYLINDER_PRESSURE: 0.4,
    })
    assert_almost_eq(runtime.get_parameter(&"releaser_gain"), 0.5, TOLERANCE)


## The engine's gain from its speed and load: 0.25 of the load and 0.75 of the speed's ratio
func test_engine_gain_uses_rpm_and_load() -> void:
    var source:MmdSoundSourceDefinition = _source("engine", 1.5, 0.5)
    var formulas:MmdSoundFormulas = MmdSoundFormulas.new(_vehicle_rid)
    var events:Array[SfxEvent] = [formulas.regular_sound(source, MmdSoundCatalog.get_entry("engine"), &"engine")]
    var runtime:SfxPlaybackRuntime = _run(formulas, events)
    var power:float = VehicleServer.vehicle_get_controller(_vehicle_rid).power

    _update(runtime, {
        MaszynaVehicleSoundServer.QUANTITY_ENGINE_RPM_RATIO: 0.8,
        MaszynaVehicleSoundServer.QUANTITY_ENGINE_POWER: 0.5 * power,
    })
    assert_almost_eq(runtime.get_parameter(&"engine_gain"), 0.5 + 1.5 * (0.25 * 0.5 + 0.75 * 0.8), TOLERANCE)

    # within the gain modulation's domain
    _update(runtime, {
        MaszynaVehicleSoundServer.QUANTITY_ENGINE_RPM_RATIO: 2.0,
        MaszynaVehicleSoundServer.QUANTITY_ENGINE_POWER: 4.0 * power,
    })
    assert_almost_eq(runtime.get_parameter(&"engine_gain"), 2.0, TOLERANCE)
    _update(runtime, {
        MaszynaVehicleSoundServer.QUANTITY_ENGINE_RPM_RATIO: -1.0,
        MaszynaVehicleSoundServer.QUANTITY_ENGINE_POWER: -100.0,
    })
    assert_almost_eq(runtime.get_parameter(&"engine_gain"), 0.5, TOLERANCE)
