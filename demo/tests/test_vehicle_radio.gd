extends MaszynaGutTest

## The train radio is a component of its own (RailVehicleRadio): the cab radio's channel and volume,
## the Mover's Radio flag, and Radio-Stop.

## Global.DefaultRadioVolume (Globals.h:181) and one press (Train.cpp:8252)
const VOLUME_DEFAULT:float = 0.75
const VOLUME_STEP:float = 0.125

var train: VehicleController
var radio: RailVehicleRadio


func before_each():
    train = build_vehicle("TestRadio")
    radio = MoverRailVehicleRadio.new()
    train.add_component(radio)
    await wait_idle_frames(2)


# Train.cpp:8246-8289 - an eighth per press, between silent and full
func test_volume_steps_and_stays_within_range():
    assert_eq(train.get_state()["radio_volume"], VOLUME_DEFAULT)
    train.send_command("radio_volume_increase", true)
    assert_eq(train.get_state()["radio_volume"], VOLUME_DEFAULT + VOLUME_STEP)
    for step:int in 10:
        train.send_command("radio_volume_increase", true)
    assert_eq(train.get_state()["radio_volume"], 1.0)
    for step:int in 10:
        train.send_command("radio_volume_decrease", true)
    assert_eq(train.get_state()["radio_volume"], 0.0)


func test_radio_switch_reaches_the_mover():
    train.send_command("radio", true)
    assert_true(train.get_state()["radio_enabled"])
    assert_false(train.get_state()["radio_powered"], "no low voltage, no power")
