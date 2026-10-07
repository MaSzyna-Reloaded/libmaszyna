extends MaszynaGutTest

## Cab logic of a cab that models none of the catalog controls (dynamic/pkp/e186_v2 lacks
## main_on_bt, dirkey, shp_reset_bt, cabactivation_sw, ...): the original runs most OnCommand_*
## without their gauge, here LegacyCabinUnmodelledControls registers them in CabinSystem.

var train: VehicleController
var logic: LegacyCabinLogic
## The front cabin, whose controls the logic registers
var cabin:RID


func before_each():
    train = build_vehicle("TestUnmodelledControls")
    train.add_component(build_power_supply(110.0))
    train.apply_configuration()
    # the reverser does not move on a vehicle without a main controller (Mover.cpp DirectionForward)
    var master_controller: RailVehicleMasterController = MoverRailVehicleMasterController.new()
    master_controller.main_position_count = 4
    train.add_component(master_controller)
    var controls: LegacyCabinControls = LegacyCabinControls.new()
    logic = LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return controls)
    cabin = RailVehicleServer.vehicle_get_front_cabin(train.get_rid())
    logic.register(train.get_rid(), cabin)
    await wait_idle_frames(2)


func after_each():
    logic.unregister()


func test_catalog_controls_with_keys_are_registered_without_widgets():
    for control: StringName in [&"dirkey", &"main_on_bt", &"main_off_bt", &"shp_reset_bt", &"battery_sw"]:
        assert_true(CabinSystem.has_control(cabin, control), "%s should be registered" % control)


# Train.cpp:7934, 7978, 8022, 2284 - the horns and the sanding refuse the command in a cab without
# their gauge, so the cab takes neither their keys nor their wiring
func test_controls_refused_without_their_gauge_are_not_registered():
    for control: StringName in [&"horn_bt", &"hornlow_bt", &"hornhigh_bt", &"whistle_bt", &"sand_bt"]:
        assert_false(CabinSystem.has_control(cabin, control), "%s should not be registered" % control)


func test_knobs_are_registered_for_their_value():
    # the AI sets them by value (MaszynaLegacyDriverBraking); a key has none to give
    for control: StringName in [&"brakectrl", &"localbrake"]:
        assert_true(CabinSystem.has_control(cabin, control), "%s should be registered" % control)


func test_controls_sharing_keys_are_registered_once():
    # mainctrl and jointctrl both take main_controller_increase/decrease
    var registered: int = 0
    for control: StringName in [&"mainctrl", &"jointctrl"]:
        registered += 1 if CabinSystem.has_control(cabin, control) else 0
    assert_eq(registered, 1)


func test_unmodelled_dirkey_steps_the_reverser():
    train.send_command("battery", true)
    train.send_command("cab_activation", true)
    await wait_idle_frames(2)

    CabinSystem.act(cabin, &"dirkey", &"increase")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["direction"], VehicleController.DIRECTION_FORWARD)


func test_reverser_buttons_set_the_direction():
    train.send_command("battery", true)
    train.send_command("cab_activation", true)
    await wait_idle_frames(2)

    CabinSystem.act(cabin, &"dirbackward_bt", &"hold")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["direction"], VehicleController.DIRECTION_BACKWARD)

    CabinSystem.act(cabin, &"dirforward_bt", &"hold")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["direction"], VehicleController.DIRECTION_FORWARD)

    CabinSystem.act(cabin, &"dirneutral_bt", &"hold")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["direction"], VehicleController.DIRECTION_NEUTRAL)


## "cabin ... toggle" of the developer console sends no value
func test_toggle_without_value_does_not_fail():
    CabinSystem.act(cabin, &"main_on_bt", &"toggle")
    assert_eq(CabinSystem.get_control(cabin, &"main_on_bt"), true)
    CabinSystem.act(cabin, &"main_on_bt", &"toggle")
    assert_eq(CabinSystem.get_control(cabin, &"main_on_bt"), false)
    CabinSystem.act(cabin, &"dirforward_bt", &"toggle")
    assert_eq(CabinSystem.get_control(cabin, &"dirforward_bt"), true)
