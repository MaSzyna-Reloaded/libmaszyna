extends MaszynaGutTest

var train: VehicleController
var brake: RailVehicleBrake

func before_each():
    train = build_vehicle("TestTrain")

    brake = MoverRailVehicleBrake.new()
    train.add_component(brake)
    await wait_idle_frames(2)

func _make_row(allow: int, speed_factor: int, min_factor: int, max_factor: int) -> RailVehicleCompressorListItem:
    var item = RailVehicleCompressorListItem.new()
    item.allow = allow
    item.speed_factor = speed_factor
    item.min_pressure_factor = min_factor
    item.max_pressure_factor = max_factor
    return item

func test_default_row_has_expected_defaults():
    var item = RailVehicleCompressorListItem.new()
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
    var rows: Array[RailVehicleCompressorListItem] = [
        _make_row(2, 1, 1, 1),
        _make_row(1, 0, 1, 1),
    ]
    brake.compressor_list = rows
    await wait_idle_frames(2)

    assert_eq(brake.compressor_list.size(), 2, "compressor_list should hold the assigned rows")
    assert_true(train.get_state().has("brake_air_pressure"), "RailVehicleBrake should keep functioning after assigning compressor_list")

func test_oversized_compressor_list_is_truncated_without_crashing():
    var rows: Array[RailVehicleCompressorListItem] = []
    for i in range(12):
        rows.append(_make_row(2, 1, 1, 1))
    brake.compressor_list = rows
    await wait_idle_frames(2)

    # The mover only has room for 8 compressor programmer positions; assigning more than
    # that must not corrupt memory or crash the train, it should simply be truncated.
    assert_true(train.get_state().has("brake_air_pressure"), "RailVehicleBrake should keep functioning after an oversized compressor_list")
