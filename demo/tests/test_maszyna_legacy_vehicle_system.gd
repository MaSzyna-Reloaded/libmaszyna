extends MaszynaGutTest

## MaszynaLegacyVehicleSystem: a MaSzyna vehicle built through the servers from what a `dynamic`
## says, held by its handle alone - no node of its own.

const TEST_GAME_DIR:String = "user://gut/vehicle_template"
const DATA_PATH:String = "fixtures"
const FILE_NAME:String = "test_vehicle"
## test_vehicle with a one-position master controller - a wagon (FizTrainCntrlParser)
const WAGON_FILE_NAME:String = "test_wagon"
const PLAYER_SCENE:PackedScene = preload("res://addons/libmaszyna/player/player.tscn")
## Long enough for TrainSoundSystem's sweep to find the vehicle within earshot and build its sound
const SOUND_TIMEOUT:float = 5.0
## Exterior and cabin - the fixture has no running sounds
const PLAYERS_PER_VEHICLE:int = 2

var _previous_game_dir:String
var _vehicles:Array[RID] = []


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    var fixture_dir:String = TEST_GAME_DIR.path_join(DATA_PATH)
    DirAccess.make_dir_recursive_absolute(fixture_dir)
    var mmd:FileAccess = FileAccess.open(fixture_dir.path_join(FILE_NAME + ".mmd"), FileAccess.WRITE)
    mmd.store_string("models: test_vehicle.t3d\n")
    mmd.close()
    var fiz:FileAccess = FileAccess.open(fixture_dir.path_join(FILE_NAME + ".fiz"), FileAccess.WRITE)
    fiz.store_string(FileAccess.get_file_as_string("res://tests/fixtures/test_vehicle.fiz"))
    fiz.close()
    var wagon_mmd:FileAccess = FileAccess.open(fixture_dir.path_join(WAGON_FILE_NAME + ".mmd"), FileAccess.WRITE)
    wagon_mmd.store_string("models: test_vehicle.t3d\n")
    wagon_mmd.close()
    var wagon_fiz:FileAccess = FileAccess.open(fixture_dir.path_join(WAGON_FILE_NAME + ".fiz"), FileAccess.WRITE)
    wagon_fiz.store_string(FileAccess.get_file_as_string("res://tests/fixtures/test_vehicle.fiz").replace("MCPN=11", "MCPN=1"))
    wagon_fiz.close()
    UserSettings.save_maszyna_game_dir(TEST_GAME_DIR)


func after_each() -> void:
    for vehicle:RID in _vehicles:
        MaszynaLegacyVehicleSystem.vehicle_free(vehicle)
    _vehicles.clear()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)
    await wait_idle_frames(2)


func _create(vehicle_name:String, file_name:String = FILE_NAME,
        driver:MaszynaDynamicData.DriverType = MaszynaDynamicData.DriverType.DRIVER_NOBODY) -> RID:
    var dynamic:MaszynaDynamicData = MaszynaDynamicData.new()
    dynamic.name = vehicle_name
    dynamic.data_path = DATA_PATH
    dynamic.file_name = file_name
    dynamic.driver_type = driver
    var vehicle:RID = MaszynaLegacyVehicleSystem.vehicle_create(dynamic, get_instance_id())
    _vehicles.append(vehicle)
    if not await wait_until(MaszynaLegacyVehicleSystem.vehicle_is_built.bind(vehicle), BUILD_TIMEOUT):
        fail_test("the vehicle was not built within %.0f s" % BUILD_TIMEOUT)
    return vehicle


func test_a_vehicle_is_built_by_its_handle_without_a_node() -> void:
    var vehicle:RID = await _create("system_test")

    assert_true(VehicleServer.vehicle_is_simulation_ready(vehicle), "it has its simulation")
    assert_eq(VehicleServer.vehicle_get_rid_by_name("system_test"), vehicle, "found by the scenery's name of it")
    assert_true(RailVehicleServer.vehicle_is_attached(vehicle), "it is a rail vehicle")
    assert_true(RailVehicleRenderingServer.vehicle_is_attached(vehicle), "it is drawn")
    assert_eq(find_children("", "RailVehicle3D", true, false).size(), 0, "no node stands for it")
    assert_eq(MaszynaLegacyVehicleSystem.vehicle_get_dynamic(vehicle).name, "system_test")


## A `dynamic` that names no file has nothing to be built from
func test_a_vehicle_without_a_file_is_built_without_a_simulation() -> void:
    var vehicle:RID = await _create("system_test_missing", "")

    assert_true(MaszynaLegacyVehicleSystem.vehicle_is_built(vehicle), "its turn came")
    assert_false(VehicleServer.vehicle_is_simulation_ready(vehicle))


## The fixture's MMD defines no cab: a locomotive's scenery driver gets the cabin of its end all
## the same (MASZYNA_ORIGINAL_QUIRKS.md, "Every vehicle has three cab positions")
func test_a_driver_of_an_end_without_a_cab_definition_is_seated_in_a_locomotive() -> void:
    var vehicle:RID = await _create("system_test_rear_driver", FILE_NAME, MaszynaDynamicData.DriverType.DRIVER_REAR)

    var cabin:RID = RailVehicleServer.vehicle_get_driver_cabin(vehicle)
    assert_true(cabin.is_valid(), "the driver sits in a cabin")
    assert_eq(RailVehicleServer.cabin_get_kind(cabin), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR, "of the rear end")
    assert_false(RailVehicleServer.vehicle_get_front_cabin(vehicle).is_valid(), "no other cabin is made up")
    assert_not_null(CabinSystem.vehicle_get_cab_logic(vehicle), "and drives through the cab logic")


func test_a_wagon_given_a_driver_has_no_cabin_for_it() -> void:
    var vehicle:RID = await _create("system_test_wagon_driver", WAGON_FILE_NAME, MaszynaDynamicData.DriverType.DRIVER_HEAD)

    assert_false(RailVehicleServer.vehicle_get_driver_cabin(vehicle).is_valid())
    assert_null(CabinSystem.vehicle_get_cab_logic(vehicle))


func test_a_freed_vehicle_is_gone_from_the_servers() -> void:
    var vehicle:RID = await _create("system_test_freed")
    _vehicles.erase(vehicle)

    MaszynaLegacyVehicleSystem.vehicle_free(vehicle)

    assert_false(MaszynaLegacyVehicleSystem.vehicle_exists(vehicle))
    assert_false(VehicleServer.vehicle_get_rid_by_name("system_test_freed").is_valid())
    assert_false(RailVehicleRenderingServer.vehicle_is_attached(vehicle), "drawn no more")


## Two vehicles of one type share what their MMD says, and still sound each on its own: a sound
## bank of its own, built once the listener is within earshot, gone with the vehicle
func test_vehicles_of_one_type_have_their_own_sound_built_within_earshot() -> void:
    var first:RID = await _create("system_test_0")
    var second:RID = await _create("system_test_1")
    assert_eq(_sound_players().size(), 0, "no sound is built before a listener is near")

    var player:MaszynaPlayer = PLAYER_SCENE.instantiate()
    add_child_autoqfree(player)
    await wait_until(
            func() -> bool: return _sound_players().size() == 2 * PLAYERS_PER_VEHICLE, SOUND_TIMEOUT)

    var players:Array[Node] = _sound_players()
    assert_eq(players.size(), 2 * PLAYERS_PER_VEHICLE, "each vehicle has its exterior and its cabin player")
    var banks:Array[SfxBank] = []
    for sound_player:Node in players:
        var bank:SfxBank = (sound_player as SfxPlayer3D).bank
        assert_false(banks.has(bank), "a bank shared by two players")
        banks.append(bank)

    _vehicles.erase(first)
    _vehicles.erase(second)
    MaszynaLegacyVehicleSystem.vehicle_free(first)
    MaszynaLegacyVehicleSystem.vehicle_free(second)
    await wait_idle_frames(2)
    assert_eq(_sound_players().size(), 0, "the sound goes with its vehicle")


## The sound players the system has built, for whatever vehicles
func _sound_players() -> Array[Node]:
    return MaszynaLegacyVehicleSystem.find_children("", "SfxPlayer3D", true, false)
