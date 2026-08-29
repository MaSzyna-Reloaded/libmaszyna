extends MaszynaGutTest

var train: TrainController
var engine: TrainDieselEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainDieselEngine.new()
    train.add_child(engine)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(engine.get("transmission/gear_teeth_motor"), 0)
    assert_eq(engine.get("transmission/gear_teeth_wheel"), 0)
    assert_eq(engine.get("transmission/efficiency"), 1.0)
    assert_eq(engine.maximum_traction_force, 0.0)
    assert_eq(engine.get("motor_blowers/speed"), 0.0)
    assert_false(engine.pressure_switch_present)
    assert_eq(engine.inverters_count, 0)

func test_round_trip_and_update_without_crashing():
    engine.set("transmission/gear_teeth_motor", 18)
    engine.set("transmission/gear_teeth_wheel", 72)
    engine.set("transmission/efficiency", 0.97)
    engine.maximum_traction_force = 180.0
    engine.set("motor_blowers/speed", 1.0)
    engine.set("motor_blowers/sustain_time", 30.0)
    engine.set("motor_blowers/start_velocity", 10.0)
    engine.pressure_switch_present = true
    engine.inverters_count = 2
    await wait_idle_frames(2)

    assert_eq(engine.get("transmission/gear_teeth_motor"), 18)
    assert_eq(engine.get("transmission/gear_teeth_wheel"), 72)
    assert_eq(engine.maximum_traction_force, 180.0)
    assert_true(engine.pressure_switch_present)
    assert_eq(engine.inverters_count, 2)
    assert_true(train.state.has("main_switch_enabled"), "TrainEngine should keep functioning after configuring the common Engine: fields")
