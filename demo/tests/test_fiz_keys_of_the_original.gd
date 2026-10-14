extends MaszynaGutTest

## FIZ sections read under the keys and with the defaults of the original's loaders - the keys of
## the game data, not guessed spellings: SpeedControl: (LoadFIZ_SpeedControl, Mover.cpp:11093-11130),
## Security: (TSecuritySystem::load, Mover.cpp:235-254), Load: (LoadFIZ_Load, Mover.cpp:10309-10331),
## Switches:/DimmerList: (LoadFIZ_Switches, Mover.cpp:11384-11403, 11537-11542).


func _parse(parser_script:Variant, line:String, prefix:String, part:String) -> Variant:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(line.to_utf8_buffer())
    var context:FizImportContext = FizImportContext.new()
    parser_script.new().parse(parser, context, prefix)
    return context.get_part(part)


func test_speed_control_keys() -> void:
    var control:RailVehicleSpeedControl = _parse(FizTrainSpeedControlParser,
            "SpeedCtrl=Yes SpeedCtrlDelay=3 SpeedCtrlType=Time SpeedButtons=40|80 BrakeIntMaxVel=20 PowerUpSpeed=0.5 PowerDownSpeed=0.7",
            "SpeedControl:", "RailVehicleSpeedControl")
    assert_eq(control.delay, 3.0)
    assert_true(control.impulse_lever, "SpeedCtrlType=Time")
    assert_eq(control.preset_speeds, PackedFloat64Array([40.0, 80.0]))
    assert_eq(control.brake_intervention_max_velocity, 20.0)
    assert_eq(control.power_up_speed, 0.5)
    assert_eq(control.power_down_speed, 0.7)


func test_speed_control_defaults() -> void:
    var control:RailVehicleSpeedControl = _parse(FizTrainSpeedControlParser, "SpeedCtrl=Yes", "SpeedControl:",
            "RailVehicleSpeedControl")
    assert_eq(control.delay, 2.0, "SpeedCtrlDelay (MOVER.h:1934)")
    assert_eq(control.preset_speeds.size(), 10, "SpeedCtrlButtons (MOVER.h:1933)")
    assert_true(control.override_manual_power, "ManualStateOverride (MOVER.h:1261)")


func test_security_keys_and_defaults() -> void:
    var security:RailVehicleSecuritySystem = _parse(FizTrainSecuritySystemParser,
            "AwareSystem=Active,CabSignal MagnetLocation=4.5 AwareMinSpeed=12 CabDependent=Yes",
            "Security:", "RailVehicleSecuritySystem")
    assert_eq(security.shp_magnet_distance, 4.5, "MagnetLocation")
    assert_eq(security.aware_min_speed, 12.0)
    assert_true(security.cab_dependent)
    assert_eq(security.aware_delay, 30.0, "AwareDelay absent (MOVER.h:1147)")
    assert_eq(security.sound_signal_delay, 5.0)
    assert_eq(security.emergency_brake_delay, 5.0)
    assert_eq(security.ca_max_hold_time, 1.5)


func test_load_minimum_offsets() -> void:
    var load:RailVehicleLoad = _parse(FizTrainLoadParser,
            "MaxLoad=60 LoadQ=tonns LoadAccepted=Coal,Ore LoadMinOffset=-1.5,-0.3", "Load:", "RailVehicleLoad")
    assert_eq(load.minimum_load_offsets.size(), 2)
    assert_almost_eq(float(load.minimum_load_offsets[0]), -1.5, 0.001)


func test_relay_reset_buttons() -> void:
    var switches:RailVehicleSwitches = _parse(FizTrainSwitchesParser, "RelayResetButton1=3 RelayResetButton2=4",
            "Switches:", "RailVehicleSwitches")
    assert_eq(switches.relay_reset_button_1, 3)
    assert_eq(switches.relay_reset_button_2, 4)


# LoadFIZ_TurboPos (Mover.cpp:10714) - the diesel engine's turbocharger position (SP42: TurboPos=5)
func test_turbo_position_reaches_the_diesel_engine() -> void:
    var description:VehicleController = FizVehicleBuilder.build_description_at(
            "res://tests/fixtures/dynamic/pkp/sp42_v1/101d.fiz")
    var engine:RailVehicleDieselEngine = null
    for component:VehicleComponent in description.get_components():
        if component is RailVehicleDieselEngine:
            engine = component
    assert_not_null(engine)
    assert_eq(engine.turbo_position, 5)
