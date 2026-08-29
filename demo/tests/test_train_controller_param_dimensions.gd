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

func test_param_and_dimensions_defaults():
    assert_eq(train.category, TrainController.CATEGORY_TRAIN)
    assert_eq(train.train_type, TrainController.TRAIN_TYPE_DEFAULT)
    assert_eq(train.reduced_mass, 0.0)
    assert_eq(train.sand_capacity, 0.0)
    assert_eq(train.heating_power, 0.0)
    assert_eq(train.light_power, 0.0)
    assert_eq(train.get("dimensions/length"), 0.0)
    assert_eq(train.get("dimensions/height"), 0.0)
    assert_eq(train.get("dimensions/width"), 0.0)
    assert_eq(train.get("dimensions/drag_coefficient"), 0.0)
    assert_eq(train.get("dimensions/floor_height"), 0.96)

func test_param_and_dimensions_round_trip_and_update():
    train.category = TrainController.CATEGORY_ROAD
    train.train_type = TrainController.TRAIN_TYPE_ET22
    train.reduced_mass = 500.0
    train.sand_capacity = 300.0
    train.heating_power = 20.0
    train.light_power = 0.56
    train.set("dimensions/length", 15.5)
    train.set("dimensions/height", 4.3)
    train.set("dimensions/width", 2.9)
    train.set("dimensions/drag_coefficient", 1.2)
    train.set("dimensions/floor_height", 1.1)
    await wait_idle_frames(2)

    assert_eq(train.category, TrainController.CATEGORY_ROAD)
    assert_eq(train.train_type, TrainController.TRAIN_TYPE_ET22)
    assert_eq(train.reduced_mass, 500.0)
    assert_eq(train.sand_capacity, 300.0)
    assert_eq(train.heating_power, 20.0)
    assert_eq(train.light_power, 0.56)
    assert_eq(train.get("dimensions/length"), 15.5)
    assert_eq(train.get("dimensions/height"), 4.3)
    assert_eq(train.get("dimensions/width"), 2.9)
    assert_eq(train.get("dimensions/drag_coefficient"), 1.2)
    assert_almost_eq(train.get("dimensions/floor_height"), 1.1, 0.001)
    assert_true(train.state.has("mass_total"), "TrainController should keep functioning after configuring Param/Dimensions")
