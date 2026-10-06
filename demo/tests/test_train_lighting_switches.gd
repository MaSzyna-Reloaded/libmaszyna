extends MaszynaGutTest

## The single light switches (leftlight_sw:, rearleftlight_sw:, ...) light the end of the cab the
## driver sits in and the far one, whether that cab is switched on or not (cab_to_end(),
## Train.h:220-227; OnCommand_headlighttoggleleft, Train.cpp:5267)

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")

var train: VehicleController


func before_each():
    train = build_vehicle("TestLightSwitches", SM42, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    train.add_component(MoverRailVehicleLighting.new())
    train.apply_configuration()
    await wait_idle_frames(2)


## docs/findings-archive.md 2026-10-06: a switch in the SU46's rear cab, switched off, lit the front
## end's lamps - the end was taken from the active cab
func test_the_rear_cab_lights_its_own_end_switched_off():
    RailVehicleServer.person_move_to_rear_cabin(get_vehicle_driver(train.get_rid()))
    assert_eq(int(train.get_state()["cabin"]), 0, "no cab switched on")

    train.send_command("light_switch", "right", true)
    train.send_command("light_switch", "rearleft", true)

    var state:Dictionary = train.get_state()
    assert_true(state["lights/rear_headlight_right_enabled"], "its own end's right lamp")
    assert_false(state["lights/front_headlight_right_enabled"], "not the front end's")
    assert_true(state["lights/front_headlight_left_enabled"], "the far end's left lamp")
    assert_false(state["lights/rear_headlight_left_enabled"], "not its own end's")
