extends MaszynaGutTest

## A trainset stands its vehicles on its track one after another from its front, each as long as
## it is, and couples each with the next (deserialize_dynamic(), simulationstateserializer.cpp:
## 1062-1076; endtrainset, simulationstateserializer.cpp:818-837). TrainSet3D hands
## RailVehicleServer its RailVehicle3D children in order; reordered, they stand anew.

const TRACK_NAME:String = "trainset_track"
const TRACK_LENGTH:float = 200.0
const FRONT:float = 150.0
const SHORT_LENGTH:float = 10.0
const LONG_LENGTH:float = 16.0
const GAP:float = 2.0
const TOLERANCE:float = 0.01
## couplingdata 3: coupler and brake hose
const COUPLING:int = RailVehicleController.COUPLING_FLAG_COUPLER | RailVehicleController.COUPLING_FLAG_BRAKEHOSE
const SETTLE_FRAMES:int = 3

var _track:RID
var _trainset:TrainSet3D
var _physics_nodes:Array[VehiclePhysicsNode] = []


func before_each() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)


func after_each() -> void:
    if is_instance_valid(_trainset):
        _trainset.free()
    for node:VehiclePhysicsNode in _physics_nodes:
        node.free()
    _physics_nodes.clear()
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


## A vehicle of the trainset, as long as asked, standing the way asked
func _add_vehicle(vehicle_name:String, length:float, direction:TrackServer.Direction) -> RailVehicle3D:
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.type_name = "test"
    model.mass = RAIL_VEHICLE_MASS
    model.dimensions_length = length
    var physics_node:VehiclePhysicsNode = RailVehiclePhysicsNode.new()
    physics_node.name = vehicle_name + "Physics"
    physics_node.controller = model
    add_child(physics_node)
    _physics_nodes.append(physics_node)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    vehicle.name = vehicle_name
    vehicle.start_direction = direction
    vehicle.controller_path = NodePath("../../%s" % physics_node.name)
    _trainset.add_child(vehicle)
    return vehicle


func _build_trainset() -> Array[RailVehicle3D]:
    _trainset = TrainSet3D.new()
    _trainset.start_track_name = TRACK_NAME
    _trainset.start_track_offset = FRONT
    var gaps:Array[float] = [0.0, GAP, 0.0]
    _trainset.vehicle_gaps = gaps
    var couplings:Array[int] = [COUPLING, COUPLING]
    _trainset.couplings = couplings
    add_child(_trainset)
    var vehicles:Array[RailVehicle3D] = [
        _add_vehicle("First", SHORT_LENGTH, TrackServer.DIRECTION_NORMAL),
        _add_vehicle("Second", LONG_LENGTH, TrackServer.DIRECTION_NORMAL),
        _add_vehicle("Third", SHORT_LENGTH, TrackServer.DIRECTION_REVERSED),
    ]
    return vehicles


func _offset(vehicle:RailVehicle3D) -> float:
    # "along" runs against a vehicle standing the track's way: its offset either way is its size
    return absf(float(RailVehicleServer.vehicle_get_track_position(vehicle.get_rid()).get("along", 0.0)))


func test_the_vehicles_stand_one_after_another_from_the_front() -> void:
    var vehicles:Array[RailVehicle3D] = _build_trainset()
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_offset(vehicles[0]), FRONT - SHORT_LENGTH * 0.5, TOLERANCE, "the first at the front")
    assert_almost_eq(_offset(vehicles[1]), FRONT - SHORT_LENGTH - GAP - LONG_LENGTH * 0.5, TOLERANCE,
            "the second its gap behind the first")
    assert_almost_eq(_offset(vehicles[2]), FRONT - SHORT_LENGTH - GAP - LONG_LENGTH - SHORT_LENGTH * 0.5, TOLERANCE,
            "the third right behind: standing reversed it has no gap")


func test_the_vehicles_are_coupled_in_order() -> void:
    var vehicles:Array[RailVehicle3D] = _build_trainset()
    await wait_idle_frames(SETTLE_FRAMES)

    var coupled:Array[RID] = RailVehicleServer.vehicle_get_coupled(
            vehicles[0].get_rid(), RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_BRAKEHOSE)
    assert_eq(coupled.size(), 3, "the three are one trainset, joined by the brake hose too")
    assert_true(VehicleServer.vehicle_get_controller(vehicles[2].get_rid()).is_coupled(
            RailVehicleController.COUPLER_END_REAR), "the reversed one is coupled at its rear")


func test_reordered_the_vehicles_stand_anew() -> void:
    var vehicles:Array[RailVehicle3D] = _build_trainset()
    await wait_idle_frames(SETTLE_FRAMES)

    _trainset.move_child(vehicles[1], 0)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_offset(vehicles[1]), FRONT - LONG_LENGTH * 0.5, TOLERANCE, "the long one at the front now")
    # the pairs of the old order let go: coupled again pair by pair they would close into a ring
    var coupled:Array[RID] = RailVehicleServer.vehicle_get_coupled(
            vehicles[1].get_rid(), RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_COUPLER)
    assert_eq(coupled, [vehicles[1].get_rid(), vehicles[0].get_rid(), vehicles[2].get_rid()] as Array[RID],
            "one trainset in the new order, open at both ends")


func test_a_trainset_takes_its_start_track_from_a_child_vehicle() -> void:
    _trainset = TrainSet3D.new()
    var first:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    first.start_track_name = TRACK_NAME
    first.start_track_offset = FRONT
    var second:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    second.start_track_name = TRACK_NAME
    second.start_track_offset = FRONT - SHORT_LENGTH
    _trainset.add_child(first)
    _trainset.add_child(second)
    add_child(_trainset)

    assert_eq(_trainset.start_track_name, TRACK_NAME)
    assert_eq(_trainset.start_track_offset, FRONT)
    assert_eq(first.start_track_name, "")
    assert_eq(first.start_track_offset, 0.0)
    assert_eq(second.start_track_name, "")
    assert_eq(second.start_track_offset, 0.0)


func test_a_trainset_with_its_own_start_track_leaves_children_unchanged() -> void:
    _trainset = TrainSet3D.new()
    _trainset.start_track_name = TRACK_NAME
    _trainset.start_track_offset = FRONT
    var vehicle:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    vehicle.start_track_name = "child_track"
    vehicle.start_track_offset = FRONT - SHORT_LENGTH
    _trainset.add_child(vehicle)
    add_child(_trainset)

    assert_eq(_trainset.start_track_name, TRACK_NAME)
    assert_eq(_trainset.start_track_offset, FRONT)
    assert_eq(vehicle.start_track_name, "child_track")
    assert_eq(vehicle.start_track_offset, FRONT - SHORT_LENGTH)
