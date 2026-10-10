extends MaszynaGutTest

## LegacyCabinCabActivation: a cab whose MMD has no cabactivation_sw gauge (dynamic/pkp/e186_v2)
## is still activated through CabinSystem, as TTrain::OnCommand_cabactivationtoggle does regardless
## of the gauge (Train.cpp:3077).

var train: VehicleController
var logic: LegacyCabinLogic
## The front cabin, whose controls the logic registers
var cabin:RID


func before_each():
    train = build_vehicle("TestCabActivation", null, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    train.add_component(build_power_supply(110.0))
    # the active cab is the master controller's - a vehicle with a cab has one
    train.add_component(MoverRailVehicleMasterController.new())
    train.apply_configuration()
    # a cabin with no controls at all
    var controls: LegacyCabinControls = LegacyCabinControls.new()
    logic = LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return controls)
    cabin = RailVehicleServer.vehicle_get_front_cabin(train.get_rid())
    logic.register(train.get_rid(), cabin)
    await wait_idle_frames(2)


func after_each():
    logic.unregister()


func test_cab_without_the_gauge_registers_the_control():
    assert_true(CabinSystem.has_control(cabin, &"cabactivation_sw"))


func test_toggle_activates_and_deactivates_the_cab():
    assert_eq(train.get_state()["cabin"], 0)

    CabinSystem.act(cabin, &"cabactivation_sw", &"toggle")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["cabin"], 1)

    CabinSystem.act(cabin, &"cabactivation_sw", &"toggle")
    await wait_idle_frames(2)
    assert_eq(train.get_state()["cabin"], 0)
