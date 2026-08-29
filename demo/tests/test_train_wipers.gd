extends MaszynaGutTest

var train: TrainController
var wipers: TrainWipers

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    wipers = TrainWipers.new()
    train.add_child(wipers)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func _make_position(mask: int, transit_time: float, period: float, return_delay: float) -> WiperListItem:
    var item = WiperListItem.new()
    item.wiper_mask = mask
    item.transit_time = transit_time
    item.period = period
    item.return_delay = return_delay
    return item

func test_defaults():
    assert_eq(wipers.angle, 0.0)
    assert_eq(wipers.default_position, 0)
    assert_eq(wipers.positions.size(), 0)

func test_round_trip_and_update_without_crashing():
    wipers.angle = 58.0
    wipers.default_position = 2
    wipers.positions = [
        _make_position(0, 0.7, 0.0, 0.5),
        _make_position(3, 0.7, 7.0, 0.5),
        _make_position(3, 0.7, 0.7, 0.0),
        _make_position(3, 0.5, 0.5, 0.0),
    ]
    await wait_idle_frames(2)

    assert_eq(wipers.angle, 58.0)
    assert_eq(wipers.positions.size(), 4)
    assert_eq((wipers.positions[1] as WiperListItem).period, 7.0)
    assert_true(is_instance_valid(train), "TrainController should keep functioning after configuring TrainWipers")
