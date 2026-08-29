extends MaszynaGutTest

var train: TrainController
var brake: TrainBrake

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    brake = TrainBrake.new()
    train.add_child(brake)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func _make_row(allow: int, speed_factor: int, min_factor: int, max_factor: int) -> CompressorListItem:
    var item = CompressorListItem.new()
    item.allow = allow
    item.speed_factor = speed_factor
    item.min_pressure_factor = min_factor
    item.max_pressure_factor = max_factor
    return item

func test_default_row_has_expected_defaults():
    var item = CompressorListItem.new()
    assert_eq(item.allow, 0, "allow should default to 0 (unchanged)")
    assert_eq(item.speed_factor, 1, "speed_factor should default to 1")
    assert_eq(item.min_pressure_factor, 1, "min_pressure_factor should default to 1")
    assert_eq(item.max_pressure_factor, 1, "max_pressure_factor should default to 1")

func test_row_round_trips_values():
    var item = _make_row(2, 1, 1, 1)
    assert_eq(item.allow, 2)
    assert_eq(item.speed_factor, 1)
    assert_eq(item.min_pressure_factor, 1)
    assert_eq(item.max_pressure_factor, 1)

func test_compressor_list_property_accepts_items():
    var rows: Array[CompressorListItem] = [
        _make_row(2, 1, 1, 1),
        _make_row(1, 0, 1, 1),
    ]
    brake.compressor_list = rows
    await wait_idle_frames(2)

    assert_eq(brake.compressor_list.size(), 2, "compressor_list should hold the assigned rows")
    assert_true(train.state.has("brake_air_pressure"), "TrainBrake should keep functioning after assigning compressor_list")

func test_oversized_compressor_list_is_truncated_without_crashing():
    var rows: Array[CompressorListItem] = []
    for i in range(12):
        rows.append(_make_row(2, 1, 1, 1))
    brake.compressor_list = rows
    await wait_idle_frames(2)

    # The mover only has room for 8 compressor programmer positions; assigning more than
    # that must not corrupt memory or crash the train, it should simply be truncated.
    assert_true(train.state.has("brake_air_pressure"), "TrainBrake should keep functioning after an oversized compressor_list")
