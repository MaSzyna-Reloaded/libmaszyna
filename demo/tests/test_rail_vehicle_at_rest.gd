extends MaszynaGutTest

## A vehicle standing still is standing still. Reported from the cab: at zero speed the transform
## changes a little from frame to frame and the vehicle visibly jumps.
##
## RailVehicleRenderingServer puts the node where RailVehicleServer places the vehicle every step
## it moved, so anything that makes the server hand back a slightly different transform each step
## is visible immediately.

var created_tracks:Array[RID] = []
var created_vehicles:Array[RailVehicle3D] = []
var created_vehicle_nodes:Array[VehiclePhysicsNode] = []


func after_each() -> void:
    for vehicle:RailVehicle3D in created_vehicles:
        if is_instance_valid(vehicle):
            if vehicle.get_parent():
                vehicle.get_parent().remove_child(vehicle)
            vehicle.queue_free()
    created_vehicles.clear()
    created_vehicle_nodes.clear()
    for track_rid:RID in created_tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    created_tracks.clear()
    TrackServer.topology_rebuild()


func test_a_vehicle_at_rest_keeps_its_transform() -> void:
    await _assert_still(_straight_track(), 100.0, "on a straight track")


## The body's transform is derived from where its two bogies sit, so on a curve a movement far too
## small to see along the track becomes a visible turn of the whole vehicle.
func test_a_vehicle_at_rest_on_a_curve_keeps_its_transform() -> void:
    await _assert_still(_curved_track(), 40.0, "on a curve, with bogies")


func _straight_track() -> String:
    var curve := TrackCurve.new()
    curve.p1 = Vector3(0.0, 0.0, 0.0)
    curve.p2 = Vector3(200.0, 0.0, 0.0)
    var track:RID = TrackServer.track_create()
    created_tracks.append(track)
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, "rest", 1.435)
    TrackServer.topology_rebuild()
    return "rest"


func _curved_track() -> String:
    var curve := TrackCurve.new()
    curve.p1 = Vector3(0.0, 0.0, 0.0)
    curve.p2 = Vector3(80.0, 0.0, 80.0)
    curve.c1 = Vector3(60.0, 0.0, 0.0)
    curve.c2 = Vector3(0.0, 0.0, -60.0)
    var track:RID = TrackServer.track_create()
    created_tracks.append(track)
    TrackServer.track_update_curves(track, curve, null)
    TrackServer.track_update(track, TrackServer.TRACK_NORMAL, "rest_curve", 1.435)
    TrackServer.topology_rebuild()
    return "rest_curve"


func _assert_still(track_name:String, offset:float, where:String) -> void:
    # a vehicle with no mass integrates to NaN, which is a fixture of no vehicle at all
    var model:RailVehicleController = MoverRailVehicleController.new()
    model.vehicle_id = "test_at_rest"
    model.mass = 74000.0
    model.type_name = "test"
    var physics_node:VehiclePhysicsNode = build_vehicle_node("test_at_rest", model)
    created_vehicle_nodes.append(physics_node)
    var wheels:RailVehicleWheels = MoverRailVehicleWheels.new()
    wheels.bogie_pivot_spacing = 6.0
    VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid()).add_component(wheels)

    var vehicle := RailVehicle3D.new()
    var front_bogie := Node3D.new()
    front_bogie.name = "FrontBogie"
    front_bogie.position.z = -3.0
    vehicle.add_child(front_bogie)
    var rear_bogie := Node3D.new()
    rear_bogie.name = "RearBogie"
    rear_bogie.position.z = 3.0
    vehicle.add_child(rear_bogie)
    vehicle.front_bogie_path = NodePath("FrontBogie")
    vehicle.rear_bogie_path = NodePath("RearBogie")
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    add_child(vehicle)
    vehicle.controller_path = vehicle.get_path_to(physics_node)
    created_vehicles.append(vehicle)
    await wait_idle_frames(4)

    var settled:Transform3D = vehicle.global_transform
    var worst_move:float = 0.0
    var worst_turn:float = 0.0
    for frame:int in 40:
        await wait_idle_frames(1)
        var now:Transform3D = vehicle.global_transform
        worst_move = maxf(worst_move, now.origin.distance_to(settled.origin))
        worst_turn = maxf(worst_turn, (now.basis.z - settled.basis.z).length())

    assert_almost_eq(
            float(VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid()).get_velocity()), 0.0, 0.001,
            "the vehicle under test is meant to be standing still %s" % where)
    # a tenth of a millimetre over 40 frames; anything a driver can see is far above this
    assert_lt(worst_move, 0.0001, "a vehicle at rest must not drift %s" % where)
    assert_lt(worst_turn, 0.0001, "and must not turn %s" % where)
