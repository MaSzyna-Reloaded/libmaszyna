extends MaszynaGutTest

var train: TrainController
var engine: TrainDieselEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainDieselEngine.new()
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

func _make_throttle(position: int, fuel_dose: float, behavior: int) -> ThrottlePositionItem:
    var item = ThrottlePositionItem.new()
    item.throttle_position = position
    item.fuel_dose = fuel_dose
    item.clutch_behavior = behavior
    return item

func test_defaults():
    assert_eq(engine.get("mechanical/min_rpm"), 0.0)
    assert_eq(engine.get("mechanical/max_rpm"), 0.0)
    assert_eq(engine.get("mechanical/inertia"), 1.0)
    assert_false(engine.get("torque_converter/present"))
    assert_false(engine.get("retarder/present"))
    assert_eq(engine.get("retarder/placement"), TrainDieselEngine.RETARDER_PLACEMENT_AFTER_GEARBOX)
    assert_eq((engine.get("throttle_table/positions") as Array).size(), 0)
    assert_eq(engine.torque_table.size(), 0)
    assert_eq((engine.get("torque_converter/table") as Array).size(), 0)

func test_mechanical_and_torque_converter_round_trip():
    engine.set("mechanical/min_rpm", 600.0)
    engine.set("mechanical/max_rpm", 2000.0)
    engine.set("mechanical/fuel_cutoff_rpm", 2100.0)
    engine.set("mechanical/inertia", 1.5)
    engine.set("mechanical/clutch/engage_speed", 0.6)
    engine.set("mechanical/clutch/disengage_speed", 0.8)
    engine.set("torque_converter/present", true)
    engine.set("torque_converter/max_torque_ratio", 2.5)
    engine.set("torque_converter/coupling_point", 0.9)
    engine.set("retarder/present", true)
    engine.set("retarder/placement", TrainDieselEngine.RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC)
    engine.set("retarder/max_torque", 500.0)
    engine.set("torque_converter/table", [_make_point(0.0, 4.89), _make_point(1.0, 0.0)])
    await wait_idle_frames(2)

    assert_eq(engine.get("mechanical/min_rpm"), 600.0)
    assert_eq(engine.get("mechanical/max_rpm"), 2000.0)
    assert_true(engine.get("torque_converter/present"))
    assert_eq(engine.get("torque_converter/max_torque_ratio"), 2.5)
    assert_true(engine.get("retarder/present"))
    assert_eq(engine.get("retarder/placement"), TrainDieselEngine.RETARDER_PLACEMENT_BETWEEN_GEARBOX_AND_TC)
    assert_eq((engine.get("torque_converter/table") as Array).size(), 2)
    assert_true(train.state.has("main_switch_enabled"), "TrainDieselEngine should keep functioning after configuring mechanical/torque converter/retarder")

func test_throttle_table_and_torque_curve_round_trip():
    engine.set("throttle_table/max_torque", 1400.0)
    engine.set("throttle_table/max_torque_rpm", 1200.0)
    engine.set("throttle_table/nominal_fuel_dose", 1.0)
    engine.set("throttle_table/nominal_fuel_consumption_rate", 210.0)
    engine.set("throttle_table/positions", [
        _make_throttle(0, 0.0, ThrottlePositionItem.CLUTCH_BEHAVIOR_NONE),
        _make_throttle(1, 0.15, ThrottlePositionItem.CLUTCH_BEHAVIOR_HALF_CLUTCH_MIN_RPM),
    ])
    engine.torque_table = [_make_point(850, 1450), _make_point(2000, 2174)]
    await wait_idle_frames(2)

    var throttle_table: Array = engine.get("throttle_table/positions")
    assert_eq(throttle_table.size(), 2)
    assert_eq((throttle_table[1] as ThrottlePositionItem).fuel_dose, 0.15)
    assert_eq(engine.torque_table.size(), 2)
    assert_true(train.state.has("main_switch_enabled"), "TrainDieselEngine should keep functioning after configuring the throttle table and torque curve")

func test_oversized_throttle_table_is_truncated_without_crashing():
    var rows: Array[ThrottlePositionItem] = []
    for i in range(70):
        rows.append(_make_throttle(i, 0.0, ThrottlePositionItem.CLUTCH_BEHAVIOR_NONE))
    engine.set("throttle_table/positions", rows)
    await wait_idle_frames(2)

    assert_true(is_instance_valid(engine), "TrainDieselEngine should keep functioning after an oversized throttle_table")
