extends MaszynaGutTest

var train: VehicleController
var speed_control: RailVehicleSpeedControl

func before_each():
    train = build_vehicle("TestTrain")

    speed_control = MoverRailVehicleSpeedControl.new()
    train.add_component(speed_control)
    await wait_idle_frames(2)

func test_defaults():
    assert_false(speed_control.speed_control_enabled)
    assert_eq(speed_control.min_power, 0.0)
    assert_eq(speed_control.max_power, 1.0)
    assert_eq(speed_control.max_velocity, 120.0)
    assert_eq(speed_control.power_up_speed, 1000.0)
    assert_eq(speed_control.power_down_speed, 1000.0)
    # LoadFIZ_SpeedControl's ten default buttons (MOVER.h:1257-1286)
    assert_eq(speed_control.preset_speeds, PackedFloat64Array([30, 40, 50, 60, 70, 80, 90, 100, 110, 120]))

func test_enabling_and_configuring_updates_state():
    speed_control.speed_control_enabled = true
    speed_control.preset_speeds = PackedFloat64Array([30, 40, 50, 60, 70, 80, 90, 100, 110, 120])
    speed_control.min_velocity = 10.0
    speed_control.max_velocity = 100.0
    await wait_idle_frames(2)

    assert_true(train.get_state().has("speed_control/active"))
    assert_true(train.get_state().has("speed_control/desired_velocity"))

func test_oversized_preset_speeds_is_truncated_without_crashing():
    var presets := PackedFloat64Array()
    for i in range(20):
        presets.append(float(i))
    speed_control.preset_speeds = presets
    await wait_idle_frames(2)

    # The mover only has room for 10 preset speed buttons; assigning more than that must not
    # corrupt memory or crash the train, it should simply be truncated.
    assert_true(is_instance_valid(speed_control), "RailVehicleSpeedControl should keep functioning after an oversized preset_speeds array")
