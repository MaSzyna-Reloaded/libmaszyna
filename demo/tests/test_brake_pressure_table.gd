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

func _make_row(handle_position: int, pipe_pressure: float, brake_type: int) -> BrakePressureTableItem:
    var item = BrakePressureTableItem.new()
    item.handle_position = handle_position
    item.pipe_pressure = pipe_pressure
    item.brake_type = brake_type
    return item

func test_default_row_has_expected_defaults():
    var item = BrakePressureTableItem.new()
    assert_eq(item.handle_position, 0, "handle_position should default to 0")
    assert_eq(item.pipe_pressure, 0.0, "pipe_pressure should default to 0.0")
    assert_eq(item.brake_cylinder_pressure, -1.0, "brake_cylinder_pressure should default to -1.0")
    assert_eq(item.fill_speed, 0.0, "fill_speed should default to 0.0")
    assert_eq(item.brake_type, BrakePressureTableItem.BRAKE_TYPE_PNEUMATIC, "brake_type should default to Pneumatic")

func test_row_round_trips_values():
    var item = _make_row(-1, 0.7, BrakePressureTableItem.BRAKE_TYPE_ELECTRO_PNEUMATIC)
    item.brake_cylinder_pressure = 3.5
    item.fill_speed = 15.0

    assert_eq(item.handle_position, -1)
    assert_eq(item.pipe_pressure, 0.7)
    assert_eq(item.brake_cylinder_pressure, 3.5)
    assert_eq(item.fill_speed, 15.0)
    assert_eq(item.brake_type, BrakePressureTableItem.BRAKE_TYPE_ELECTRO_PNEUMATIC)

func test_brake_pressure_table_accepts_negative_handle_positions():
    var rows: Array[BrakePressureTableItem] = [
        _make_row(-1, 0.7, BrakePressureTableItem.BRAKE_TYPE_PNEUMATIC),
        _make_row(0, 0.5, BrakePressureTableItem.BRAKE_TYPE_PNEUMATIC),
        _make_row(6, 0.0, BrakePressureTableItem.BRAKE_TYPE_PNEUMATIC),
    ]
    brake.brake_pressure_table = rows
    await wait_idle_frames(2)

    assert_eq(brake.brake_pressure_table.size(), 3, "brake_pressure_table should hold the assigned rows")
    assert_true(train.state.has("brake_air_pressure"), "TrainBrake should keep functioning after assigning brake_pressure_table")
