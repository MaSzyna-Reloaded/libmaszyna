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
    assert_eq(trainset.members[0].vehicle.vehicle_id, "en57-636ra", "EN57-636ra as the importer names it")
    assert_eq(trainset.get_driver_vehicle().vehicle_id, "en57-636ra", "the player's vehicle by that name")
    assert_eq(trainset.members[0].vehicle.name, "EN57-636ra", "shown as the .scn writes it")


func test_a_vehicle_knows_its_place_in_the_declared_trainset() -> void:
    var info:MaszynaSceneryInfo = MaszynaSceneryInfo.read("trainsets_repeated_ids.scn")
    assert_eq(info.trainsets.size(), 1, "the scenery's trainset")
    if not info.trainsets:
        return
    var places:Array[int] = []
    for member:TrainsetMemberData in info.trainsets[0].members:
        places.append(member.trainset_index)
    assert_eq(places, [0, 1, 2, 3], "front first - the wagons share one id")


## The scenery selector shows and loads a trainset's couplings as the .scn declares them: an EN57's
## cars for good (couplingdata 255: every flag, permanent), its last car to nothing (0)
func test_a_vehicle_couples_as_the_scenery_declares_it() -> void:
    var info:MaszynaSceneryInfo = MaszynaSceneryInfo.read("startup_en57-636ra.scn")
    assert_eq(info.trainsets.size(), 1, "the scenery's trainset")
    if not info.trainsets:
        return
    var modes:Array = info.trainsets[0].members.map(
            func(member:TrainsetMemberData) -> int: return member.coupling_mode)
    assert_eq(modes, [RailVehicleController.COUPLING_MODE_PERMANENT, RailVehicleController.COUPLING_MODE_PERMANENT,
            RailVehicleController.COUPLING_MODE_NONE])


## couplingdata 3 (the coupler and the brake hose) between the loco and its wagons, 0 behind the last
func test_a_freight_trainset_couples_as_the_scenery_declares_it() -> void:
    var info:MaszynaSceneryInfo = MaszynaSceneryInfo.read("trainsets_repeated_ids.scn")
    if not info.trainsets:
        fail_test("the scenery's trainset")
        return
    var modes:Array = info.trainsets[0].members.map(
            func(member:TrainsetMemberData) -> int: return member.coupling_mode)
    assert_eq(modes, [RailVehicleController.COUPLING_MODE_BRAKE, RailVehicleController.COUPLING_MODE_BRAKE,
            RailVehicleController.COUPLING_MODE_BRAKE, RailVehicleController.COUPLING_MODE_NONE])
