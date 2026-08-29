extends MaszynaGutTest

var train: TrainController
var ai_hints: TrainAIHints

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    ai_hints = TrainAIHints.new()
    train.add_child(ai_hints)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(ai_hints.pantograph_state, TrainAIHints.PANTOGRAPH_STATE_FRONT)
    assert_true(ai_hints.raise_pantographs_when_idle)
    assert_eq(ai_hints.local_brake_acceleration_factor, 1.05)

func test_round_trip_and_update_without_crashing():
    ai_hints.pantograph_state = TrainAIHints.PANTOGRAPH_STATE_BOTH
    ai_hints.raise_pantographs_when_idle = false
    ai_hints.local_brake_acceleration_factor = 1.2
    await wait_idle_frames(2)

    assert_eq(ai_hints.pantograph_state, TrainAIHints.PANTOGRAPH_STATE_BOTH)
    assert_false(ai_hints.raise_pantographs_when_idle)
    assert_eq(ai_hints.local_brake_acceleration_factor, 1.2)
    assert_true(is_instance_valid(train), "TrainController should keep functioning after configuring TrainAIHints")
