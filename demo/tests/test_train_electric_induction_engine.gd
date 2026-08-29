extends MaszynaGutTest

var train: TrainController
var engine: TrainElectricInductionEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainElectricInductionEngine.new()
    engine.set("power/source", TrainController.POWER_SOURCE_CURRENTCOLLECTOR)
    train.add_child(engine)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func _make_point(x: float, y: float) -> CurvePointItem:
    var item = CurvePointItem.new()
    item.x = x
    item.y = y
    return item

func test_defaults():
    assert_eq(engine.slip_current_ratio, 0.0)
    assert_eq(engine.pole_pairs, 0.0)
    assert_eq(engine.max_power, 0.0)
    assert_eq(engine.motor_max_current, 0.0)
    assert_eq(engine.max_power_table.size(), 0)

func test_round_trip_and_update_without_crashing():
    engine.slip_current_ratio = 0.1
    engine.max_slip = 0.2
    engine.pole_pairs = 2.0
    engine.nominal_uf_ratio = 1.5
    engine.current_torque_ratio = 0.9
    engine.current_three_phase_ratio = 0.8
    engine.max_supply_voltage = 2800.0
    engine.max_supply_voltage_braking = 2400.0
    engine.inverter_voltage_drop = 10.0
    engine.no_load_current = 5.0
    engine.inverter_uf_setpoint = 1.0
    engine.inverter_uf_setpoint_braking = 0.9
    engine.initial_force = 200.0
    engine.force_drop_rate = 1.5
    engine.max_power = 1200.0
    engine.max_braking_force = 180.0
    engine.max_braking_power = 1000.0
    engine.braking_decay_velocity = 5.0
    engine.braking_decay_start_velocity = 20.0
    engine.motor_max_current = 600.0
    engine.max_power_table = [_make_point(0.0, 1200.0), _make_point(100.0, 600.0)]
    await wait_idle_frames(2)

    assert_eq(engine.slip_current_ratio, 0.1)
    assert_eq(engine.max_power, 1200.0)
    assert_eq(engine.max_power_table.size(), 2)
    assert_true(train.state.has("main_switch_enabled"), "TrainElectricInductionEngine should keep functioning after configuring EIM parameters")
