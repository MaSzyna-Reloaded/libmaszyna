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
    importer = load("res://addons/libmaszyna/legacy/scenery/maszyna_node_dynamic_importer.gd").new()
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    var fixture_dir:String = TEST_GAME_DIR.path_join("dynamic/fixtures")
    DirAccess.make_dir_recursive_absolute(fixture_dir)
    FileAccess.open(fixture_dir.path_join("MixedVehicle.fiz"), FileAccess.WRITE).close()
    UserSettings.save_maszyna_game_dir(TEST_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func _trainset_context(offset:float) -> MaszynaImporterContext:
    var context := MaszynaImporterContext.new()
    var parser := MaszynaParser.new()
    parser.initialize(("trainset start %s 0" % offset).to_utf8_buffer(), [])
    load("res://addons/libmaszyna/legacy/scenery/maszyna_trainset_importer.gd").new().import(parser, context)
    return context


func _import(context:MaszynaImporterContext, text:String) -> MaszynaDynamicData:
    var parser := MaszynaParser.new()
    parser.initialize(text.to_utf8_buffer(), [])
    return importer.import(parser, context)


func test_offset_minus_one_sentinel_imports_as_reversed() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var dynamic:MaszynaDynamicData = _import(context, "fixtures skin short -1.0 headdriver 99 0 enddynamic")
    assert_eq(dynamic.direction, TrackServer.DIRECTION_REVERSED)
    assert_eq(dynamic.gap, 0.0, "a reversed vehicle stands right behind")


func test_normal_offset_imports_as_normal_direction() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var dynamic:MaszynaDynamicData = _import(context, "fixtures skin short 2.5 headdriver 99 0 enddynamic")
    assert_eq(dynamic.direction, TrackServer.DIRECTION_NORMAL)
    assert_eq(dynamic.gap, 2.5, "its offset is its gap in the trainset")


func test_the_trainset_places_its_vehicles_not_the_vehicles_themselves() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var first:MaszynaDynamicData = _import(context, "fixtures skin short 0 headdriver 3 0 enddynamic")
    var second:MaszynaDynamicData = _import(context, "fixtures skin long 0 nobody 3 0 enddynamic")

    assert_eq(context.trainsets.size(), 1, "the vehicles of a trainset are no trainsets of their own")
    var trainset:MaszynaTrainsetData = context.trainsets[0]
    assert_eq(trainset.track_name, "start")
    assert_almost_eq(trainset.offset, 20.0, 0.001, "the trainset's front is the offset of trainset:")
    assert_eq(trainset.dynamics, [first, second] as Array[MaszynaDynamicData])
    assert_eq([first.coupling, second.coupling], [3, 3], "the couplingdata of every vehicle")


## Its front at -offset: a trainset from an offset of 0, the vehicle's offset its gap - what
## RailVehicleServer.trainset_place() stands with its centre half its length behind
func test_a_vehicle_outside_a_trainset_is_a_trainset_of_its_own() -> void:
    var context := MaszynaImporterContext.new()
    var dynamic:MaszynaDynamicData = _import(context, "fixtures skin short start -20.0 headdriver 12.5 0 enddynamic")

    assert_eq(context.trainsets.size(), 1)
    var trainset:MaszynaTrainsetData = context.trainsets[0]
    assert_eq(trainset.track_name, "start")
    assert_eq(trainset.offset, 0.0)
    assert_eq(trainset.timetable, "", "no timetable: nothing is sent to its driver")
    assert_eq(trainset.dynamics, [dynamic] as Array[MaszynaDynamicData])
    assert_almost_eq(dynamic.gap, -20.0, 0.001)
    assert_almost_eq(dynamic.velocity, 12.5, 0.001, "its own velocity")


func test_vehicle_file_and_skin_keep_the_spelling_from_the_scenery() -> void:
    var context := MaszynaImporterContext.new()
    var dynamic:MaszynaDynamicData = _import(
        context, "FIXTURES MixedSkin MixedVehicle start -20.0 headdriver 0 0 enddynamic"
    )
    assert_eq(dynamic.data_path, "dynamic/fixtures", "the missing mixed-case directory falls back")
    assert_eq(dynamic.file_name, "MixedVehicle", "the existing exact filename is not lowercased")
    assert_eq(dynamic.skin, "MixedSkin", "the skin keeps its authored spelling")



## DynObj.cpp:1812-1825 - the driver type picks the cabin the driver sits in.
func test_driver_type_selects_driver_cabin() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var head:MaszynaDynamicData = _import(context, "fixtures skin short 0 headdriver 3 0 enddynamic")
    var rear:MaszynaDynamicData = _import(context, "fixtures skin short 0 reardriver 3 0 enddynamic")
    var nobody:MaszynaDynamicData = _import(context, "fixtures skin short 0 nobody 3 0 enddynamic")

    assert_eq(head.driver_type, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    assert_eq(rear.driver_type, MaszynaDynamicData.DriverType.DRIVER_REAR)
    assert_eq(nobody.driver_type, MaszynaDynamicData.DriverType.DRIVER_NOBODY)


## What the scenery loaded the vehicle with. The count comes first and the cargo's name only
## follows it when the count is not zero; a count with no name behind it is not a load at all
## (simulationstateserializer.cpp:1031), which is how a `dynamic` ending right there reads.
func test_the_load_a_dynamic_declares_reaches_the_vehicle() -> void:
    var context:MaszynaImporterContext = _trainset_context(20.0)
    var loaded:MaszynaDynamicData = _import(
            context, "fixtures skin short 0 nobody 3 24 coal enddynamic")
    var empty:MaszynaDynamicData = _import(context, "fixtures skin short 0 nobody 3 0 enddynamic")
    var unnamed:MaszynaDynamicData = _import(context, "fixtures skin short 0 nobody 3 24 enddynamic")

    assert_eq(loaded.load_name, "coal", "the cargo is named as the scenery names it")
    assert_almost_eq(loaded.load_amount, 24.0, 0.001, "and carried in the amount it declares")
    assert_eq(empty.load_name, "", "a count of zero carries nothing")
    assert_almost_eq(empty.load_amount, 0.0, 0.001)
    assert_eq(unnamed.load_name, "", "a count with no cargo named behind it is not a load")
    assert_almost_eq(unnamed.load_amount, 0.0, 0.001)
