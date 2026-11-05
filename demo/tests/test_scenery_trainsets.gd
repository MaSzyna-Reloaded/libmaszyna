extends MaszynaGutTest

## A loaded scenery places its trainsets and vehicles through the servers, without a node for
## either: MaszynaVehicleSystem builds the vehicles, RailVehicleServer stands each trainset
## on its track and couples it, and the scenery holds their handles until it unloads.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## A trainset "express" of `first` (with a driver) and `second` on main_track, `lone` on
## side_track (written "Lone"), and `road_car` on a track the scenery does not have
const SCENERY:String = "trainsets.scn"
## A trainset "freight" of `loco` and three wagons all named `wagon`, loaded load_a, load_b and
## load_c, the last one not coupled to a next vehicle
const REPEATED_IDS_SCENERY:String = "trainsets_repeated_ids.scn"
## A trainset "moving" of `head`, `reversed` (stood the other way round) and `tail`, starting at
## 40 km/h
const MOVING_REVERSED_SCENERY:String = "trainset_moving_reversed.scn"
## A trainset of one vehicle, its coupling 0
const SINGLE_SCENERY:String = "trainset_single.scn"
## An entry of the arrangement standing for a vehicle added to the trainset
const ADDED:int = -1
## couplingdata 3 of the fixtures: the coupler and the brake hose
const DECLARED:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_BRAKE
## couplingdata 0 of REPEATED_IDS_SCENERY's last wagon
const UNCOUPLED:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_NONE
## What the scenery selector gives a vehicle the player adds, and what the player chooses here
const FULL:RailVehicleController.CouplingMode = RailVehicleController.COUPLING_MODE_FULL
## How long the moving trainset is let run [s] - about 20 m at its 40 km/h
const MOVING_SECONDS:float = 2.0
## How much less than its start-up velocity would take it the trainset may run [m]
const MOVING_MIN_DISTANCE:float = 15.0
## How far apart the vehicles of a trainset may drift while it runs - the play of their couplers [m]
const COUPLED_DRIFT_TOLERANCE:float = 0.1
## How exactly the second vehicle stands a vehicle's length behind the first [m]
const PLACEMENT_TOLERANCE:float = 0.01

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


func test_a_loaded_scenery_has_no_node_for_a_vehicle_or_a_trainset() -> void:
    await _scenery.load()

    assert_eq(_scenery.find_children("", "RailVehicle3D", true, false).size(), 0)
    assert_eq(_scenery.find_children("", "TrainSet3D", true, false).size(), 0)
    assert_eq(_ids(_scenery.get_vehicles()), ["first", "second", "lone"],
            "its vehicles in the order of the file, without the one on a track that is not built")
    assert_eq(_scenery.get_trainsets().size(), 2, "the trainset, and the dynamic outside one")
    for vehicle:RID in _scenery.get_vehicles():
        assert_true(RailVehicleRenderingServer.vehicle_is_attached(vehicle), "drawn without a node")


func test_the_trainset_stands_its_vehicles_in_order_and_couples_them() -> void:
    await _scenery.load()

    var trainset:RID = _scenery.get_trainsets()[0]
    assert_eq(RailVehicleServer.trainset_get_name(trainset), "express")
    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(trainset))
    assert_eq(_ids(vehicles), ["first", "second"])
    var coupled:Array[RID] = []
    coupled.assign(RailVehicleServer.vehicle_get_coupled(
            vehicles[0], RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER))
    assert_eq(coupled.size(), 2, "the two are coupled")
    assert_almost_eq(
            RailVehicleServer.vehicle_get_transform(vehicles[0]).origin.distance_to(
                    RailVehicleServer.vehicle_get_transform(vehicles[1]).origin),
            VehicleServer.vehicle_get_dimensions(vehicles[0]).z, PLACEMENT_TOLERANCE,
            "the second stands right behind the first")


func test_the_vehicle_with_somebody_aboard_gets_its_driver() -> void:
    await _scenery.load()

    assert_eq(_scenery.first_vehicle_id, "first", "the player belongs in the vehicle with a driver")
    assert_true(DriverServer.vehicle_get_driver(VehicleServer.vehicle_get_rid_by_id("first")).is_valid())
    assert_false(DriverServer.vehicle_get_driver(VehicleServer.vehicle_get_rid_by_id("second")).is_valid())


## A vehicle is found by its id - its name in lower case, as the original reads it - and shown by
## its name as the .scn writes it (maszyna_node_importer.gd, simulationstateserializer.cpp:424-431)
func test_a_vehicle_is_found_by_its_id_and_shown_by_its_name() -> void:
    await _scenery.load()

    var lone:RID = VehicleServer.vehicle_get_rid_by_id("lone")
    assert_true(lone.is_valid(), "found by its id")
    assert_eq(VehicleServer.vehicle_get_id(lone), "lone")
    assert_eq(VehicleServer.vehicle_get_name(lone), "Lone", "shown as the .scn writes it")
    assert_false(VehicleServer.vehicle_get_rid_by_id("Lone").is_valid(), "the name finds nothing")


func test_a_skin_chosen_for_the_load_is_that_vehicles_alone() -> void:
    _scenery.trainset_override.assign([
        _arranged("first", 0, "dynamic/test/synthetic_v1", "synthetic", "chosen", DECLARED),
        _arranged("second", 1, "dynamic/test/synthetic_v1", "synthetic", "none", DECLARED),
    ])
    await _scenery.load()

    var vehicles:Array[RID] = _scenery.get_vehicles()
    assert_eq(MaszynaVehicleSystem.vehicle_get_dynamic(vehicles[0]).skin, "chosen")
    assert_eq(MaszynaVehicleSystem.vehicle_get_dynamic(vehicles[1]).skin, "none")


func test_an_arranged_trainset_changes_adds_removes_and_moves_its_vehicles() -> void:
    # second goes first, first becomes another vehicle and keeps its driver, a vehicle is added
    # at the end, and lone - another trainset's - stays as declared
    _scenery.trainset_override.assign([
        _arranged("second", 1, "dynamic/test/synthetic_v1", "synthetic", "none", DECLARED),
        _arranged("first", 0, "dynamic/test/animated_v1", "animated", "none", DECLARED),
        _arranged("", ADDED, "dynamic/test/synthetic_v1", "synthetic", "none", FULL),
    ])
    # and the second is turned round
    _scenery.trainset_override[0].direction = TrackServer.DIRECTION_REVERSED
    await _scenery.load()

    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    assert_eq(_ids(vehicles), ["second", "first", "second_2"], "in the arranged order, the added one named")
    assert_eq(MaszynaVehicleSystem.vehicle_get_dynamic(vehicles[1]).file_name, "animated")
    # "along" runs one way for a vehicle standing with its track, the other way against it
    var turned:float = float(RailVehicleServer.vehicle_get_track_position(vehicles[0])["along"])
    var kept:float = float(RailVehicleServer.vehicle_get_track_position(vehicles[1])["along"])
    assert_lt(signf(turned) * signf(kept), 0.0, "the turned vehicle stands the other way round")
    assert_eq(MaszynaVehicleSystem.vehicle_get_dynamic(vehicles[1]).driver_type,
            MaszynaVehicleData.DriverType.DRIVER_HEAD, "the changed vehicle keeps its driver")
    assert_eq(MaszynaVehicleSystem.vehicle_get_dynamic(vehicles[2]).driver_type,
            MaszynaVehicleData.DriverType.DRIVER_NOBODY, "nobody aboard the added one")
    var coupled:Array[RID] = []
    coupled.assign(RailVehicleServer.vehicle_get_coupled(
            vehicles[0], RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER))
    assert_eq(coupled.size(), 3, "the three are coupled")
    assert_true(VehicleServer.vehicle_get_rid_by_id("lone").is_valid(), "another trainset is left as declared")


func test_a_vehicle_left_out_of_the_arranged_trainset_is_not_built() -> void:
    _scenery.trainset_override.assign([_arranged("first", 0, "dynamic/test/synthetic_v1", "synthetic", "none", DECLARED)])
    await _scenery.load()

    assert_eq(_ids(_scenery.get_vehicles()), ["first", "lone"])


func test_an_arranged_trainset_with_repeated_ids_is_coupled_through() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [0, 1, 2, 3]
    _scenery.trainset_override.assign(_arranged_repeated_ids(places))
    await _scenery.load()

    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    assert_eq(_loads(vehicles), ["", "load_a", "load_b", "load_c"], "each wagon with its own load")
    var coupled:Array[RID] = []
    coupled.assign(RailVehicleServer.vehicle_get_coupled(
            vehicles[0], RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER))
    assert_eq(coupled.size(), 4, "every wagon coupled, not only the one behind the loco")


func test_a_moved_wagon_of_a_repeated_id_keeps_its_own_load() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    # the last wagon goes behind the loco, coupled to the next as the player chooses - its own mode
    # couples it to nothing
    var places:Array[int] = [0, 3, 1, 2]
    var arranged:Array[TrainsetMemberData] = _arranged_repeated_ids(places)
    arranged[1].coupling_mode = FULL
    _scenery.trainset_override.assign(arranged)
    await _scenery.load()

    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    assert_eq(_loads(vehicles), ["", "load_c", "load_a", "load_b"])


func test_a_reversed_vehicle_of_a_moving_trainset_runs_with_it() -> void:
    _scenery.filename = MOVING_REVERSED_SCENERY
    await _scenery.load()
    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    var start:Array[Vector3] = []
    for vehicle:RID in vehicles:
        start.append(VehicleServer.vehicle_get_position(vehicle))
    assert_lt(VehicleServer.vehicle_get_velocity(vehicles[1]), 0.0,
            "the reversed one starts backwards along its own front")

    await step(ticks(MOVING_SECONDS))

    var head_run:float = VehicleServer.vehicle_get_position(vehicles[0]).distance_to(start[0])
    assert_gt(head_run, MOVING_MIN_DISTANCE, "the trainset runs on")
    for index:int in vehicles.size():
        assert_almost_eq(VehicleServer.vehicle_get_position(vehicles[index]).distance_to(start[index]), head_run,
                COUPLED_DRIFT_TOLERANCE, "%s runs as far as the head" % VehicleServer.vehicle_get_id(vehicles[index]))


## MaSzyna-Reloaded/libmaszyna#305: the declared last wagon couples to nothing (couplingdata 0); a
## vehicle added before it couples to it
func test_a_vehicle_added_before_the_uncoupled_last_wagon_couples_it() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [0, 1, 2, ADDED, 3]
    _scenery.trainset_override.assign(_arranged_repeated_ids(places))
    await _scenery.load()

    assert_eq(_coupled_through(), 5, "the whole trainset coupled")


## A moved vehicle keeps its coupling mode: the uncoupled last wagon moved into the middle couples to
## the next as the player chooses
func test_the_last_wagon_moved_into_the_middle_couples_as_chosen() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [0, 3, 1, 2]
    var arranged:Array[TrainsetMemberData] = _arranged_repeated_ids(places)
    assert_eq(arranged[1].coupling_mode, UNCOUPLED, "moved with its own mode")
    arranged[1].coupling_mode = FULL
    _scenery.trainset_override.assign(arranged)
    await _scenery.load()

    assert_eq(_coupled_through(), 4, "the whole trainset coupled")


func test_the_last_wagon_moved_into_the_middle_keeps_it_uncoupled() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [0, 3, 1, 2]
    _scenery.trainset_override.assign(_arranged_repeated_ids(places))
    await _scenery.load()

    assert_eq(_coupled_through(), 2, "the loco and the moved wagon, which couples to nothing behind")


func test_a_wagon_turned_round_in_the_middle_couples_it() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [0, 1, 2, 3]
    var arranged:Array[TrainsetMemberData] = _arranged_repeated_ids(places)
    arranged[2].direction = TrackServer.DIRECTION_REVERSED
    _scenery.trainset_override.assign(arranged)
    await _scenery.load()

    assert_eq(_coupled_through(), 4, "the whole trainset coupled")


func test_a_vehicle_added_behind_the_last_wagon_couples_as_chosen() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [0, 1, 2, 3, ADDED]
    var arranged:Array[TrainsetMemberData] = _arranged_repeated_ids(places)
    arranged[3].coupling_mode = FULL
    _scenery.trainset_override.assign(arranged)
    await _scenery.load()

    assert_eq(_coupled_through(), 5, "the whole trainset coupled")


## A member couples by its mode of what both couplers allow: the loco to its wagon by the .scn's
## couplingdata 3 (the coupler and the brake hose), the added vehicle fully - by all synthetic.fiz's
## AllowedFlag 63 gives (the coupler, the brake hose, the control line, the high voltage, the
## gangway, the main hose)
func test_each_member_couples_by_its_mode_of_what_the_couplers_allow() -> void:
    _scenery.filename = REPEATED_IDS_SCENERY
    var places:Array[int] = [ADDED, 0, 1, 2, 3]
    _scenery.trainset_override.assign(_arranged_repeated_ids(places))
    await _scenery.load()

    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    var added:RailVehicleController = VehicleServer.vehicle_get_controller(vehicles[0])
    var loco:RailVehicleController = VehicleServer.vehicle_get_controller(vehicles[1])
    var allowed:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_COUPLER
            | RailVehicleController.COUPLING_FLAG_BRAKEHOSE | RailVehicleController.COUPLING_FLAG_CONTROL
            | RailVehicleController.COUPLING_FLAG_HIGHVOLTAGE | RailVehicleController.COUPLING_FLAG_GANGWAY
            | RailVehicleController.COUPLING_FLAG_MAINHOSE) as RailVehicleController.CouplingFlags
    assert_eq(RailVehicleServer.coupler_get_joinable_flags(vehicles[0], RailVehicleController.COUPLER_END_REAR),
            allowed, "what both couplers allow")
    assert_true(added.is_coupled_by(RailVehicleController.COUPLER_END_REAR, allowed),
            "the added vehicle couples to the loco by all of it")
    var declared:RailVehicleController.CouplingFlags = (RailVehicleController.COUPLING_FLAG_COUPLER
            | RailVehicleController.COUPLING_FLAG_BRAKEHOSE) as RailVehicleController.CouplingFlags
    assert_true(loco.is_coupled_by(RailVehicleController.COUPLER_END_REAR, declared),
            "the loco and its wagon as the .scn couples them")
    assert_false(loco.is_coupled_by(RailVehicleController.COUPLER_END_REAR, RailVehicleController.COUPLING_FLAG_CONTROL),
            "and by nothing the .scn does not ask for")


func test_a_vehicle_added_to_a_trainset_of_one_couples_to_it() -> void:
    _scenery.filename = SINGLE_SCENERY
    _scenery.trainset_override.assign([
        _arranged("loco", 0, "dynamic/test/synthetic_v1", "synthetic", "none", FULL),
        _arranged("", ADDED, "dynamic/test/synthetic_v1", "synthetic", "none", FULL),
    ])
    await _scenery.load()

    assert_eq(_coupled_through(), 2, "the two coupled")


func test_unloading_frees_the_vehicles_and_the_trainsets() -> void:
    await _scenery.load()
    var vehicles:Array[RID] = _scenery.get_vehicles().duplicate()

    _scenery.filename = ""
    await _scenery.load()

    assert_eq(_scenery.get_vehicles().size(), 0)
    assert_eq(_scenery.get_trainsets().size(), 0)
    for vehicle:RID in vehicles:
        assert_false(MaszynaVehicleSystem.vehicle_exists(vehicle))
    assert_false(VehicleServer.vehicle_get_rid_by_id("first").is_valid())


## How many vehicles the first trainset's front one is coupled through to, itself among them
func _coupled_through() -> int:
    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    return RailVehicleServer.vehicle_get_coupled(
            vehicles[0], RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER).size()


func _loads(vehicles:Array[RID]) -> Array:
    return vehicles.map(func(vehicle:RID) -> String:
        return MaszynaVehicleSystem.vehicle_get_dynamic(vehicle).load_name)


## The trainset of REPEATED_IDS_SCENERY as the selector sends it, its vehicles at `places`: each
## declared one with its own coupling mode, an added one coupled fully
func _arranged_repeated_ids(places:Array[int]) -> Array[TrainsetMemberData]:
    const LAST_WAGON:int = 3
    var arranged:Array[TrainsetMemberData] = []
    for place:int in places:
        var mode:RailVehicleController.CouplingMode = (
                FULL if place == ADDED else UNCOUPLED if place == LAST_WAGON else DECLARED)
        arranged.append(_arranged("loco" if place == 0 else "wagon", place, "dynamic/test/synthetic_v1",
                "synthetic", "none", mode))
    return arranged


func _ids(vehicles:Array[RID]) -> Array:
    return vehicles.map(func(vehicle:RID) -> String: return VehicleServer.vehicle_get_id(vehicle))


## An entry of the player's trainset as the scenery selector arranges it: a declared vehicle at
## its place in the declared trainset, -1 for one added, coupled to the next by coupling_mode
func _arranged(vehicle_name:String, trainset_index:int, data_path:String, file_name:String,
        skin:String, coupling_mode:RailVehicleController.CouplingMode) -> TrainsetMemberData:
    var dynamic:MaszynaVehicleData = MaszynaVehicleData.new()
    dynamic.vehicle_id = vehicle_name
    dynamic.data_path = data_path
    dynamic.file_name = file_name
    dynamic.skin = skin
    var member:TrainsetMemberData = TrainsetMemberData.new()
    member.vehicle = dynamic
    member.trainset_index = trainset_index
    member.coupling_mode = coupling_mode
    return member
