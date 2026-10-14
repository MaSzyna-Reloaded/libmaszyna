extends MaszynaGutTest

var train: VehicleController
var ai_hints: RailVehicleAIHints

func before_each():
    train = build_vehicle("TestTrain")

    ai_hints = MoverRailVehicleAIHints.new()
    train.add_component(ai_hints)
    await wait_idle_frames(2)

func test_defaults():
    assert_eq(ai_hints.pantograph_state, RailVehicleAIHints.PANTOGRAPH_STATE_AUTOMATIC)
    assert_true(ai_hints.raise_pantographs_when_idle)
    assert_eq(ai_hints.local_brake_acceleration_factor, 1.05)

func test_round_trip_and_update_without_crashing():
    ai_hints.pantograph_state = RailVehicleAIHints.PANTOGRAPH_STATE_BOTH
    ai_hints.raise_pantographs_when_idle = false
    ai_hints.local_brake_acceleration_factor = 1.2
    await wait_idle_frames(2)

    assert_eq(ai_hints.pantograph_state, RailVehicleAIHints.PANTOGRAPH_STATE_BOTH)
    assert_false(ai_hints.raise_pantographs_when_idle)
    assert_eq(ai_hints.local_brake_acceleration_factor, 1.2)
    assert_true(is_instance_valid(train), "VehicleController should keep functioning after configuring RailVehicleAIHints")
