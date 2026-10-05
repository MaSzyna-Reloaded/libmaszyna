extends MaszynaGutTest

## A cab's lamps are dark without the low voltage, whatever they show: lowvoltagepower
## (Power24vIsAvailable || Power110vIsAvailable, Train.cpp:8843) is handed to every lamp
## (TButton::Update(Power), Button.cpp:126; the btLampka block, Train.cpp:9022, 9242). The radio
## lamp of the fixtures' EP07 follows its radio switch, lit only while the battery gives the
## low voltage.

const EP07_PATH:String = "res://tests/fixtures/dynamic/pkp/303e_v1/303e-ep-tv.fiz"
## The lamps look at the vehicle every 0.1 simulated seconds (CabinIndicator3D._process,
## CabinSpotLight3D._process - a cab element's time is the simulation's)
const LAMP_REFRESH_SECONDS:float = 0.1
## Simulated seconds within which a lamp shows the vehicle's state - lit, dark, or proved to stay so:
## a refresh, and the step the low voltage changes on
const LAMP_SECONDS:float = LAMP_REFRESH_SECONDS + TICK

var train:VehicleController
var indicator:CabinIndicator3D
var spot_light:CabinSpotLight3D


func before_each() -> void:
    # a driven vehicle is simulated (FINDINGS, 09-23); the test drives it, no AI sits aboard
    train = build_vehicle("TestLampsLowVoltage", FizVehicleBuilder.build_description_at(EP07_PATH), 0.0,
            MaszynaDynamicData.DriverType.DRIVER_HEAD)
    indicator = CabinIndicator3D.new()
    indicator.state_property = "radio_enabled"
    add_child_autofree(indicator)
    indicator.set_vehicle_rid(train.get_rid())
    spot_light = CabinSpotLight3D.new()
    spot_light.state_property = "radio_enabled"
    add_child_autofree(spot_light)
    spot_light.set_vehicle_rid(train.get_rid())


func test_lamps_are_dark_without_the_low_voltage() -> void:
    VehicleServer.vehicle_send_command(train.get_rid(), "battery", false)
    VehicleServer.vehicle_send_command(train.get_rid(), "radio", true)
    assert_true(VehicleServer.vehicle_dump_state(train.get_rid())["radio_enabled"], "the radio is switched on")
    # proved dark by a refresh of the lamps
    await step(ticks(LAMP_SECONDS))
    assert_false(indicator.enabled, "no battery, no lamp")
    assert_false(spot_light.enabled, "no battery, no lamp")
    VehicleServer.vehicle_send_command(train.get_rid(), "battery", true)
    if not await wait_simulated_until(func() -> bool: return indicator.enabled and spot_light.enabled, LAMP_SECONDS,
            "the lamps lit by the battery"):
        return
    assert_true(VehicleServer.vehicle_dump_state(train.get_rid())["power24_available"]
            or VehicleServer.vehicle_dump_state(train.get_rid())["power110_available"], "the battery gives the low voltage")
    assert_true(indicator.enabled, "lit with the low voltage")
    assert_true(spot_light.enabled, "lit with the low voltage")
    VehicleServer.vehicle_send_command(train.get_rid(), "battery", false)
    if not await wait_simulated_until(func() -> bool: return not indicator.enabled and not spot_light.enabled,
            LAMP_SECONDS, "the lamps dark with the battery off"):
        return
    assert_false(indicator.enabled, "dark again without it")
    assert_false(spot_light.enabled, "dark again without it")
