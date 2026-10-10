extends MaszynaGutTest

var train: VehicleController

func before_each():
    train = build_vehicle("TestTrain")
    await wait_idle_frames(2)

func test_param_and_dimensions_defaults():
    assert_eq(train.category, VehicleController.CATEGORY_TRAIN)
    assert_eq(train.train_type, RailVehicleController.TRAIN_TYPE_DEFAULT)
    assert_eq(train.reduced_mass, 0.0)
    assert_eq(train.sand_capacity, 0.0)
    assert_eq(train.heating_power, 0.0)
    assert_eq(train.light_power, 0.0)
    assert_eq(train.dimensions_length, 0.0)
    assert_eq(train.dimensions_height, 0.0)
    assert_eq(train.dimensions_width, 0.0)
    assert_eq(train.dimensions_drag_coefficient, 0.0)
    assert_eq(train.dimensions_floor_height, 0.96)

func test_param_and_dimensions_round_trip_and_update():
    train.category = VehicleController.CATEGORY_ROAD
    train.train_type = RailVehicleController.TRAIN_TYPE_ET22
    train.reduced_mass = 500.0
    train.sand_capacity = 300.0
    train.heating_power = 20.0
    train.light_power = 0.56
    train.dimensions_length = 15.5
    train.dimensions_height = 4.3
    train.dimensions_width = 2.9
    train.dimensions_drag_coefficient = 1.2
    train.dimensions_floor_height = 1.1
    await wait_idle_frames(2)

    assert_eq(train.category, VehicleController.CATEGORY_ROAD)
    assert_eq(train.train_type, RailVehicleController.TRAIN_TYPE_ET22)
    assert_eq(train.reduced_mass, 500.0)
    assert_eq(train.sand_capacity, 300.0)
    assert_eq(train.heating_power, 20.0)
    assert_eq(train.light_power, 0.56)
    assert_eq(train.dimensions_length, 15.5)
    assert_eq(train.dimensions_height, 4.3)
    assert_eq(train.dimensions_width, 2.9)
    assert_eq(train.dimensions_drag_coefficient, 1.2)
    assert_almost_eq(train.dimensions_floor_height, 1.1, 0.001)
    assert_true(train.get_state().has("mass_total"), "VehicleController should keep functioning after configuring Param/Dimensions")
