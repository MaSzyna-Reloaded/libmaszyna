extends MaszynaGutTest

## Regression test for the reported "na postoju pojazdy sa odwrocone domyslnie, a jak wsiadziesz
## i predkosc zrobi sie >0, pojazd sie nagle odwraca" bug.
##
## Root cause, confirmed live against the real EP07-424 on td.scn
## (test_zzz_ep07_orientation_diagnostic.gd) and reproduced here in isolation:
## RailVehicle3D::_update_track_transform()'s bogie-refined orientation branch
## (RailVehicle3D.cpp) samples "front_transform" at +bogie_pivot_spacing*0.5 and
## "rear_transform" at -bogie_pivot_spacing*0.5 along the track, unconditionally, regardless of
## the vehicle's own DIRECTION_NORMAL/DIRECTION_REVERSED placement. But
## RailVehicleServer._move_vehicle_state()'s distance sign convention is itself
## direction-dependent (a positive distance argument DECREASES the track offset for a NORMAL
## vehicle, and INCREASES it for a REVERSED one - see vehicle_set_track()'s own
## "direction_sign" and process_movement()'s "front-relative vs rear-relative" comment). For a
## DIRECTION_NORMAL vehicle this makes "front_transform" land at a *lower* offset than
## "rear_transform" - backwards - so body_forward = front_transform.origin -
## rear_transform.origin ends up pointing opposite the vehicle's real forward direction. This
## branch only actually executes when RailVehicle3D::_update_track_transform() runs with the
## bogie nodes already resolved *and* the vehicle has visibly moved (or `force_detail_refresh`
## is set) - while parked, the coarse (correct) transform from vehicle_get_transform() is all
## that gets applied, so the vehicle looks fine until it moves, at which point the very first
## recomputation flips it 180 degrees. f01cd7f ("Optimize vehicle and sound runtime updates")
## didn't create this sign bug, but by gating _update_track_transform() to velocity != 0 it
## changed *when* this latent bug first fires - from "shortly after spawn, whenever the async
## E3D bogie submodels resolve" to "the instant the vehicle starts actually moving", which is
## what made it look like a fresh regression.
##
## Companion test: test_rail_vehicle_idle_pantograph_voltage_regression.gd (same commit,
## different symptom).

var created_tracks:Array[RID] = []
var created_vehicles:Array[RailVehicle3D] = []
var created_vehicle_nodes: Array[VehiclePhysicsNode] = []


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


func test_normal_direction_vehicle_orientation_does_not_flip_once_it_moves() -> void:
    var vehicle:RailVehicle3D = _spawn_bogie_vehicle(TrackServer.DIRECTION_NORMAL)

    await wait_idle_frames(5)
    var forward_while_parked:Vector3 = _vehicle_forward(vehicle)
    assert_true(
        forward_while_parked.distance_to(Vector3(0.0, 0.0, 1.0)) < 0.1,
        "vehicle should already face along the track while parked, not reversed - got %s" % [
                forward_while_parked],
    )

    # A real driver boarding and moving off - even a tiny nudge is enough to trigger the
    # bogie-refined transform recompute that the coarse "parked" transform above never touched.
    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.5)

    assert_true(
        forward_while_parked.distance_to(_vehicle_forward(vehicle)) < 0.1,
        "vehicle orientation flipped once it moved: parked=%s after moving=%s" % [
                forward_while_parked, _vehicle_forward(vehicle)],
    )


## Same scenario for a DIRECTION_REVERSED vehicle - the closer analog to an EZT's physically
## turned member. Included to pin down whether the sign bug is NORMAL-specific (the vehicle
## already parked reversed relative to the track, so a further flip would put it back to
## looking "normal", which is just as wrong for a REVERSED vehicle).
func test_reversed_direction_vehicle_orientation_does_not_flip_once_it_moves() -> void:
    var vehicle:RailVehicle3D = _spawn_bogie_vehicle(TrackServer.DIRECTION_REVERSED)

    await wait_idle_frames(5)
    var forward_while_parked:Vector3 = _vehicle_forward(vehicle)
    assert_true(
        forward_while_parked.distance_to(Vector3(0.0, 0.0, -1.0)) < 0.1,
        "reversed vehicle should already face opposite the track's own direction while parked, got %s" % [
                forward_while_parked],
    )

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.5)

    assert_true(
        forward_while_parked.distance_to(_vehicle_forward(vehicle)) < 0.1,
        "reversed vehicle orientation flipped once it moved: parked=%s after moving=%s" % [
                forward_while_parked, _vehicle_forward(vehicle)],
    )


func _spawn_bogie_vehicle(direction:TrackServer.Direction) -> RailVehicle3D:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(0.0, 0.0, 60.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start",
    )
    TrackServer.topology_rebuild()

    var physics_node: VehiclePhysicsNode = _create_vehicle_node()
    var controller: VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())
    # the pivot spacing belongs to the wheels, and RailVehicle3D reads it off the vehicle's
    # composed configuration - so the vehicle has to actually have wheels
    var wheels: RailVehicleWheels = MoverRailVehicleWheels.new()
    wheels.bogie_pivot_spacing = 6.0
    controller.add_component(wheels)

    var vehicle:RailVehicle3D = RailVehicle3D.new()
    var front_bogie:Node3D = Node3D.new()
    front_bogie.name = "FrontBogie"
    front_bogie.position.z = -3.0
    var rear_bogie:Node3D = Node3D.new()
    rear_bogie.name = "RearBogie"
    rear_bogie.position.z = 3.0
    vehicle.add_child(front_bogie)
    vehicle.add_child(rear_bogie)
    vehicle.front_bogie_path = NodePath("FrontBogie")
    vehicle.rear_bogie_path = NodePath("RearBogie")
    vehicle.start_track_name = "start"
    vehicle.start_track_offset = 20.0
    vehicle.start_direction = direction
    add_child(vehicle)
    vehicle.controller_path = vehicle.get_path_to(physics_node)
    created_vehicles.append(vehicle)
    return vehicle


## The vehicle's node, because RailVehicle3D is pointed at it by path - the controller it owns is
## not a node and has none.
func _create_vehicle_node() -> VehiclePhysicsNode:
    var physics_node: VehiclePhysicsNode = build_vehicle_node(
            "test_train_%d" % created_vehicle_nodes.size())
    created_vehicle_nodes.append(physics_node)
    return physics_node


func _register_track(
    curve1:TrackCurve,
    curve2:TrackCurve = null,
    type:int = TrackServer.TRACK_NORMAL,
    name:String = "",
) -> RID:
    var track_rid:RID = TrackServer.track_create()
    created_tracks.append(track_rid)
    TrackServer.track_update_curves(track_rid, curve1, curve2)
    TrackServer.track_update(track_rid, type, name, 1.435)
    return track_rid


func _curve(p1:Vector3, p2:Vector3, roll1:float = 0.0, roll2:float = 0.0) -> TrackCurve:
    var curve:TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    curve.roll1 = roll1
    curve.roll2 = roll2
    return curve


func _vehicle_forward(vehicle:RailVehicle3D) -> Vector3:
    return -vehicle.global_basis.z.normalized()
