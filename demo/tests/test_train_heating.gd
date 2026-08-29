extends MaszynaGutTest

var train: TrainController
var heating: TrainHeating

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    heating = TrainHeating.new()
    train.add_child(heating)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func test_defaults():
    assert_eq(heating.get("heating/source"), TrainController.POWER_SOURCE_GENERATOR)
    assert_eq(heating.get("heating/generator/engine"), TrainEngine.MAIN)
    assert_eq(heating.get("heating/generator/min_rpm"), 0.0)
    assert_eq(heating.get("heating/generator/max_rpm"), 0.0)
    assert_eq(heating.get("heating/power_cable/type"), TrainController.POWER_TYPE_ELECTRIC)
    assert_eq(heating.get("heating/max_voltage"), 0.0)

func test_generator_source_updates_without_crashing():
    heating.set("heating/source", TrainController.POWER_SOURCE_GENERATOR)
    heating.set("heating/generator/min_rpm", 600.0)
    heating.set("heating/generator/max_rpm", 1200.0)
    heating.set("heating/generator/min_voltage", 100.0)
    heating.set("heating/generator/max_voltage", 140.0)
    await wait_idle_frames(2)

    assert_true(train.state.has("heating_enabled"), "TrainController should keep functioning after configuring the generator heating source")

func test_power_cable_source_updates_without_crashing():
    heating.set("heating/source", TrainController.POWER_SOURCE_POWERCABLE)
    heating.set("heating/power_cable/type", TrainController.POWER_TYPE_ELECTRIC)
    heating.set("heating/max_voltage", 3000.0)
    await wait_idle_frames(2)

    assert_true(train.state.has("heating_power"), "TrainController should keep functioning after configuring the power cable heating source")
