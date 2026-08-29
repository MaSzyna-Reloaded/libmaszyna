extends MaszynaGutTest

var train: TrainController
var engine: TrainDieselElectricEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainDieselElectricEngine.new()
    train.add_child(engine)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func _make_row(rpm: float, gen_power: float) -> WWListItem:
    var item = WWListItem.new()
    item.rpm = rpm
    item.max_power = gen_power
    return item

func test_defaults():
    assert_false(engine.generator_voltage_flat)
    assert_eq(engine.hyperbolic_speed, 1.0)
    assert_eq(engine.additional_speed, 1.0)
    assert_eq(engine.rpm_change_rate, 2.0)
    assert_eq(engine.power_correction_ratio, 1.0)
    assert_eq(engine.shunt_relay_type, 0)
    assert_false(engine.shunt_mode_allowed)
    assert_eq(engine.heating_rpm, 0.0)
    assert_eq(engine.wwlist.size(), 0)

func test_round_trip_and_wwlist_update():
    engine.generator_voltage_flat = true
    engine.hyperbolic_speed = 1.1
    engine.additional_speed = 1.2
    engine.rpm_change_rate = 1.25
    engine.power_correction_ratio = 0.95
    engine.shunt_relay_type = 1
    engine.shunt_mode_allowed = true
    engine.heating_rpm = 700.0
    engine.wwlist = [_make_row(696, 0), _make_row(629, 96)]
    await wait_idle_frames(2)

    assert_true(engine.generator_voltage_flat)
    assert_eq(engine.rpm_change_rate, 1.25)
    assert_eq(engine.wwlist.size(), 2)
    assert_true(train.state.has("main_switch_enabled"), "TrainDieselElectricEngine should keep functioning after configuring its Engine: fields and wwlist")

func test_inherited_mechanical_fields_stay_at_defaults_when_unused():
    # TrainDieselElectricEngine inherits TrainDieselEngine's mechanical-transmission
    # properties, but a diesel-electric vehicle should simply leave them at their defaults.
    await wait_idle_frames(2)
    assert_false(engine.get("torque_converter/present"))
    assert_false(engine.get("retarder/present"))
