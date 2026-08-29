extends MaszynaGutTest

var train: TrainController

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(train.get("cntrl/battery_start_mode"), TrainController.START_MODE_MANUAL)
    assert_true(train.get("cntrl/automatic_cab_activation"))
    assert_eq(train.get("cntrl/inactive_cab_flag"), 0)

func test_round_trip_and_update_without_crashing():
    train.set("cntrl/battery_start_mode", TrainController.START_MODE_AUTOMATIC)
    train.set("cntrl/ground_relay_start_mode", TrainController.START_MODE_AUTOMATIC)
    train.set("cntrl/compartment_lights_start_mode", TrainController.START_MODE_MANUAL)
    train.set("cntrl/automatic_cab_activation", false)
    train.set("cntrl/inactive_cab_flag", 1 | 32) # emergency brake + apply spring brake
    await wait_idle_frames(2)

    assert_eq(train.get("cntrl/battery_start_mode"), TrainController.START_MODE_AUTOMATIC)
    assert_false(train.get("cntrl/automatic_cab_activation"))
    assert_eq(train.get("cntrl/inactive_cab_flag"), 33)
    assert_true(train.state.has("mass_total"), "TrainController should keep functioning after configuring the Cntrl. section")
