extends MaszynaGutTest

## FIZ keys of the brake and the doors read as LoadFIZ_Brake/LoadFIZ_Cntrl/LoadFIZ_Doors read them
## (Mover.cpp:10469-10528, 10720-10895, LoadFIZ_Doors), with their defaults when absent.

## The Sgs wagon of reports#13 (`l053_poludnie.scn`, `31514541864-8`): its `Brake:` line gives no
## `BCM`, only `BCMlo=3.43 BCMHi=12.00` - the gear ratio is the load's, chosen by
## CheckLocomotiveParameters() (Mover.cpp:11918-11931)
const SGS_WAGON_PATH:String = "res://tests/fixtures/dynamic/pkp/sgs_v2/412z.fiz"
## What the report's wagons carried, as the scenery names it - 40 t of the wagon's 57 is over
## 0.55 of it, so the original sets the loaded ratio (BCMHi)
const SGS_WAGON_LOAD_NAME:String = "c40z20"
const SGS_WAGON_LOAD_AMOUNT:float = 40.0
## Steps a vehicle just built takes to be configured
const SETTLE_TICKS:int = 2


func _key_values(line:String) -> Dictionary:
    var parser:MaszynaParser = MaszynaParser.new()
    parser.initialize(line.to_utf8_buffer())
    return FizLineUtil.read_key_values(parser)


func _cntrl(line:String) -> RailVehicleBrake:
    var brake:RailVehicleBrake = MaszynaMoverRailVehicleBrake.new()
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
            MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_NO_HANDLE)
    assert_eq(_cntrl("BrakeSystem=Pneumatic LocBrakeHandle=Westinghouse").cntrl_local_brake_handle_type,
            MaszynaMoverRailVehicleBrake.BRAKE_HANDLE_TYPE_WESTINGHOUSE)


# Mover.cpp:10528 - a diesel's releaser works only at zero unless its FIZ says otherwise
func test_releaser_power_position_lock_defaults_by_the_engine() -> void:
    assert_true(_brake("BrakeValve=LSt MBF=85", RailVehicleEngine.DIESEL_ELECTRIC).releaser_enabled_only_at_no_power_pos)
    assert_false(_brake("BrakeValve=LSt MBF=85", RailVehicleEngine.ELECTRIC_SERIES_MOTOR).releaser_enabled_only_at_no_power_pos)
    assert_false(_brake("BrakeValve=LSt ReleaserPowerPosLock=No", RailVehicleEngine.DIESEL).releaser_enabled_only_at_no_power_pos)


# Mover.cpp:10469 - BM absent or unknown is 0
func test_brake_method_without_bm_is_none() -> void:
    assert_eq(_brake("BrakeValve=LSt MBF=85", RailVehicleEngine.NONE).brake_method, MaszynaMoverRailVehicleBrake.BRAKE_METHOD_NONE)
    assert_eq(_brake("BrakeValve=LSt BM=Cosid", RailVehicleEngine.NONE).brake_method, MaszynaMoverRailVehicleBrake.BRAKE_METHOD_COSID)


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


# Mover.cpp, BM - Frenoplast FR510 is a brake method of its own
func test_brake_method_fr510() -> void:
    assert_eq(_brake("BrakeValve=ESt3 BM=FR510", RailVehicleEngine.NONE).brake_method,
            MaszynaMoverRailVehicleBrake.BRAKE_METHOD_FR510)


# utilities.h:151 - extract_value() skips the spaces after the "=", the next token is the value
func test_value_after_a_space_is_read() -> void:
    var kv:Dictionary = _key_values("NBpA= 2 BCN=4 BCM= 6.5")
    assert_eq(kv["NBpA"], "2")
    assert_eq(kv["BCN"], "4")
    assert_eq(kv["BCM"], "6.5")


# LoadFIZ() calls LoadFIZ_Brake() for every Brake: line, and a key absent from a line keeps the
# value of an earlier one
func test_brake_lines_add_up() -> void:
    var context:FizImportContext = FizImportContext.new()
    var parser:FizTrainBrakeParser = FizTrainBrakeParser.new()
    for line:String in ["BrakeValve=ESt3 NBpA=2 MaxBP=3.8 BCN=1 BCM=6.5", "BCR=0.2 BVV=200"]:
        var line_parser:MaszynaParser = MaszynaParser.new()
        line_parser.initialize(line.to_utf8_buffer())
        parser.parse(line_parser, context, "Brake:")
    var brake:MaszynaMoverRailVehicleBrake = context.get_part("RailVehicleBrake")
    assert_eq(brake.valve_type, MaszynaMoverRailVehicleBrake.BRAKE_VALVE_EST3)
    assert_eq(brake.friction_elements_per_axle, 2)
    assert_eq(brake.cylinder_count, 1)
    assert_eq(brake.cylinder_gear_ratio, 6.5)
    assert_eq(brake.cylinder_radius, 0.2)


## The brake of the Sgs wagon as the scenery places it, with `load_amount` of the report's cargo -
## the load is the node's, taken before the controller starts, as the scenery's (the vehicle
## system's load_set() before vehicle_bind_controller())
func _sgs_wagon_brake(vehicle_id:String, load_amount:float) -> RailVehicleBrake:
    var node:RailVehiclePhysicsNode = RailVehiclePhysicsNode.new()
    node.controller = MaszynaFizManager.load_description_at(SGS_WAGON_PATH)
    node.vehicle_id = vehicle_id
    if load_amount > 0.0:
        node.load_name = SGS_WAGON_LOAD_NAME
        node.load_amount = load_amount
    add_child_autofree(node)
    await step(SETTLE_TICKS)
    var controller:VehicleController = VehicleServer.vehicle_get_controller(node.get_vehicle_rid())
    return controller.get_rail_component(RailVehicleComponentType.COMPONENT_BRAKES) as RailVehicleBrake


# reports#13 - the wagons had full cylinders and no brake force: BrakeForceP() (Mover.cpp:5145)
# multiplies by BrakeCylMult[0], which a FIZ without BCM leaves at 0 until the load picks it
func test_a_loaded_wagon_without_bcm_brakes_by_its_loaded_ratio() -> void:
    var loaded:RailVehicleBrake = await _sgs_wagon_brake("SgsLoaded", SGS_WAGON_LOAD_AMOUNT)
    var empty:RailVehicleBrake = await _sgs_wagon_brake("SgsEmpty", 0.0)
    var loaded_force:float = loaded.get_force_at(1.0, 0.0)
    var empty_force:float = empty.get_force_at(1.0, 0.0)
    assert_gt(empty_force, 0.0, "an empty wagon brakes by BCMlo")
    assert_gt(loaded_force, empty_force, "a loaded wagon brakes harder, by BCMHi")
