extends MaszynaGutTest

## A .scn's trainset comes with what its comments say of it: its mission (//$o), and whether the
## scenario offers it to the player - not one whose mission begins with "-"

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const SCENERY:String = "trainsets_missions.scn"

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.filename = SCENERY
    _scenery.autoload = false
    add_child(_scenery)


func after_each() -> void:
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _trainset(vehicle_id:String) -> RID:
    return RailVehicleServer.vehicle_get_trainset(VehicleServer.vehicle_get_rid_by_id(vehicle_id))


func test_a_trainset_has_its_mission_and_is_offered_to_the_player() -> void:
    await _scenery.load()

    var express:RID = _trainset("first")
    assert_eq(RailVehicleServer.trainset_get_description(express), "Take the express to the next station.\nMind the signals.")
    assert_true(RailVehicleServer.trainset_is_player_drivable(express))


func test_a_trainset_whose_mission_begins_with_a_dash_is_not_offered() -> void:
    await _scenery.load()

    assert_false(RailVehicleServer.trainset_is_player_drivable(_trainset("loco")))
