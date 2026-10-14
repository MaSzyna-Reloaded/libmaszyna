extends MaszynaGutTest

## The original's brake sound logic (Train.cpp:8474-8641, DynObj.cpp:4545-4760), driven through
## BrakeSoundModel's public operations with hand-made vehicle state.

const DELTA:float = 0.1
const MAX_SPEED:float = 100.0
const MAX_CYLINDER_PRESSURE:float = 4.0
const TOLERANCE:float = 0.0001


func _model(sources:Array[MmdSoundSourceDefinition], handle_type:int = RailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE) -> BrakeSoundModel:
    var model := BrakeSoundModel.new()
    var no_chunks:Array[StringName] = []
    for source:MmdSoundSourceDefinition in sources:
        model.add_sound(source, StringName(source.label), no_chunks)
    model.readings.max_speed = MAX_SPEED
    model.readings.max_cylinder_pressure = MAX_CYLINDER_PRESSURE
    model.readings.handle_sounds = handle_type == RailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A
    return model


## The model's update() with the vehicle read as `values` (Readings field -> value); the fields
## not given are idle
func _update(model:BrakeSoundModel, values:Dictionary, accelerator_count:int, delta:float) -> Dictionary:
    var idle := BrakeSoundModel.Readings.new()
    for field:Dictionary in idle.get_property_list():
        if field["usage"] & PROPERTY_USAGE_SCRIPT_VARIABLE and not field["name"].begins_with("max_") \
                and not field["name"] in ["handle_sounds", "electro_pneumatic"]:
            model.readings.set(field["name"], values.get(field["name"], idle.get(field["name"])))
    return model.update(accelerator_count, delta)


func _source(label:String, amplitude_factor:float = 1.0, amplitude_offset:float = 0.0) -> MmdSoundSourceDefinition:
    var source := MmdSoundSourceDefinition.new()
    source.label = label
    source.sound_main = label
    source.amplitude_factor = amplitude_factor
    source.amplitude_offset = amplitude_offset
    return source


func _sources(source:MmdSoundSourceDefinition) -> Array[MmdSoundSourceDefinition]:
    var sources:Array[MmdSoundSourceDefinition] = [source]
    return sources


func test_local_brake_release_hiss_follows_the_rate_of_pressure_drop_and_fades_out() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("localbrakesound", 2.5, 0.2)))
    assert_eq(_update(model, {"local_pressure": 3.0}, 0, DELTA), {})

    # rate 10 * -0.1 / 0.1 = -10, filtered 0.1 -> -1.0; gain 0.2 + 2.5 * 1.0 * 0.05
    var result:Dictionary = _update(model, {"local_pressure": 2.9}, 0, DELTA)[&"localbrakesound"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], 0.325, TOLERANCE)

    # the cylinders are held by the train brake now - no more release hiss, the gain falls 0.1/s
    result = _update(model, {"local_pressure": 2.9, "cylinder_pressure": 5.0}, 0, 1.0)[&"localbrakesound"]
    assert_eq(result["action"], BrakeSoundModel.Action.LOOP)
    assert_almost_eq(result["parameters"][&"gain"], 0.225, TOLERANCE)

    result = _update(model, {"local_pressure": 2.9, "cylinder_pressure": 5.0}, 0, 2.0)[&"localbrakesound"]
    assert_eq(result["action"], BrakeSoundModel.Action.STOP)


func test_local_brake_engage_hiss_follows_the_rate_of_pressure_rise() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("localbrakesound2", 2.0)))
    _update(model, {"local_pressure": 0.0}, 0, DELTA)

    var result:Dictionary = _update(model, {"local_pressure": 0.1}, 0, DELTA)[&"localbrakesound2"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], 0.1, TOLERANCE)
    assert_almost_eq(result["parameters"][&"pitch"], 1.0, TOLERANCE)


func test_fv4a_handle_hisses_from_its_own_flows_at_half_volume() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("airsound")), RailVehicleBrake.BRAKE_HANDLE_TYPE_FV4A)

    # flow filtered 0.05 -> 0.5; volume 0.5 * 0.25 = 0.125, half of it heard
    var result:Dictionary = _update(model, {"handle_braking_flow": 10.0, "main_valve_flow": 1.0}, 0, DELTA)[&"airsound"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], 0.0625, TOLERANCE)


func test_other_handles_hiss_from_the_averaged_main_valve_flow() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("airsound")))

    # (4 * 0 + 1) / 5 = 0.2; volume 2 * 0.2
    var result:Dictionary = _update(model, {"main_valve_flow": 1.0}, 0, DELTA)[&"airsound"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], 0.4, TOLERANCE)

    # 0.2 * 0.8^n falls under the 0.05 threshold - a hard stop
    var action:int = BrakeSoundModel.Action.LOOP
    for step:int in range(10):
        var results:Dictionary = _update(model, {"main_valve_flow": 0.0}, 0, DELTA)
        action = results[&"airsound"]["action"]
        if action == BrakeSoundModel.Action.STOP:
            break
    assert_eq(action, BrakeSoundModel.Action.STOP)


func test_brake_shoes_play_with_force_and_speed_and_stop_without() -> void:
    var source:MmdSoundSourceDefinition = _source("brakesound")
    source.frequency_offset = 1.0
    var model:BrakeSoundModel = _model(_sources(source))
    var state:Dictionary = {"unit_force": 20.0, "speed": 50.0, "force_ratio": 1.0}

    var result:Dictionary = _update(model, state, 0, DELTA)[&"brakesound"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], sqrt(lerpf(0.4, 1.0, 50.0 / 101.0)), TOLERANCE)
    assert_almost_eq(result["parameters"][&"pitch"], 1.0 + 50.0 / 101.0, TOLERANCE)

    state["speed"] = 0.0
    assert_eq(_update(model, state, 0, DELTA)[&"brakesound"]["action"], BrakeSoundModel.Action.STOP)


func test_squeal_fades_below_its_speed_and_stops_when_quiet() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("brake")))
    var state:Dictionary = {"unit_force": 20.0, "speed": 50.0, "force_ratio": 1.0}
    assert_eq(_update(model, state, 0, DELTA)[&"brake"]["action"], BrakeSoundModel.Action.START)

    state["speed"] = 1.0
    var result:Dictionary = _update(model, state, 0, DELTA)[&"brake"]
    assert_eq(result["action"], BrakeSoundModel.Action.LOOP)
    assert_almost_eq(result["parameters"][&"gain"], 0.75, TOLERANCE)

    assert_eq(_update(model, state, 0, 1.0)[&"brake"]["action"], BrakeSoundModel.Action.STOP)


func test_cylinder_release_hiss_needs_a_falling_pressure_in_a_filled_cylinder() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("unbrake", 20.0)))
    _update(model, {"cylinder_pressure": 3.8}, 0, DELTA)

    # rate 8 bar/s, filtered 0.05 -> 0.4; 20 * 0.4 * (0.25 + 0.75 * 0.75) = 6.5, heard at most 1
    var result:Dictionary = _update(model, {"cylinder_pressure": 3.0}, 0, DELTA)[&"unbrake"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], BrakeSoundModel.MAX_GAIN, TOLERANCE)


func test_emergency_hiss_starts_and_stops_with_a_hysteresis() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("emergencybrake")))

    assert_eq(_update(model, {"emergency_valve_flow": 0.03}, 0, DELTA)[&"emergencybrake"]["action"],
            BrakeSoundModel.Action.START)
    assert_false(_update(model, {"emergency_valve_flow": 0.02}, 0, DELTA).has(&"emergencybrake"))
    assert_eq(_update(model, {"emergency_valve_flow": 0.01}, 0, DELTA)[&"emergencybrake"]["action"],
            BrakeSoundModel.Action.STOP)


func test_cylinder_clicks_once_per_fifteenth_of_full_pressure() -> void:
    var sources:Array[MmdSoundSourceDefinition] = [_source("brakecylinderinc"), _source("brakecylinderdec")]
    var model:BrakeSoundModel = _model(sources)
    _update(model, {"cylinder_pressure": 0.0}, 0, DELTA)

    var results:Dictionary = _update(model, {"cylinder_pressure": MAX_CYLINDER_PRESSURE * 0.2}, 0, DELTA)
    assert_eq(results[&"brakecylinderinc"]["action"], BrakeSoundModel.Action.ONE_SHOT)
    assert_false(_update(model, {"cylinder_pressure": MAX_CYLINDER_PRESSURE * 0.2}, 0, DELTA).has(&"brakecylinderinc"))

    results = _update(model, {"cylinder_pressure": 0.0}, 0, DELTA)
    assert_eq(results[&"brakecylinderdec"]["action"], BrakeSoundModel.Action.ONE_SHOT)


func test_spring_brake_plays_one_sound_and_stops_the_other() -> void:
    var sources:Array[MmdSoundSourceDefinition] = [_source("springbrake"), _source("springbrakeoff")]
    var model:BrakeSoundModel = _model(sources)

    var results:Dictionary = _update(model, {"spring_brake_active": true}, 0, DELTA)
    assert_eq(results[&"springbrake"]["action"], BrakeSoundModel.Action.EXCLUSIVE_ONE_SHOT)
    assert_eq(results[&"springbrakeoff"]["action"], BrakeSoundModel.Action.STOP)
    assert_false(_update(model, {"spring_brake_active": true}, 0, DELTA).has(&"springbrake"))


func test_accelerator_plays_once_per_reported_event() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("brakeacc")))

    assert_false(_update(model, {}, 0, DELTA).has(&"brakeacc"))
    assert_eq(_update(model, {}, 1, DELTA)[&"brakeacc"]["action"], BrakeSoundModel.Action.EXCLUSIVE_ONE_SHOT)
    assert_false(_update(model, {}, 1, DELTA).has(&"brakeacc"))


func test_silence_restarts_the_sounds_and_the_pressure_rates() -> void:
    var model:BrakeSoundModel = _model(_sources(_source("releaser")))
    var state:Dictionary = {"releaser_active": true, "cylinder_pressure": 0.4}
    assert_eq(_update(model, state, 0, DELTA)[&"releaser"]["action"], BrakeSoundModel.Action.START)
    assert_eq(_update(model, state, 0, DELTA)[&"releaser"]["action"], BrakeSoundModel.Action.LOOP)

    model.silence()
    var result:Dictionary = _update(model, state, 0, DELTA)[&"releaser"]
    assert_eq(result["action"], BrakeSoundModel.Action.START)
    assert_almost_eq(result["parameters"][&"gain"], 0.5, TOLERANCE)
