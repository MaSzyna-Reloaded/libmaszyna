extends MaszynaGutTest

## Placement of .scn `dynamic` nodes, ported from deserialize_dynamic()
## (simulationstateserializer.cpp:960-988) and TDynamicObject::Init() (DynObj.cpp):
## - offset == -1.0 means "reversed in the trainset" (`Init(..., (offset == -1.0), ...)`,
##   DynObj.cpp:1807).
## - inside a trainset the vehicle stands where the trainset puts it: the trainset takes the
##   track and the offset of `trainset:`, and the vehicle's `offset` as its gap.
## - outside one it is a trainset of its own, from an offset of 0.

const TEST_GAME_DIR:String = "user://gut/dynamic_importer_fixture"

var importer:RefCounted
var _previous_game_dir:String


func before_each() -> void:
    importer = load("res://addons/libmaszyna/scenery/maszyna_node_dynamic_importer.gd").new()
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    var fixture_dir:String = TEST_GAME_DIR.path_join("dynamic/fixtures")
    DirAccess.make_dir_recursive_absolute(fixture_dir)
    FileAccess.open(fixture_dir.path_join("MixedVehicle.fiz"), FileAccess.WRITE).close()
    MaszynaGameDirectory.save_game_dir(TEST_GAME_DIR)


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _trainset_context(offset:float) -> MaszynaImporterContext:
    var context := MaszynaImporterContext.new()
    var parser := MaszynaParser.new()
    parser.initialize(("trainset start %s 0" % offset).to_utf8_buffer(), [])
    load("res://addons/libmaszyna/scenery/maszyna_trainset_importer.gd").new().import(parser, context)
    return context


func _import(context:MaszynaImporterContext, text:String) -> MaszynaVehicleData:
    var parser := MaszynaParser.new()
    parser.initialize(text.to_utf8_buffer(), [])
    return importer.import(parser, context, "Test")


## The trainset member the last imported vehicle is - how it stands in its trainset
func _member(context:MaszynaImporterContext) -> TrainsetMemberData:
    return context.trainsets.back().members.back()


func test_offset_minus_one_sentinel_imports_as_reversed() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    _import(context, "fixtures skin short -1.0 headdriver 99 0 enddynamic")
    assert_eq(_member(context).direction, TrackServer.DIRECTION_REVERSED)
    assert_eq(_member(context).gap, 0.0, "a reversed vehicle stands right behind")


func test_normal_offset_imports_as_normal_direction() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    _import(context, "fixtures skin short 2.5 headdriver 99 0 enddynamic")
    assert_eq(_member(context).direction, TrackServer.DIRECTION_NORMAL)
    assert_eq(_member(context).gap, 2.5, "its offset is its gap in the trainset")


func test_the_trainset_places_its_vehicles_not_the_vehicles_themselves() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var first:MaszynaVehicleData = _import(context, "fixtures skin short 0 headdriver 3 0 enddynamic")
    var second:MaszynaVehicleData = _import(context, "fixtures skin long 0 nobody 3 0 enddynamic")

    assert_eq(context.trainsets.size(), 1, "the vehicles of a trainset are no trainsets of their own")
    var trainset:TrainsetData = context.trainsets[0]
    assert_eq(trainset.track_name, "start")
    assert_almost_eq(trainset.offset, 20.0, 0.001, "the trainset's front is the offset of trainset:")
    assert_eq(trainset.members.map(func(member:TrainsetMemberData) -> VehicleData: return member.vehicle),
            [first, second])
    # couplingdata 3: the coupler and the brake hose
    assert_eq(trainset.members.map(func(member:TrainsetMemberData) -> int: return member.coupling_mode),
            [RailVehicleController.COUPLING_MODE_BRAKE, RailVehicleController.COUPLING_MODE_BRAKE],
            "the couplingdata of every vehicle")


## Its front at -offset: a trainset from an offset of 0, the vehicle's offset its gap - what
## RailVehicleServer.trainset_place() stands with its centre half its length behind
func test_a_vehicle_outside_a_trainset_is_a_trainset_of_its_own() -> void:
    var context := MaszynaImporterContext.new()
    var dynamic:MaszynaVehicleData = _import(context, "fixtures skin short start -20.0 headdriver 12.5 0 enddynamic")

    assert_eq(context.trainsets.size(), 1)
    var trainset:TrainsetData = context.trainsets[0]
    assert_eq(trainset.track_name, "start")
    assert_eq(trainset.offset, 0.0)
    assert_eq(trainset.timetable, "", "no timetable: nothing is sent to its driver")
    assert_eq(trainset.members.size(), 1)
    assert_eq(trainset.members[0].vehicle, dynamic)
    assert_almost_eq(trainset.members[0].gap, -20.0, 0.001)
    assert_almost_eq(trainset.velocity, 12.5, 0.001, "its own velocity is its trainset's")


func test_vehicle_file_and_skin_keep_the_spelling_from_the_scenery() -> void:
    var context := MaszynaImporterContext.new()
    var dynamic:MaszynaVehicleData = _import(
        context, "FIXTURES MixedSkin MixedVehicle start -20.0 headdriver 0 0 enddynamic"
    )
    assert_eq(dynamic.data_path, "dynamic/fixtures", "the missing mixed-case directory falls back")
    assert_eq(dynamic.file_name, "MixedVehicle", "the existing exact filename is not lowercased")
    assert_eq(dynamic.skin, "MixedSkin", "the skin keeps its authored spelling")



## DynObj.cpp:1812-1825 - the driver type picks the cabin the driver sits in.
func test_driver_type_selects_driver_cabin() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var head:MaszynaVehicleData = _import(context, "fixtures skin short 0 headdriver 3 0 enddynamic")
    var rear:MaszynaVehicleData = _import(context, "fixtures skin short 0 reardriver 3 0 enddynamic")
    var nobody:MaszynaVehicleData = _import(context, "fixtures skin short 0 nobody 3 0 enddynamic")

    assert_eq(head.driver_type, MaszynaVehicleData.DriverType.DRIVER_HEAD)
    assert_eq(rear.driver_type, MaszynaVehicleData.DriverType.DRIVER_REAR)
    assert_eq(nobody.driver_type, MaszynaVehicleData.DriverType.DRIVER_NOBODY)


## What the scenery loaded the vehicle with. The count comes first and the cargo's name only
## follows it when the count is not zero; a count with no name behind it is not a load at all
## (simulationstateserializer.cpp:1031), which is how a `dynamic` ending right there reads.
func test_the_load_a_dynamic_declares_reaches_the_vehicle() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var loaded:MaszynaVehicleData = _import(
            context, "fixtures skin short 0 nobody 3 24 coal enddynamic")
    var empty:MaszynaVehicleData = _import(context, "fixtures skin short 0 nobody 3 0 enddynamic")
    var unnamed:MaszynaVehicleData = _import(context, "fixtures skin short 0 nobody 3 24 enddynamic")

    assert_eq(loaded.load_name, "coal", "the cargo is named as the scenery names it")
    assert_almost_eq(loaded.load_amount, 24.0, 0.001, "and carried in the amount it declares")
    assert_eq(empty.load_name, "", "a count of zero carries nothing")
    assert_almost_eq(empty.load_amount, 0.0, 0.001)
    assert_eq(unnamed.load_name, "", "a count with no cargo named behind it is not a load")
    assert_almost_eq(unnamed.load_amount, 0.0, 0.001)


## The track a trainset and a lone vehicle stand on is named in lower case, as the track itself is
## (simulationstateserializer.cpp:425-430, 777-782, 993-995) - "Tor_A" would find no track
func test_the_track_named_with_capitals_is_read_in_lower_case() -> void:
    var context := MaszynaImporterContext.new()
    var parser := MaszynaParser.new()
    parser.initialize("Pociag_1 Tor_A 20 0".to_utf8_buffer(), [])
    load("res://addons/libmaszyna/scenery/maszyna_trainset_importer.gd").new().import(parser, context)
    assert_eq(context.trainset.track_name, "tor_a", "the trainset's track")
    assert_eq(context.trainset.name, "pociag_1", "the trainset's name")
    var lone := MaszynaImporterContext.new()
    _import(lone, "fixtures skin short Tor_B 0 headdriver 0 0 enddynamic")
    assert_eq(lone.trainsets[0].track_name, "tor_b", "a lone vehicle's track")


## A couplingdata of the flags, as a .scn writes it: the Mover's coupling:: bits
func _couplingdata(flags:int) -> String:
    return str(MaszynaMoverCoupling.flags_to_mover(flags))


func _imported_mode(couplingdata:String, offset:String = "0") -> RailVehicleController.CouplingMode:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    _import(context, "fixtures skin short %s nobody %s 0 enddynamic" % [offset, couplingdata])
    return _member(context).coupling_mode


## MASZYNA_ORIGINAL_QUIRKS.md, "A trainset's coupling is the Mover's raw bits": the mask of a
## couplingdata becomes one of the few ways a trainset couples
func test_a_couplingdata_becomes_its_coupling_mode() -> void:
    var coupler:int = RailVehicleController.COUPLING_FLAG_COUPLER
    var brake:int = coupler | RailVehicleController.COUPLING_FLAG_BRAKEHOSE
    var passenger:int = (brake | RailVehicleController.COUPLING_FLAG_GANGWAY
            | RailVehicleController.COUPLING_FLAG_MAINHOSE | RailVehicleController.COUPLING_FLAG_HEATING)
    var unit:int = passenger | RailVehicleController.COUPLING_FLAG_PERMANENT
    assert_eq(_imported_mode("0"), RailVehicleController.COUPLING_MODE_NONE, "no flags")
    assert_eq(_imported_mode(_couplingdata(coupler)), RailVehicleController.COUPLING_MODE_COUPLER,
            "the coupler alone")
    assert_eq(_imported_mode(_couplingdata(brake)), RailVehicleController.COUPLING_MODE_BRAKE,
            "the coupler and the brake hose")
    assert_eq(_imported_mode(_couplingdata(passenger)), RailVehicleController.COUPLING_MODE_FULL,
            "the coupler with anything else")
    assert_eq(_imported_mode(_couplingdata(unit)), RailVehicleController.COUPLING_MODE_PERMANENT,
            "with the permanent flag")
    assert_eq(_imported_mode("-" + _couplingdata(passenger)), RailVehicleController.COUPLING_MODE_PERMANENT,
            "a negative one")


func test_a_couplingdata_without_the_coupler_couples_nothing() -> void:
    var hoses:int = RailVehicleController.COUPLING_FLAG_BRAKEHOSE | RailVehicleController.COUPLING_FLAG_HIGHVOLTAGE
    assert_eq(_imported_mode("-" + _couplingdata(hoses)), RailVehicleController.COUPLING_MODE_NONE,
            "SM03-268ad's -10: hoses with no coupler to hold them")


func test_bits_with_no_coupling_flag_are_left_out() -> void:
    # a bit beyond the last of the original's enum coupling (power3x400v, MOVER.h:174)
    const UNKNOWN_BIT:int = 1 << 12
    var brake:int = RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_BRAKEHOSE
    var couplingdata:String = str(int(_couplingdata(brake)) | UNKNOWN_BIT)
    assert_eq(_imported_mode(couplingdata), RailVehicleController.COUPLING_MODE_BRAKE)


func test_a_vehicle_too_far_behind_is_not_coupled() -> void:
    var brake:int = RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_BRAKEHOSE
    assert_eq(_imported_mode(_couplingdata(brake), "1.0"), RailVehicleController.COUPLING_MODE_NONE,
            "more than 0.5 m behind (simulationstateserializer.cpp:1028)")


## Every coupling flag of the core has one bit of the Mover's, and no bit two flags
func test_every_coupling_flag_has_its_own_mover_bit() -> void:
    var seen:int = 0
    for flag_name:String in ClassDB.class_get_enum_constants("RailVehicleController", "CouplingFlags"):
        var flag:int = ClassDB.class_get_integer_constant("RailVehicleController", flag_name)
        if not flag:
            continue
        var bit:int = MaszynaMoverCoupling.flags_to_mover(flag)
        assert_gt(bit, 0, "%s is a Mover bit" % flag_name)
        assert_eq(bit & (bit - 1), 0, "%s is one bit" % flag_name)
        assert_eq(seen & bit, 0, "%s is no other flag's bit" % flag_name)
        seen |= bit
        assert_eq(MaszynaMoverCoupling.flags_from_mover(bit), flag, "%s read back" % flag_name)
