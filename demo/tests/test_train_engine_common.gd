extends MaszynaGutTest

var train: VehicleController
var engine: RailVehicleDieselEngine

func before_each():
    train = build_vehicle("TestTrain")

    engine = MoverRailVehicleDieselEngine.new()
    train.add_component(engine)
    await wait_idle_frames(2)

func test_defaults():
    assert_eq(engine.transmission_gear_teeth_motor, 0)
    assert_eq(engine.transmission_gear_teeth_wheel, 0)
    assert_eq(engine.transmission_efficiency, 1.0)
    assert_eq(engine.maximum_traction_force, 0.0)
    assert_eq(engine.motor_blowers_speed, 0.0)
    assert_false(engine.pressure_switch_present)
    assert_eq(engine.inverters_count, 0)

func test_round_trip_and_update_without_crashing():
    engine.transmission_gear_teeth_motor = 18
    engine.transmission_gear_teeth_wheel = 72
    engine.transmission_efficiency = 0.97
    engine.maximum_traction_force = 180.0
    engine.motor_blowers_speed = 1.0
    engine.motor_blowers_sustain_time = 30.0
    engine.motor_blowers_start_velocity = 10.0
    engine.pressure_switch_present = true
    engine.inverters_count = 2
    await wait_idle_frames(2)

    assert_eq(engine.transmission_gear_teeth_motor, 18)
    assert_eq(engine.transmission_gear_teeth_wheel, 72)
    assert_eq(engine.maximum_traction_force, 180.0)
    assert_true(engine.pressure_switch_present)
    assert_eq(engine.inverters_count, 2)
    assert_true(train.get_state().has("main_switch_enabled"), "RailVehicleEngine should keep functioning after configuring the common Engine: fields")
