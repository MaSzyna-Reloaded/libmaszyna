extends MaszynaGutTest

var train: TrainController
var brake: TrainBrake

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    brake = TrainBrake.new()
    train.add_child(brake)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(brake.get("compressor/emergency_valve_area"), 0.0)
    assert_eq(brake.get("universal_brake_button/1"), 0)
    assert_eq(brake.get("universal_brake_button/2"), 0)
    assert_eq(brake.get("universal_brake_button/3"), 0)

func test_round_trip_and_update_without_crashing():
    brake.set("compressor/emergency_valve_area", 1.5)
    brake.set("universal_brake_button/1", 1)  # releaser
    brake.set("universal_brake_button/2", 16) # anti-skid brake
    brake.set("universal_brake_button/3", 8)  # assimilation
    await wait_idle_frames(2)

    assert_eq(brake.get("compressor/emergency_valve_area"), 1.5)
    assert_eq(brake.get("universal_brake_button/1"), 1)
    assert_eq(brake.get("universal_brake_button/2"), 16)
    assert_eq(brake.get("universal_brake_button/3"), 8)
    assert_true(train.state.has("brake_air_pressure"), "TrainBrake should keep functioning after configuring universal brake buttons")
