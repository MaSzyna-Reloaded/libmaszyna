extends MaszynaGutTest

## FIZ keys of the brake and the doors read as LoadFIZ_Brake/LoadFIZ_Cntrl/LoadFIZ_Doors read them
## (Mover.cpp:10469-10528, 10720-10895, LoadFIZ_Doors), with their defaults when absent.


func _key_values(line:String) -> Dictionary:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(line.to_utf8_buffer())
    return FizLineUtil.read_key_values(parser)


func _cntrl(line:String) -> RailVehicleBrake:
    var brake:RailVehicleBrake = MoverRailVehicleBrake.new()
    FizTrainBrakeParser.new().apply_cntrl(_key_values(line), brake, FizImportContext.new())
    return brake


func _brake(line:String, engine_type:int) -> RailVehicleBrake:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(line.to_utf8_buffer())
    var context:FizImportContext = FizImportContext.new()
    context.engine_type = engine_type
    FizTrainBrakeParser.new().parse(parser, context, "Brake:")
    return context.get_part("RailVehicleBrake")


# Mover.cpp:10817-10895 - the local, manual and spring brake keys whatever the brake system
func test_individual_brake_reads_its_local_brake() -> void:
    var brake:RailVehicleBrake = _cntrl("LocalBrake=PneumaticBrake ManualBrake=Yes SpringBrakeCutsOffDrive=No")
    assert_eq(brake.cntrl_brake_system, RailVehicleBrake.BRAKE_SYSTEM_INDIVIDUAL)
    assert_eq(brake.cntrl_local_brake_type, RailVehicleBrake.LOCAL_BRAKE_TYPE_PNEUMATIC)
    assert_true(brake.cntrl_manual_brake_present)
    assert_false(brake.cntrl_spring_brake_cuts_off_drive)


# Mover.cpp:10805 - "yes" is a kind of its own, ASBType 128
func test_asb_yes() -> void:
    assert_eq(_cntrl("BrakeSystem=Pneumatic BCPN=6 ASB=Yes").cntrl_anti_skid_brake_type,
            RailVehicleBrake.ANTI_SKID_BRAKE_YES)


# Mover.cpp:10778 - the local handle is FD1, Knorr or Westinghouse, nothing else
func test_local_brake_handle_takes_only_local_handles() -> void:
    assert_eq(_cntrl("BrakeSystem=Pneumatic LocBrakeHandle=FV4a").cntrl_local_brake_handle_type,
            RailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE)
    assert_eq(_cntrl("BrakeSystem=Pneumatic LocBrakeHandle=Westinghouse").cntrl_local_brake_handle_type,
            RailVehicleBrake.BRAKE_HANDLE_TYPE_WESTINGHOUSE)


# Mover.cpp:10528 - a diesel's releaser works only at zero unless its FIZ says otherwise
func test_releaser_power_position_lock_defaults_by_the_engine() -> void:
    assert_true(_brake("BrakeValve=LSt MBF=85", RailVehicleEngine.DIESEL_ELECTRIC).releaser_enabled_only_at_no_power_pos)
    assert_false(_brake("BrakeValve=LSt MBF=85", RailVehicleEngine.ELECTRIC_SERIES_MOTOR).releaser_enabled_only_at_no_power_pos)
    assert_false(_brake("BrakeValve=LSt ReleaserPowerPosLock=No", RailVehicleEngine.DIESEL).releaser_enabled_only_at_no_power_pos)


# Mover.cpp:10469 - BM absent or unknown is 0
func test_brake_method_without_bm_is_none() -> void:
    assert_eq(_brake("BrakeValve=LSt MBF=85", RailVehicleEngine.NONE).brake_method, RailVehicleBrake.BRAKE_METHOD_NONE)
    assert_eq(_brake("BrakeValve=LSt BM=Cosid", RailVehicleEngine.NONE).brake_method, RailVehicleBrake.BRAKE_METHOD_COSID)


# LoadFIZ_Doors - the delays are DoorOpenDelay= and DoorCloseDelay=; no shift without its key
func test_door_delays_and_shift() -> void:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize("OpenCtrl=DriverCtrl DoorOpenDelay=0.5 DoorCloseDelay=1.5".to_utf8_buffer())
    var context:FizImportContext = FizImportContext.new()
    FizTrainDoorsParser.new().parse(parser, context, "Doors:")
    var doors:RailVehicleDoors = context.get_part("RailVehicleDoors")
    assert_eq(doors.open_delay, 0.5)
    assert_eq(doors.close_delay, 1.5)
    assert_eq(doors.max_shift, 0.0, "DoorMaxShiftL/R absent")
    assert_eq(doors.max_shift_plug, 0.0, "DoorMaxShiftPlug absent")
