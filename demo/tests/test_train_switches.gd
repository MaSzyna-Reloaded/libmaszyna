extends MaszynaGutTest

var train: VehicleController
var switches: RailVehicleSwitches

func before_each():
    train = build_vehicle("TestTrain")

    switches = MoverRailVehicleSwitches.new()
    train.add_component(switches)
    await wait_idle_frames(2)

func _make_dimmer(high_beam: bool, dimmed: bool, off: bool) -> RailVehicleDimmerListItem:
    var item = RailVehicleDimmerListItem.new()
    item.high_beam = high_beam
    item.dimmed = dimmed
    item.off = off
    return item

func test_defaults():
    assert_false(switches.pantograph_impulse)
    assert_false(switches.converter_impulse)
    assert_true(switches.motor_connectors_impulse)
    assert_eq(switches.relay_reset_button_1, 0)
    assert_false(switches.modern_dimmer)
    assert_eq((switches.dimmer_list_positions as Array).size(), 0)

func test_round_trip_and_update_without_crashing():
    switches.pantograph_impulse = true
    switches.converter_impulse = true
    switches.motor_connectors_impulse = false
    switches.relay_reset_button_1 = 1 | 2
    switches.pantograph_presets = PackedInt32Array([0, 1, 3, 2])
    switches.pantograph_preset_default = 1
    switches.modern_dimmer = true
    switches.dimmer_list_cycle = false
    switches.dimmer_list_default_position = 3
    switches.dimmer_list_positions = [
        _make_dimmer(true, false, false),
        _make_dimmer(false, false, true),
    ]
    await wait_idle_frames(2)

    assert_true(switches.pantograph_impulse)
    assert_eq(switches.relay_reset_button_1, 3)
    assert_eq(switches.pantograph_presets.size(), 4)
    var dimmer_list: Array = switches.dimmer_list_positions
    assert_eq(dimmer_list.size(), 2)
    assert_true(is_instance_valid(train), "VehicleController should keep functioning after configuring RailVehicleSwitches")


# Mover.cpp:11402 - PantsPreset "0|1|3|2" when the FIZ declares none
func test_default_pantograph_presets_are_the_originals():
    assert_eq(switches.pantograph_presets, PackedInt32Array([
            RailVehicleSwitches.PANTOGRAPH_PRESET_NONE, RailVehicleSwitches.PANTOGRAPH_PRESET_OWN_END,
            RailVehicleSwitches.PANTOGRAPH_PRESET_BOTH, RailVehicleSwitches.PANTOGRAPH_PRESET_OTHER_END]))


# Train.cpp:3522 - each digit of PantographPresets= is a preset
func test_fiz_pantograph_presets_map_to_the_enum():
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize("PantographPresets=0|3|1".to_utf8_buffer())
    var context:FizImportContext = FizImportContext.new()
    FizTrainSwitchesParser.new().parse(parser, context, "Switches:")
    var parsed:RailVehicleSwitches = context.get_part("RailVehicleSwitches")
    assert_eq(parsed.pantograph_presets, PackedInt32Array([
            RailVehicleSwitches.PANTOGRAPH_PRESET_NONE, RailVehicleSwitches.PANTOGRAPH_PRESET_BOTH,
            RailVehicleSwitches.PANTOGRAPH_PRESET_OWN_END]))


# The cab's open motor connectors button springs back unless the vehicle's is a toggle
# (StLinSwitchType, Train.cpp:5045)
func test_config_says_whether_the_motor_connectors_button_is_impulse():
    assert_true(train.get_config()["motor_connectors_switch_impulse"])
    switches.motor_connectors_impulse = false
    await wait_idle_frames(2)
    assert_false(train.get_config()["motor_connectors_switch_impulse"])


# Train.cpp:5045 - every MotorConnectors= value but "toggle" is an impulse button
func test_fiz_motor_connectors_is_impulse_unless_toggle():
    var cases:Dictionary[String, bool] = {"Toggle": false, "impulse": true, "push": true}
    for value:String in cases:
        var parser:MaszynaParser = MaszynaParser.new()
        parser.initialize(("MotorConnectors=%s" % value).to_utf8_buffer())
        var context:FizImportContext = FizImportContext.new()
        FizTrainSwitchesParser.new().parse(parser, context, "Switches:")
        var parsed:RailVehicleSwitches = context.get_part("RailVehicleSwitches")
        assert_eq(parsed.motor_connectors_impulse, cases[value], value)
