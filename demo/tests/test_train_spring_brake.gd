extends MaszynaGutTest

var train: VehicleController

func before_each():
    train = build_vehicle("TestTrain", load("res://tests/fixtures/sm42_vehicle.tres"))
    await step(2)
    train.send_command("battery", true)
    await step(2)

func test_successful_activate_spring_brake():
    train.send_command("set_spring_brake_active", true)
    await step(2)
    assert_true(train.get_state()["spring_brake/active"], "Spring brake should be active")

func test_successful_deactivate_spring_brake():
    train.send_command("set_spring_brake_active", false)
    await step(2)
    assert_false(train.get_state()["spring_brake/active"], "Spring brake should not be active")

func test_successful_enable_spring_brake():
    train.send_command("set_spring_brake_enabled", true)
    await step(2)
    assert_false(train.get_state()["spring_brake/shut_off"], "An enabled spring brake is not shut off")

func test_successful_disable_spring_brake():
    train.send_command("set_spring_brake_enabled", false)
    await step(2)
    assert_true(train.get_state()["spring_brake/shut_off"], "A disabled spring brake is shut off")

func test_configured_spring_brake_starts_armed_and_not_shut_off():
    assert_false(train.get_state()["spring_brake/shut_off"], "Spring brake should not start shut off")
    assert_true(train.get_state()["spring_brake/is_ready"], "Spring brake should start armed")
