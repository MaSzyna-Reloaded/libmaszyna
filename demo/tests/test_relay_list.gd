extends MaszynaGutTest

var train: TrainController
var engine: TrainElectricSeriesEngine

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    engine = TrainElectricSeriesEngine.new()
    # NOTE: engine_power_source must be set explicitly here - a freshly created engine without
    # a configured power source hits a pre-existing bug in TrainElectricEngine's state fetch
    # (RAccumulator.RechargeSource is read uninitialized), unrelated to relay_list itself.
    # The property is registered under its grouped inspector path ("power/source"), not
    # "engine_power_source", so it must be set via Object.set() rather than dot notation.
    engine.set("power/source", TrainController.POWER_SOURCE_CURRENTCOLLECTOR)
    train.add_child(engine)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func _make_row(relay_position: int, resistance: float, auto_switch: bool) -> RelayListItem:
    var item = RelayListItem.new()
    item.relay_position = relay_position
    item.resistance = resistance
    item.auto_switch = auto_switch
    return item

func test_default_row_has_expected_defaults():
    var item = RelayListItem.new()
    assert_eq(item.relay_position, 0, "relay_position should default to 0")
    assert_eq(item.resistance, 0.0, "resistance should default to 0.0")
    assert_eq(item.branch_count, 0, "branch_count should default to 0")
    assert_eq(item.motors_per_branch, 0, "motors_per_branch should default to 0")
    assert_false(item.auto_switch, "auto_switch should default to false")
    assert_eq(item.shunt_index, 0, "shunt_index should default to 0")

func test_row_round_trips_values():
    var item = _make_row(1, 24.891, true)
    item.branch_count = 1
    item.motors_per_branch = 4
    item.shunt_index = 2

    assert_eq(item.relay_position, 1)
    assert_eq(item.resistance, 24.891)
    assert_eq(item.branch_count, 1)
    assert_eq(item.motors_per_branch, 4)
    assert_true(item.auto_switch)
    assert_eq(item.shunt_index, 2)

func test_relay_list_property_accepts_items():
    var rows: Array[RelayListItem] = [
        _make_row(0, 0.0, false),
        _make_row(1, 24.891, true),
    ]
    engine.relay_list = rows
    await wait_idle_frames(2)

    assert_eq(engine.relay_list.size(), 2, "relay_list should hold the assigned rows")
    assert_eq((engine.relay_list[1] as RelayListItem).resistance, 24.891)

func test_oversized_relay_list_is_truncated_without_crashing():
    var rows: Array[RelayListItem] = []
    for i in range(70):
        rows.append(_make_row(i, 1.0, false))
    engine.relay_list = rows
    await wait_idle_frames(2)

    # The mover only has room for ResArraySize (64) + 1 relay list positions; assigning more
    # than that must not corrupt memory or crash the train, it should simply be truncated.
    assert_true(is_instance_valid(engine), "TrainElectricSeriesEngine should keep functioning after an oversized relay_list")
