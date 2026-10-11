extends MaszynaGutTest

## Catalog entries added after the "unhandled MMD label" survey - each pins down the exact
## command/state_property pair verified against the C++ wrapper source (see
## mmd_semantic_catalog.gd's own inline comments for the exact source lines), so a future
## refactor that accidentally breaks one of these strings fails a test instead of silently
## regressing in-game.

func test_battery_sw_uses_battery_command_and_state():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("battery_sw")
    assert_eq(entry["widget_class"], CabinButton)
    assert_eq(entry["fixed_fields"]["command"], "battery")
    assert_eq(entry["fixed_fields"]["state_property"], "battery_enabled")
    assert_eq(entry["fixed_fields"]["action"], "battery_toggle")


# the converter switch is the cab logic's (LegacyCabinConverter, OnCommand_convertertoggle,
# Train.cpp:4382-4458): its key, and its kind - an impulse one springs back - the vehicle's
func test_converter_sw_is_the_cab_logics_and_springs_back_by_the_vehicle():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("converter_sw")
    assert_eq(entry["widget_class"], CabinButton)
    assert_eq(entry["fixed_fields"]["action"], "converter_toggle")
    assert_eq(entry["monostable_from_config"], "converter_switch_impulse")
    assert_false(entry["fixed_fields"].has("command"), "the cab logic sends the command")


func test_stlinoff_bt_springs_back_unless_the_vehicles_button_is_a_toggle():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("stlinoff_bt")
    assert_eq(entry["widget_class"], CabinButton)
    assert_true(entry["fixed_fields"]["monostable"], "impulse is the original's default (Train.cpp:5045)")
    assert_eq(entry["monostable_from_config"], "motor_connectors_switch_impulse")
    assert_eq(entry["fixed_fields"]["command"], "motor_connectors_open")


func test_compressor_sw_uses_compressor_command_and_state():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("compressor_sw")
    assert_eq(entry["widget_class"], CabinButton)
    assert_eq(entry["fixed_fields"]["command"], "compressor")
    assert_eq(entry["fixed_fields"]["state_property"], "compressor_enabled")
    assert_eq(entry["fixed_fields"]["action"], "compressor_toggle")


func test_radiochannel_sw_is_an_interactive_multi_position_switch():
    # some vehicles (Radmor-style radios) have a real turnable selector knob under this label -
    # CabinGauge is display-only (no input), so it must be CabinSwitch like mainctrl.
    var entry:Dictionary = MmdSemanticCatalog.get_entry("radiochannel_sw")
    assert_eq(entry["widget_class"], CabinSwitch)
    assert_eq(entry["fixed_fields"]["command_increase"], "radio_channel_increase")
    assert_eq(entry["fixed_fields"]["command_decrease"], "radio_channel_decrease")
    assert_eq(entry["fixed_fields"]["state_property"], "radio_channel")
    assert_eq(entry["fixed_fields"]["switch_min_position"], 1)
    assert_eq(entry["fixed_fields"]["switch_max_position"], 10)
    # channel 1 is the knob's physical rest position, not switch_position=0 (0 isn't a valid
    # channel at all) - without this the knob renders one full step past rest for every channel.
    assert_eq(entry["fixed_fields"]["value_offset"], 1)


func test_radiochannelnext_sw_fires_increase_once_per_press():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("radiochannelnext_sw")
    assert_eq(entry["widget_class"], CabinButton)
    assert_eq(entry["fixed_fields"]["command"], "radio_channel_increase")
    assert_eq(entry["fixed_fields"]["controller_mode"], CabinButton.ControllerMode.On)
    assert_eq(entry["fixed_fields"]["action"], "radio_channel_increase")


func test_radiochannelprev_sw_fires_decrease_once_per_press():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("radiochannelprev_sw")
    assert_eq(entry["widget_class"], CabinButton)
    assert_eq(entry["fixed_fields"]["command"], "radio_channel_decrease")
    assert_eq(entry["fixed_fields"]["controller_mode"], CabinButton.ControllerMode.On)
    assert_eq(entry["fixed_fields"]["action"], "radio_channel_decrease")


# LegacyCabinPantographs owns pantfront_sw - it springs back when the vehicle's pantograph
# switches are impulse ones (Train.cpp:3170), and carries no vehicle command of its own
func test_pantfront_sw_is_shaped_by_the_vehicles_switch_type():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("pantfront_sw")
    assert_eq(entry["widget_class"], CabinButton)
    assert_eq(entry["monostable_from_config"], "pantograph_switch_impulse")
    assert_false(entry["fixed_fields"].has("command"))
    assert_eq(entry["fixed_fields"]["state_property"], "current_collector/pantograph_first_active")


func test_distcounter_binds_total_distance():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("distcounter")
    assert_eq(entry["widget_class"], CabinGauge)
    assert_eq(entry["fixed_fields"]["state_property"], "total_distance")


func test_hvcurrent1_binds_current1():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("hvcurrent1")
    assert_eq(entry["widget_class"], CabinGauge)
    assert_eq(entry["fixed_fields"]["state_property"], "current1")


func test_i_radio_indicator_and_powered_omnilight_are_separate():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("i-radio")
    assert_eq(entry["widget_class"], CabinSpotLight3D)
    assert_eq(entry["fixed_fields"]["state_property"], "radio_enabled")
    assert_true(entry["position_at_submodel"])
    assert_eq(entry["light_widget_class"], CabinOmniLight3D)
    assert_eq(entry["light_fixed_fields"]["state_property"], "radio_powered")
    # the lamp takes the submodel's diffuse, which tints its greyscale texture (Model3d.cpp:1918)
    assert_true(entry["light_color_from_submodel"])
    assert_false(entry["light_fixed_fields"].has("light_color"))
    assert_eq(entry["light_fixed_fields"]["light_energy_on"], 0.05)
    assert_eq(entry["light_fixed_fields"]["omni_range"], 0.1)


func test_brake_cylinder_and_driver_valve_control_reservoir_gauges():
    var brakepressb:Dictionary = MmdSemanticCatalog.get_entry("brakepressb")
    var limpipepress:Dictionary = MmdSemanticCatalog.get_entry("limpipepress")
    assert_eq(brakepressb["widget_class"], CabinGauge)
    assert_eq(brakepressb["fixed_fields"]["state_property"], "brake_air_pressure")
    assert_eq(brakepressb["mmd_scale_multiplier"], 0.1)
    assert_eq(limpipepress["widget_class"], CabinGauge)
    assert_eq(limpipepress["fixed_fields"]["state_property"], "brake_handle_control_pressure")
    assert_eq(limpipepress["mmd_scale_multiplier"], 0.1)


func test_spring_brake_indicators_show_the_spring_braking_and_its_inverse():
    var active:Dictionary = MmdSemanticCatalog.get_entry("i-springbrakeactive")
    var inactive:Dictionary = MmdSemanticCatalog.get_entry("i-springbrakeinactive")
    assert_eq(active["fixed_fields"]["state_property"], "spring_brake/braking")
    assert_false(active["fixed_fields"].get("invert_value", false))
    assert_eq(inactive["widget_class"], CabinIndicator3D)
    assert_eq(inactive["fixed_fields"]["state_property"], "spring_brake/braking")
    assert_true(inactive["fixed_fields"]["invert_value"])


func test_cab_light_indicator_and_spotlight_are_separate():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("i-cablight")
    assert_eq(entry["widget_class"], CabinIndicator3D)
    assert_eq(entry["fixed_fields"]["cab_light"], CabinState.Light.CAB)
    assert_eq(entry["light_widget_class"], CabinSpotLight3D)
    assert_true(entry["flip_upward_spotlight"])
    # the level: dimmed (cablightdim_sw) and 24 V-only it shines at part of its energy
    assert_eq(entry["light_fixed_fields"]["cab_light"], CabinState.Light.CAB)
    assert_true(entry["light_fixed_fields"]["light_enabled"])


func test_instrument_light_glows_at_each_backlight_piece():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("i-instrumentlight")
    assert_eq(entry["widget_class"], CabinIndicator3D)
    assert_eq(entry["fixed_fields"]["cab_light"], CabinState.Light.INSTRUMENT)
    assert_eq(entry["island_lights"], MmdSemanticCatalog.IslandLights.GLOW)


func test_alerter_lights_each_of_its_lamps():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("i-security_aware")
    assert_eq(entry["island_lights"], MmdSemanticCatalog.IslandLights.WIDGET_LIGHT)


func test_front_and_rear_light_indicators_bind_to_the_correct_ilights_bit():
    var expected:Dictionary = {
        "i-upperlight": "lights/front_headlight_upper_enabled",
        "i-leftlight": "lights/front_headlight_left_enabled",
        "i-rightlight": "lights/front_headlight_right_enabled",
        "i-leftend": "lights/front_redmarker_left_enabled",
        "i-rightend": "lights/front_redmarker_right_enabled",
        "i-rearupperlight": "lights/rear_headlight_upper_enabled",
        "i-rearleftlight": "lights/rear_headlight_left_enabled",
        "i-rearrightlight": "lights/rear_headlight_right_enabled",
        "i-rearleftend": "lights/rear_redmarker_left_enabled",
        "i-rearrightend": "lights/rear_redmarker_right_enabled",
    }
    for label:String in expected:
        var entry:Dictionary = MmdSemanticCatalog.get_entry(label)
        assert_eq(entry["widget_class"], CabinSpotLight3D, label)
        assert_eq(entry["fixed_fields"]["state_property"], expected[label], label)
        assert_true(entry["position_at_submodel"], label)
        assert_false(entry["fixed_fields"].has("blink_time"), "%s is steady on/off, not blinking" % label)


func test_i_security_cabsignal_binds_cabsignal_blinking():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("i-security_cabsignal")
    assert_eq(entry["widget_class"], CabinSpotLight3D)
    assert_eq(entry["fixed_fields"]["state_property"], "cabsignal_blinking")
    assert_eq(entry["fixed_fields"]["blink_time"], 0.2)
    assert_false(entry["fixed_fields"].has("light_enabled"))


func test_i_security_aware_indicator_opts_into_an_integrated_light():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("i-security_aware")
    assert_true(entry["fixed_fields"]["light_enabled"])


func test_cabin_spot_light_3d_defaults_light_enabled_to_false():
    var widget := CabinSpotLight3D.new()
    add_child_autofree(widget)
    assert_false(widget.light_enabled)


func test_cab_light_switches_are_the_cabs_own():
    for label:String in ["cablight_sw", "cablightdim_sw", "instrumentlight_sw"]:
        var entry:Dictionary = MmdSemanticCatalog.get_entry(label)
        assert_eq(entry["widget_class"], CabinButton, label)
        assert_false(entry["fixed_fields"].has("command"), "%s sends no vehicle command" % label)
        assert_false(entry["fixed_fields"].has("state_property"), "%s shows what its cab holds" % label)
    assert_eq(MmdSemanticCatalog.get_entry("cablightdim_sw")["fixed_fields"]["action"], "cabin_light_dim_toggle")


func test_every_speedometer_of_the_original_binds_its_speed():
    # Train.cpp:12101-12134
    assert_eq(MmdSemanticCatalog.get_entry("tachometerb")["fixed_fields"]["state_property"], "tachometer_speed_jump")
    assert_eq(MmdSemanticCatalog.get_entry("tachometern")["fixed_fields"]["state_property"], "tachometer_speed")
    assert_eq(MmdSemanticCatalog.get_entry("tachometerd")["fixed_fields"]["state_property"], "tachometer_speed")


func test_brake_operation_mode_switch_shows_the_mode_position():
    var entry:Dictionary = MmdSemanticCatalog.get_entry("brakeopmode_sw")
    assert_eq(entry["widget_class"], CabinSwitch)
    assert_eq(entry["fixed_fields"]["state_property"], "brake_operation_mode_position")
    assert_eq(entry["config_max_property"], "brake_operation_mode_position_max")


func test_speed_control_buttons_are_lit_while_it_is_active():
    for label:String in ["speedinc_bt", "speeddec_bt", "speedctrlpowerinc_bt", "speedctrlpowerdec_bt", "speedbutton0", "speedbutton9"]:
        assert_eq(MmdSemanticCatalog.get_entry(label)["state_light"]["state_property"], "speed_control/active", label)
    assert_eq(MmdSemanticCatalog.get_entry("speedbutton9")["fixed_fields"]["command_param"], 9)


# drivermode.cpp:352 - a control the hand reaches says what it is; the original misses some
# (cabactivation_sw, mirrors_sw, radiocall1_sw), MmdCabControlCaptions has them all
func test_every_operable_control_has_a_caption():
    var uncaptioned:Array[String] = []
    for label:String in MmdSemanticCatalog.get_labels():
        var widget_class:Variant = MmdSemanticCatalog.get_entry(label)["widget_class"]
        if (widget_class == CabinButton or widget_class == CabinSwitch or widget_class == CabinKnob) \
                and not MmdCabControlCaptions.caption(StringName(label)):
            uncaptioned.append(label)
    assert_eq(uncaptioned, [] as Array[String])
