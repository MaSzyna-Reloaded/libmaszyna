extends MaszynaGutTest

## MaszynaSceneryInfo reads the trainsets the scenario selector arranges. A vehicle's name is read in
## lower case, as the scenery's importer and the original read it (simulationstateserializer.cpp:
## 425-430): the arranged trainset and the player's vehicle are found by it - "EN57-636ra" started
## the player outside

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_a_vehicles_name_is_read_in_lower_case() -> void:
    var info:MaszynaSceneryInfo = MaszynaSceneryInfo.read("startup_en57-636ra.scn")
    assert_eq(info.trainsets.size(), 1, "the scenery's trainset")
    if not info.trainsets:
        return
    var trainset:MaszynaSceneryInfo.Trainset = info.trainsets[0]
    assert_eq(trainset.vehicles[0].train_id, "en57-636ra", "EN57-636ra as the importer names it")
    assert_eq(trainset.get_driver_train_id(), "en57-636ra", "the player's vehicle by that name")
