extends MaszynaGutTest

var train: VehicleController
var brake: RailVehicleBrake

func before_each():
    train = build_vehicle("TestTrain")

    brake = MoverRailVehicleBrake.new()
    train.add_component(brake)
    await wait_idle_frames(2)

func test_defaults():
    assert_eq(brake.compressor_emergency_valve_area, 0.0)
    assert_eq(brake.universal_brake_button_1, 0)
    assert_eq(brake.universal_brake_button_2, 0)
    assert_eq(brake.universal_brake_button_3, 0)

func test_round_trip_and_update_without_crashing():
    brake.compressor_emergency_valve_area = 1.5
    brake.universal_brake_button_1 = 1  # releaser
    brake.universal_brake_button_2 = 16 # anti-skid brake
    brake.universal_brake_button_3 = 8  # assimilation
    await wait_idle_frames(2)

    assert_eq(brake.compressor_emergency_valve_area, 1.5)
    assert_eq(brake.universal_brake_button_1, 1)
    assert_eq(brake.universal_brake_button_2, 16)
    assert_eq(brake.universal_brake_button_3, 8)
    assert_true(train.get_state().has("brake_air_pressure"), "RailVehicleBrake should keep functioning after configuring universal brake buttons")
