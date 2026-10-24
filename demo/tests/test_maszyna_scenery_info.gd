extends MaszynaGutTest

## MaszynaSceneryInfo reads the trainsets the scenario selector arranges. A vehicle's name is read in
## lower case, as the scenery's importer and the original read it (simulationstateserializer.cpp:
## 425-430): the arranged trainset and the player's vehicle are found by it - "EN57-636ra" started
## the player outside

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

var _previous_game_dir:String = ""


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func test_a_vehicles_name_is_read_in_lower_case() -> void:
    var info:MaszynaSceneryInfo = MaszynaSceneryInfo.read("startup_en57-636ra.scn")
    assert_eq(info.trainsets.size(), 1, "the scenery's trainset")
    if not info.trainsets:
        return
    var trainset:TrainsetData = info.trainsets[0]
    assert_eq(trainset.vehicles[0].vehicle_id, "en57-636ra", "EN57-636ra as the importer names it")
    assert_eq(trainset.get_driver_vehicle().vehicle_id, "en57-636ra", "the player's vehicle by that name")
    assert_eq(trainset.vehicles[0].name, "EN57-636ra", "shown as the .scn writes it")


func test_a_vehicle_knows_its_place_in_the_declared_trainset() -> void:
    var info:MaszynaSceneryInfo = MaszynaSceneryInfo.read("trainsets_repeated_ids.scn")
    assert_eq(info.trainsets.size(), 1, "the scenery's trainset")
    if not info.trainsets:
        return
    var places:Array[int] = []
    for vehicle:VehicleData in info.trainsets[0].vehicles:
        places.append(vehicle.trainset_index)
    assert_eq(places, [0, 1, 2, 3], "front first - the wagons share one id")
