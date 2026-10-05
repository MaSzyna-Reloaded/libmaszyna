extends MaszynaGutTest

## The light selector (lights_sw) steps through the FIZ LightsList: presets and lights the vehicle
## with the preset it lands on (TTrain::OnCommand_lightspresetactivatenext, Train.cpp:5193;
## TDynamicObject::SetLights, DynObj.cpp:7322).

var train: VehicleController
var lighting: RailVehicleLighting


func before_each():
    lighting = MoverRailVehicleLighting.new()
    var upper := RailVehicleLightListItem.new()
    upper.cabin_a_head_light = true
    upper.cabin_b_left_red_signal = true
    upper.cabin_b_right_red_signal = true
    var lower_pair := RailVehicleLightListItem.new()
    lower_pair.cabin_a_left_white_signal = true
    lower_pair.cabin_a_right_white_signal = true
    var presets:Array[RailVehicleLightListItem] = [upper, lower_pair]
    lighting.lights_list = presets
    lighting.lights_default_selector_position = 1
    # in the description before the vehicle takes it, as the game builds one: the selector starts
    # where CheckLocomotiveParameters() puts it, after the configuration (Mover.cpp:8885)
    var description:VehicleController = MoverRailVehicleController.new()
    description.add_component(lighting)
    train = build_vehicle("TestLightPresets", description)


func test_the_selector_starts_at_the_default_preset():
    assert_eq(train.get_state()["light_position"], 1)
    assert_eq(train.get_state()["light_selector_position"], 0)
    assert_eq(train.get_config()["light_position_max"], 1)


func test_stepping_the_selector_lights_the_next_preset():
    train.send_command("increase_light_selector_position")
    assert_eq(train.get_state()["light_position"], 2)
    assert_true(train.get_state()["lights/front_headlight_left_enabled"])
    assert_true(train.get_state()["lights/front_headlight_right_enabled"])
    assert_false(train.get_state()["lights/front_headlight_upper_enabled"])

    train.send_command("decrease_light_selector_position")
    assert_eq(train.get_state()["light_position"], 1)
    assert_true(train.get_state()["lights/front_headlight_upper_enabled"])
    assert_true(train.get_state()["lights/rear_redmarker_left_enabled"])
    assert_true(train.get_state()["lights/rear_redmarker_right_enabled"])


# Train.cpp:5205 - without Wrap= the selector stops at its last preset
func test_the_selector_does_not_wrap_unless_told_to():
    train.send_command("increase_light_selector_position")
    train.send_command("increase_light_selector_position")
    assert_eq(train.get_state()["light_position"], 2)


# Train.cpp:2922-3135 - with a preset selector the single light switches do nothing
func test_the_single_light_switches_do_nothing_with_a_selector():
    # the upper preset, lit by stepping the selector away and back
    train.send_command("increase_light_selector_position")
    train.send_command("decrease_light_selector_position")
    assert_true(train.get_state()["lights/front_headlight_upper_enabled"])
    train.send_command("light_switch", "leftlight", false)
    train.send_command("light_switch", "upperlight", false)
    assert_true(train.get_state()["lights/front_headlight_upper_enabled"])
    train.send_command("light_switch", "leftlight", true)
    assert_false(train.get_state()["lights/front_headlight_left_enabled"])
