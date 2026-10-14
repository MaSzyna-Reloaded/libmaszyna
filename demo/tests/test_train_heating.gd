extends MaszynaGutTest

var train: VehicleController
var heating: RailVehicleHeating

func before_each():
    train = build_vehicle("TestTrain")

    heating = MoverRailVehicleHeating.new()
    train.add_component(heating)
    await wait_idle_frames(2)

func test_defaults():
    assert_eq(heating.heating_source, RailVehicleController.POWER_SOURCE_GENERATOR)
    assert_eq(heating.heating_generator_engine, RailVehicleEngine.MAIN)
    assert_eq(heating.heating_generator_min_rpm, 0.0)
    assert_eq(heating.heating_generator_max_rpm, 0.0)
    assert_eq(heating.heating_power_cable_type, RailVehicleController.POWER_TYPE_ELECTRIC)
    assert_eq(heating.heating_max_voltage, 0.0)

func test_generator_source_updates_without_crashing():
    heating.heating_source = RailVehicleController.POWER_SOURCE_GENERATOR
    heating.heating_generator_min_rpm = 600.0
    heating.heating_generator_max_rpm = 1200.0
    heating.heating_generator_min_voltage = 100.0
    heating.heating_generator_max_voltage = 140.0
    await wait_idle_frames(2)

    assert_true(train.get_state().has("heating_enabled"), "VehicleController should keep functioning after configuring the generator heating source")

func test_power_cable_source_updates_without_crashing():
    heating.heating_source = RailVehicleController.POWER_SOURCE_POWERCABLE
    heating.heating_power_cable_type = RailVehicleController.POWER_TYPE_ELECTRIC
    heating.heating_max_voltage = 3000.0
    await wait_idle_frames(2)

    assert_true(train.get_state().has("heating_power"), "VehicleController should keep functioning after configuring the power cable heating source")
