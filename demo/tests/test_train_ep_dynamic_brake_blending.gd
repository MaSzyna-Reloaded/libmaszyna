extends MaszynaGutTest

var train: TrainController
var ep_brake: TrainElectroPneumaticDynamicBrake

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    ep_brake = TrainElectroPneumaticDynamicBrake.new()
    train.add_child(ep_brake)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_blending_defaults():
    assert_eq(ep_brake.get("blending/max_velocity"), 0.0)
    assert_eq(ep_brake.get("blending/min_velocity"), 0.0)
    assert_eq(ep_brake.get("blending/reference_velocity"), 0.0)
    assert_eq(ep_brake.get("blending/max_deceleration"), 9.81)
    assert_false(ep_brake.get("blending/velocity_correction"))
    assert_false(ep_brake.get("blending/load_correction"))
    assert_eq(ep_brake.get("blending/min_ed_brake_request"), 0.0)

func test_blending_round_trip_and_update():
    ep_brake.set("blending/max_velocity", 120.0)
    ep_brake.set("blending/min_velocity", 5.0)
    ep_brake.set("blending/reference_velocity", 80.0)
    ep_brake.set("blending/max_deceleration", 1.2)
    ep_brake.set("blending/velocity_correction", true)
    ep_brake.set("blending/load_correction", true)
    ep_brake.set("blending/min_ed_brake_request", 0.1)
    await wait_idle_frames(2)

    assert_eq(ep_brake.get("blending/max_velocity"), 120.0)
    assert_eq(ep_brake.get("blending/min_velocity"), 5.0)
    assert_eq(ep_brake.get("blending/reference_velocity"), 80.0)
    assert_eq(ep_brake.get("blending/max_deceleration"), 1.2)
    assert_true(ep_brake.get("blending/velocity_correction"))
    assert_true(ep_brake.get("blending/load_correction"))
    assert_eq(ep_brake.get("blending/min_ed_brake_request"), 0.1)
    assert_true(train.state.has("dcemued/ep_fuse"), "TrainElectroPneumaticDynamicBrake should keep functioning after configuring blending")
