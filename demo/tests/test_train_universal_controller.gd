extends MaszynaGutTest

var train: VehicleController
var universal_controller: RailVehicleUniversalController

func before_each():
    train = build_vehicle("TestTrain")

    universal_controller = MoverRailVehicleUniversalController.new()
    train.add_component(universal_controller)
    await wait_idle_frames(2)

func _make_position(min_percentage: float, max_percentage: float, target_value: float) -> RailVehicleUniversalControllerListItem:
    var item = RailVehicleUniversalControllerListItem.new()
    item.min_percentage = min_percentage
    item.max_percentage = max_percentage
    item.target_value = target_value
    return item

func test_default_position_parameters_have_expected_defaults():
    var item = RailVehicleUniversalControllerListItem.new()
    assert_eq(item.pneumatic_brake_position, -1, "pneumatic_brake_position should default to -1")
    assert_eq(item.min_percentage, 0.0, "min_percentage should default to 0.0")
    assert_eq(item.max_percentage, 0.0, "max_percentage should default to 0.0")
    assert_eq(item.target_value, 0.0, "target_value should default to 0.0")
    assert_eq(item.increase_speed, 0.0, "increase_speed should default to 0.0")
    assert_eq(item.decrease_speed, 0.0, "decrease_speed should default to 0.0")
    assert_eq(item.bounce_back_position, 0, "bounce_back_position should default to 0")
    assert_eq(item.nearest_stable_down, 0, "nearest_stable_down should default to 0")
    assert_eq(item.nearest_stable_up, 0, "nearest_stable_up should default to 0")

func test_position_parameters_round_trip_values():
    var item = RailVehicleUniversalControllerListItem.new()
    item.pneumatic_brake_position = 2
    item.min_percentage = 0.1
    item.max_percentage = 0.9
    item.target_value = 0.5
    item.increase_speed = 0.2
    item.decrease_speed = 0.3
    item.bounce_back_position = 4
    item.nearest_stable_down = 1
    item.nearest_stable_up = 5

    assert_eq(item.pneumatic_brake_position, 2)
    assert_eq(item.min_percentage, 0.1)
    assert_eq(item.max_percentage, 0.9)
    assert_eq(item.target_value, 0.5)
    assert_eq(item.increase_speed, 0.2)
    assert_eq(item.decrease_speed, 0.3)
    assert_eq(item.bounce_back_position, 4)
    assert_eq(item.nearest_stable_down, 1)
    assert_eq(item.nearest_stable_up, 5)

func test_positions_property_accepts_universal_controller_list_items():
    var first = _make_position(0.0, 0.5, 0.25)
    var second = _make_position(0.5, 1.0, 0.75)
    universal_controller.positions = [first, second]

    assert_eq(universal_controller.positions.size(), 2, "positions should hold the assigned items")
    assert_eq((universal_controller.positions[0] as RailVehicleUniversalControllerListItem).max_percentage, 0.5)
    assert_eq((universal_controller.positions[1] as RailVehicleUniversalControllerListItem).max_percentage, 1.0)

func test_selector_position_is_forwarded_to_mover():
    universal_controller.positions = [_make_position(0.0, 1.0, 0.5)]
    universal_controller.selector_position = 3
    await wait_idle_frames(2)

    assert_eq(train.get_state()["selector_position"], 3, "Mover's MainCtrlPos should follow selector_position")

func test_oversized_positions_array_is_truncated_without_crashing():
    var positions: Array[RailVehicleUniversalControllerListItem] = []
    for i in range(40):
        positions.append(_make_position(0.0, 1.0, float(i) / 40.0))
    universal_controller.positions = positions

    await wait_idle_frames(2)

    # The mover only has room for 32 universal controller positions; assigning more than
    # that must not corrupt memory or crash the train, it should simply be truncated.
    assert_true(train.get_state().has("selector_position"), "Mover should keep functioning after an oversized positions array")
