extends MaszynaGutTest

var train: VehicleController

func before_each():
    train = build_vehicle("TestTrain", load("res://tests/fixtures/sm42_vehicle.tres"))
    await wait_idle_frames(2)

func test_successful_ep_fuse_enabling():
    train.send_command("switch_ep_fuse", true)
    await wait_idle_frames(2)
    print(train.get_state())
    assert_true(train.get_state()["dcemued/ep_fuse"], "EP Fuse should be enabled")

func test_successful_ep_fuse_disabling():
    train.send_command("switch_ep_fuse", false)
    await wait_idle_frames(2)
    assert_false(train.get_state()["dcemued/ep_fuse"], "EP Fuse should be disabled")
