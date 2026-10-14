extends MaszynaGutTest
## A RailVehicle3D is placed on its start track once both exist - the track and the vehicle its
## VehiclePhysicsNode builds. In the editor the tracks come first and the vehicle only once its .fiz
## is read; the placement used to be spent on the tracks alone and the vehicle stood off its track
## until start_track_offset was touched.

const TRACK_NAME: String = "start_track_late_vehicle"
const TRACK_LENGTH: float = 100.0
const OFFSET: float = 30.0

var _track: RID = RID()


func after_each() -> void:
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func test_a_vehicle_built_after_the_tracks_is_placed_on_its_start_track() -> void:
    _track = build_track(TRACK_NAME, TRACK_LENGTH)
    var vehicle: RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = TRACK_NAME
    vehicle.start_track_offset = OFFSET
    add_child_autofree(vehicle)
    # the tracks are announced while the vehicle does not exist yet
    TrackServer.topology_rebuild()

    var description: RailVehicleController = MoverRailVehicleController.new()
    description.mass = RAIL_VEHICLE_MASS
    var physics_node: RailVehiclePhysicsNode = RailVehiclePhysicsNode.new()
    physics_node.name = "LateVehicle"
    physics_node.set_controller(description)
    add_child_autofree(physics_node)
    vehicle.controller_path = NodePath("../LateVehicle")
    await wait_idle_frames(3)

    assert_eq(vehicle.get_rid(), physics_node.get_vehicle_rid(), "the node draws the vehicle built later")
    assert_eq(RailVehicleServer.vehicle_get_track_position(vehicle.get_rid())["track_rid"], _track,
            "and it stands on its start track")
    remove_child(vehicle)
