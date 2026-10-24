extends MaszynaGutTest

## A trainset is the trainset now: uncoupled, one becomes two - the part that stays keeps the RID -
## coupled, two become one and the other ends; a vehicle coupled to none is a trainset of its own

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## A trainset "freight" of `loco` (with a driver) and three wagons, coupled one to the next
const FREIGHT_SCENERY:String = "trainsets_repeated_ids.scn"
## `lone`, a dynamic outside any trainset
const LONE_SCENERY:String = "trainsets.scn"

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode


func before_each() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.autoload = false
    add_child(_scenery)


func after_each() -> void:
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)


func _load(filename:String) -> void:
    _scenery.filename = filename
    await _scenery.load()


func _vehicles(trainset:RID) -> Array[RID]:
    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(trainset))
    return vehicles


func test_a_placed_trainset_is_its_vehicles_coupled() -> void:
    await _load(FREIGHT_SCENERY)

    var trainset:RID = _scenery.get_trainsets()[0]
    var vehicles:Array[RID] = _vehicles(trainset)
    assert_eq(vehicles.size(), 4)
    for vehicle:RID in vehicles:
        assert_eq(RailVehicleServer.vehicle_get_trainset(vehicle), trainset)


func test_uncoupled_one_trainset_becomes_two_the_driven_part_keeping_it() -> void:
    await _load(FREIGHT_SCENERY)
    var trainset:RID = _scenery.get_trainsets()[0]
    var loco:RID = _vehicles(trainset)[0]
    var wagons:Array[RID] = _vehicles(trainset).slice(1)
    RailVehicleServer.trainset_set_player_drivable(trainset, false)
    watch_signals(RailVehicleServer)

    (VehicleServer.vehicle_get_controller(loco) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)

    assert_eq(_vehicles(trainset), [loco] as Array[RID], "the loco with its driver keeps the trainset")
    var split:RID = RailVehicleServer.vehicle_get_trainset(wagons[0])
    assert_ne(split, trainset)
    assert_eq(_vehicles(split), wagons, "the wagons are a trainset of their own")
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_split", [trainset, split])
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_created", [split])
    assert_false(RailVehicleServer.trainset_is_player_drivable(split), "not offered, as the trainset it split off")


func test_coupled_two_trainsets_become_one_the_other_ending() -> void:
    await _load(FREIGHT_SCENERY)
    var trainset:RID = _scenery.get_trainsets()[0]
    var loco:RID = _vehicles(trainset)[0]
    var wagons:Array[RID] = _vehicles(trainset).slice(1)
    (VehicleServer.vehicle_get_controller(loco) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)
    var split:RID = RailVehicleServer.vehicle_get_trainset(wagons[0])
    watch_signals(RailVehicleServer)

    RailVehicleServer.vehicle_couple(loco, RailVehicleController.COUPLER_END_REAR, wagons[0],
            RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)

    assert_eq(_vehicles(trainset).size(), 4, "the loco's trainset takes the wagons back")
    assert_eq(RailVehicleServer.vehicle_get_trainset(wagons[2]), trainset)
    assert_false(split in RailVehicleServer.trainset_list(), "the wagons' trainset ended")
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_merged", [trainset, split])
    assert_signal_emitted_with_parameters(RailVehicleServer, "trainset_freed", [split])


func test_a_vehicle_coupled_to_none_is_a_trainset_of_its_own() -> void:
    await _load(LONE_SCENERY)

    var lone:RID = VehicleServer.vehicle_get_rid_by_id("lone")
    var trainset:RID = RailVehicleServer.vehicle_get_trainset(lone)
    assert_true(trainset.is_valid())
    assert_eq(_vehicles(trainset), [lone] as Array[RID])
