extends MaszynaGutTest

## Regression coverage for a real bug found via the in-game console: radio_channel_set had no
## ClassDB::bind_method at all (register_command("radio_channel_set", Callable(this,
## "radio_channel_set")) silently built an invalid Callable), and radio_channel_min/max defaulted
## to 0/0 - a range the original engine never actually varies per vehicle
## (OnCommand_radiochannelset hardcodes std::clamp(..., 1, 10) for every vehicle) - so
## radio_channel_increase/decrease/set all silently clamped to a permanent 0 on every vehicle,
## since nothing anywhere sets radio_channel_min/max.

var train: VehicleController
var radio: RailVehicleRadio

func before_each():
    train = build_vehicle("TestTrain", load("res://tests/fixtures/sm42_vehicle.tres"))
    radio = MoverRailVehicleRadio.new()
    train.add_component(radio)
    await wait_idle_frames(2)

func test_defaults_match_the_original_engines_universal_1_to_10_range():
    assert_eq(radio.channel_min, 1)
    assert_eq(radio.channel_max, 10)

func test_radio_channel_starts_at_1_not_0():
    # confirmed real: vehicle/Driver.h defaults iRadioChannel to 1, not 0 - starting at 0 (below
    # the class's own valid 1..10 range) meant the very first radio_channel_increase call was
    # visually invisible, since CabinSwitch's own switch_min_position clamp had already displayed
    # the invalid 0 as channel 1 before any command ran.
    await wait_idle_frames(2)
    assert_eq(train.get_state()["radio_channel"], 1)

func test_radio_channel_set_is_a_valid_bound_command():
    train.send_command("radio_channel_set", 5)
    await wait_idle_frames(2)
    assert_eq(train.get_state()["radio_channel"], 5)

func test_radio_channel_increase_actually_changes_state():
    train.send_command("radio_channel_set", 3)
    await wait_idle_frames(2)
    train.send_command("radio_channel_increase")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["radio_channel"], 4)

func test_radio_channel_decrease_actually_changes_state():
    train.send_command("radio_channel_set", 3)
    await wait_idle_frames(2)
    train.send_command("radio_channel_decrease")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["radio_channel"], 2)
