extends MaszynaGutTest

var train: TrainController
var switches: TrainSwitches

func before_each():
    train = TrainController.new()
    train.train_id = "TestTrain"
    add_child(train)

    switches = TrainSwitches.new()
    train.add_child(switches)
    await wait_idle_frames(2)

func after_each():
    remove_child(train)
    train.free()

func _make_dimmer(high_beam: bool, dimmed: bool, off: bool) -> DimmerListItem:
    var item = DimmerListItem.new()
    item.high_beam = high_beam
    item.dimmed = dimmed
    item.off = off
    return item

func test_defaults():
    assert_false(switches.pantograph_impulse)
    assert_false(switches.converter_impulse)
    assert_true(switches.motor_connectors_impulse)
    assert_eq(switches.get("relay_reset_button/1"), 0)
    assert_false(switches.modern_dimmer)
    assert_eq((switches.get("dimmer_list/positions") as Array).size(), 0)

func test_round_trip_and_update_without_crashing():
    switches.pantograph_impulse = true
    switches.converter_impulse = true
    switches.motor_connectors_impulse = false
    switches.set("relay_reset_button/1", 1 | 2)
    switches.pantograph_presets = PackedInt32Array([0, 1, 3, 2])
    switches.pantograph_preset_default = 1
    switches.modern_dimmer = true
    switches.set("dimmer_list/cycle", false)
    switches.set("dimmer_list/default_position", 3)
    switches.set("dimmer_list/positions", [
        _make_dimmer(true, false, false),
        _make_dimmer(false, false, true),
    ])
    await wait_idle_frames(2)

    assert_true(switches.pantograph_impulse)
    assert_eq(switches.get("relay_reset_button/1"), 3)
    assert_eq(switches.pantograph_presets.size(), 4)
    var dimmer_list: Array = switches.get("dimmer_list/positions")
    assert_eq(dimmer_list.size(), 2)
    assert_true(is_instance_valid(train), "TrainController should keep functioning after configuring TrainSwitches")
