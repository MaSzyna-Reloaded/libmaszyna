extends MaszynaGutTest

## The converter switch (TTrain::OnCommand_convertertoggle/converterenable/converterdisable,
## Train.cpp:4382-4458): a two-state one turns the converter on and off by what it shows; an impulse
## one (Switches: Converter=impulse) turns it on only with the main circuit closed and springs back.

const LOCOMOTIVE_PATH:String = "res://tests/fixtures/dynamic/pkp/ep09_v1/104e-039.fiz"
## An EN57 cab car: Switches: Converter=impulse, no engine of its own
const IMPULSE_PATH:String = "res://tests/fixtures/dynamic/pkp/en57-2000_v1/6ba.fiz"

var vehicle_rid:RID
## The front cabin, the driver's
var cabin:RID
var converter:LegacyCabinConverter
var received:Array[Array] = []


func _drive(fiz_path:String) -> void:
    var train:VehicleController = build_vehicle("TestCabinConverter", FizVehicleBuilder.build_description_at(fiz_path),
            0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    vehicle_rid = train.get_rid()
    await wait_idle_frames(2)
    var config:Dictionary = CabinSystem.vehicle_config(vehicle_rid)
    converter = LegacyCabinConverter.new(bool(config.get("converter_switch_impulse", false)), false)
    cabin = RailVehicleServer.vehicle_get_front_cabin(vehicle_rid)
    converter.register(vehicle_rid, cabin)
    VehicleServer.vehicle_command_received.connect(_on_command)


func after_each() -> void:
    converter.unregister()
    VehicleServer.vehicle_command_received.disconnect(_on_command)
    received.clear()


func _on_command(rid:RID, command:String, p1:Variant, _p2:Variant) -> void:
    if rid == vehicle_rid and command == "converter":
        received.append([command, p1])


func test_a_two_state_switch_turns_the_converter_on_and_off() -> void:
    await _drive(LOCOMOTIVE_PATH)
    assert_false(CabinSystem.vehicle_config(vehicle_rid).get("converter_switch_impulse", true))
    CabinSystem.act(cabin, LegacyCabinConverter.SWITCH, &"toggle")
    CabinSystem.act(cabin, LegacyCabinConverter.SWITCH, &"toggle")
    assert_eq(received, [["converter", true], ["converter", false]] as Array[Array])


func test_an_impulse_switch_needs_the_main_circuit_and_springs_back() -> void:
    await _drive(IMPULSE_PATH)
    assert_true(CabinSystem.vehicle_config(vehicle_rid).get("converter_switch_impulse", false))
    CabinSystem.act(cabin, LegacyCabinConverter.SWITCH, &"hold")
    assert_true(CabinSystem.get_control(cabin, LegacyCabinConverter.SWITCH), "pressed")
    assert_eq(received, [] as Array[Array], "no main circuit, no converter")
    CabinSystem.act(cabin, LegacyCabinConverter.SWITCH, &"release")
    assert_false(CabinSystem.get_control(cabin, LegacyCabinConverter.SWITCH), "sprung back")
