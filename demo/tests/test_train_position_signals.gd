extends MaszynaGutTest

var _created_tracks: Array[RID] = []
var _created_vehicles: Array[RailVehicle3D] = []
var _created_vehicle_nodes: Array[VehiclePhysicsNode] = []


func after_each() -> void:
    for vehicle: RailVehicle3D in _created_vehicles:
        if is_instance_valid(vehicle):
            if vehicle.get_parent():
                vehicle.get_parent().remove_child(vehicle)
            vehicle.queue_free()
    _created_vehicles.clear()

    _created_vehicle_nodes.clear()

    for track_rid: RID in _created_tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    _created_tracks.clear()
    TrackServer.topology_rebuild()


func test_train_position_changed_signal_emits_after_crossing_one_meter() -> void:
    var fixture: Dictionary = await _create_fixture(0.0)
    var train: VehicleController = fixture["controller"]
    var vehicle: RailVehicle3D = fixture["vehicle"]

    watch_signals(train)
    watch_signals(VehicleServer)

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.5)
    assert_signal_not_emitted(train, "position_changed", "Should not emit for a 0.5m move")
    assert_signal_not_emitted(VehicleServer, "vehicle_moved", "VehicleServer should not relay yet")

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.6)
    assert_signal_emitted(train, "position_changed", "Should emit after crossing 1m total movement")
    assert_signal_emitted_with_parameters(VehicleServer, "vehicle_moved", [train.get_rid(), train.get_world_position()])


func test_train_position_changed_signal_rearms_after_last_emission() -> void:
    var fixture: Dictionary = await _create_fixture(1.1)
    var train: VehicleController = fixture["controller"]
    var vehicle: RailVehicle3D = fixture["vehicle"]

    watch_signals(train)

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.9)
    assert_signal_not_emitted(train, "position_changed", "Should not emit before another full meter of movement")

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.2)
    assert_signal_emitted(train, "position_changed", "Should emit after another 1m from the last emitted position")


func test_vehicle_server_stops_relaying_a_detached_controller() -> void:
    var fixture: Dictionary = await _create_fixture(0.0, "test_train_2")
    var train: VehicleController = fixture["controller"]
    var vehicle: RailVehicle3D = fixture["vehicle"]

    watch_signals(VehicleServer)
    VehicleServer.vehicle_bind_controller(train.get_rid(), RID())

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 2.0)
    assert_signal_not_emitted(VehicleServer, "vehicle_moved", "Should not relay a controller that left its handle")


func _create_fixture(offset: float, train_id: String = "test_train") -> Dictionary:
    var track_rid: RID = TrackServer.track_create()
    _created_tracks.append(track_rid)
    TrackServer.track_update_curves(track_rid, _curve(Vector3(0.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)), null)
    TrackServer.track_update(track_rid, TrackServer.TRACK_NORMAL, "start", 1.435)
    TrackServer.topology_rebuild()

    var physics_node: VehiclePhysicsNode = build_vehicle_node(train_id)
    var controller: VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())
    _created_vehicle_nodes.append(physics_node)

    var vehicle: RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = "start"
    vehicle.start_track_offset = offset
    vehicle.set("start_direction", TrackServer.DIRECTION_REVERSED)
    add_child(vehicle)
    vehicle.controller_path = vehicle.get_path_to(physics_node)
    _created_vehicles.append(vehicle)
    await step(2)

    return {
        "controller": VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid()),
        "vehicle": vehicle,
    }


func _curve(p1: Vector3, p2: Vector3) -> TrackCurve:
    var curve: TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    return curve
