extends MaszynaGutTest

var created_tracks: Array[RID] = []
var created_vehicles: Array[RailVehicle3D] = []
var created_vehicle_nodes: Array[VehiclePhysicsNode] = []


func after_each() -> void:
    for vehicle: RailVehicle3D in created_vehicles:
        if is_instance_valid(vehicle):
            if vehicle.get_parent():
                vehicle.get_parent().remove_child(vehicle)
            vehicle.queue_free()
    created_vehicles.clear()

    created_vehicle_nodes.clear()

    for track_rid: RID in created_tracks:
        if TrackServer.track_exists(track_rid):
            TrackServer.track_free(track_rid)
    created_tracks.clear()
    TrackServer.topology_rebuild()


func test_dynamic_vehicle_track_properties_do_not_rebuild_vehicle() -> void:
    var dynamic_vehicle:MaszynaRailVehicle3D = MaszynaRailVehicle3D.new()
    watch_signals(dynamic_vehicle)
    add_child_autofree(dynamic_vehicle)
    await step(2)

    dynamic_vehicle.start_track_name = "common"
    dynamic_vehicle.start_track_offset = 12.0
    dynamic_vehicle.start_direction = TrackServer.DIRECTION_REVERSED
    await step(2)

    assert_signal_emit_count(dynamic_vehicle, "vehicle_built", 1, "track placement should not rebuild the dynamic vehicle")


func test_start_track_name_initializes_and_clamps_offset() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    TrackServer.topology_rebuild()

    var fixture: Dictionary = await _create_vehicle("start", 15.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    _assert_vector_eq(controller.get_world_position(), _rail_position(10.0, 0.0), "controller should be placed at track end")
    _assert_vector_eq(vehicle.global_position, _rail_position(10.0, 0.0), "vehicle should be placed at track end")


func test_bogies_follow_track_tangents() -> void:
    _register_track(
        _demo_curve(
            Vector3(0.0, 0.0, 0.0),
            Vector3(10.0, 0.0, 10.0),
            Vector3(10.0, 0.0, 0.0),
            Vector3(0.0, 0.0, -10.0),
        ),
        null,
        TrackServer.TRACK_NORMAL,
        "curve",
    )
    TrackServer.topology_rebuild()

    var physics_node: VehiclePhysicsNode = _create_vehicle_node()
    var controller: VehicleController = VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid())
    # the pivot spacing belongs to the wheels, and RailVehicle3D reads it off the vehicle's
    # composed configuration - so the vehicle has to actually have wheels
    # a component is configured and then attached - attaching is what writes it to the backend
    var wheels: RailVehicleWheels = MoverRailVehicleWheels.new()
    wheels.bogie_pivot_spacing = 6.0
    controller.add_component(wheels)
    var vehicle:RailVehicle3D = RailVehicle3D.new()
    var submodels:Dictionary = {
        "bogie1": Transform3D(Basis(), Vector3(0.0, 0.0, -3.0)),
        "bogie2": Transform3D(Basis(), Vector3(0.0, 0.0, 3.0)),
        "wheel01": Transform3D(),
    }
    var parents:Dictionary = {"wheel01": "bogie1"}
    vehicle.add_child(build_model_instance(submodels, parents))
    vehicle.model_instance_path = NodePath("Model")
    vehicle.front_bogie_path = NodePath("Model/bogie1")
    vehicle.rear_bogie_path = NodePath("Model/bogie2")
    vehicle.powered_wheel_paths = [NodePath("Model/bogie1/wheel01")]
    vehicle.start_track_name = "curve"
    vehicle.start_track_offset = TrackServer.track_get_length(created_tracks[0]) * 0.5
    vehicle.start_direction = TrackServer.DIRECTION_REVERSED
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    created_vehicles.append(vehicle)
    await step(2)
    var front_bogie:Node3D = vehicle.get_node("Model/bogie1")
    var rear_bogie:Node3D = vehicle.get_node("Model/bogie2")
    var powered_wheel:Node3D = vehicle.get_node("Model/bogie1/wheel01")

    var front_forward:Vector3 = -front_bogie.global_basis.z.normalized()
    var rear_forward:Vector3 = -rear_bogie.global_basis.z.normalized()
    assert_true(
        front_forward.distance_to(rear_forward) > 0.05,
        "bogies should follow different tangents on a curved track",
    )

    # The vehicle publishes the axle angle; the node's only job is to put it on the wheel, around
    # its local X, like the original's UpdateAxle() (DynObj.cpp:489). Driving the vehicle is what
    # makes the angle non-trivial - the state is read-only, so it cannot be injected.
    RailVehicleServer.vehicle_move(vehicle.get_rid(), 1.5)
    await step(2)
    var published_angle:float = wheels.get_angle_powered_deg()
    assert_almost_eq(
        Basis(Vector3.RIGHT, deg_to_rad(published_angle)).get_euler().x,
        powered_wheel.transform.basis.get_euler().x,
        0.001,
        "the powered wheel should carry the angle the vehicle publishes",
    )



func test_start_track_name_retries_after_tracks_changed_when_track_is_added_later() -> void:
    var fixture: Dictionary = await _create_vehicle("late_track", 4.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    _assert_vector_eq(controller.get_world_position(), Vector3.ZERO, "controller should stay at origin before the named track exists")
    _assert_vector_eq(vehicle.global_position, Vector3.ZERO, "vehicle should stay at origin before the named track exists")

    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "late_track"
    )
    TrackServer.topology_rebuild()
    await step(2)

    _assert_vector_eq(controller.get_world_position(), _rail_position(4.0, 0.0), "controller should move after the missing track appears")
    _assert_vector_eq(vehicle.global_position, _rail_position(4.0, 0.0), "vehicle should move after the missing track appears")


func test_move_on_track_moves_forward_and_backward_on_current_track() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 2.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 3.0)
    _assert_vector_eq(controller.get_world_position(), _rail_position(5.0, 0.0), "positive movement should update controller position")
    _assert_vector_eq(vehicle.global_position, _rail_position(5.0, 0.0), "positive movement should update position")

    RailVehicleServer.vehicle_move(vehicle.get_rid(), -4.0)
    _assert_vector_eq(controller.get_world_position(), _rail_position(1.0, 0.0), "negative movement should update controller position")
    _assert_vector_eq(vehicle.global_position, _rail_position(1.0, 0.0), "negative movement should update position")


func test_move_on_track_crosses_connected_tracks_and_clamps_at_dead_end() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 5.0)
    _assert_vector_eq(controller.get_world_position(), _rail_position(13.0, 0.0), "movement should continue on next track")
    _assert_vector_eq(vehicle.global_position, _rail_position(13.0, 0.0), "position should continue on next track")

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 20.0)
    _assert_vector_eq(controller.get_world_position(), _rail_position(20.0, 0.0), "movement should clamp at graph end")
    _assert_vector_eq(vehicle.global_position, _rail_position(20.0, 0.0), "dead-end clamp should place vehicle at endpoint")


func test_find_vehicle_measures_between_the_ends_across_tracks() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(100.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    _register_track(
        _curve(Vector3(100.0, 0.0, 0.0), Vector3(200.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "next"
    )
    TrackServer.topology_rebuild()
    var searching: Dictionary = await _create_vehicle("start", 10.0)
    var standing: Dictionary = await _create_vehicle("next", 50.0)
    var searching_rid: RID = (searching["vehicle"] as RailVehicle3D).get_rid()
    var standing_rid: RID = (standing["vehicle"] as RailVehicle3D).get_rid()
    var lengths: float = (searching["controller"] as VehicleController).get_dimensions_length() \
            + (standing["controller"] as VehicleController).get_dimensions_length()

    var ahead: RailVehicleNeighbour = RailVehicleServer.vehicle_find_vehicle(
            searching_rid, RailVehicleController.COUPLER_END_FRONT, 1000.0)
    var behind: RailVehicleNeighbour = RailVehicleServer.vehicle_find_vehicle(
            searching_rid, RailVehicleController.COUPLER_END_REAR, 1000.0)
    var found: RailVehicleNeighbour = ahead if ahead else behind

    assert_not_null(found, "the other vehicle is on the next track")
    assert_true(ahead == null or behind == null, "and only one way")
    assert_eq(found.vehicle_rid, standing_rid)
    _assert_float_eq(found.distance, 140.0 - 0.5 * lengths, "centres 140 m apart, less the half lengths")
    assert_null(RailVehicleServer.vehicle_find_vehicle(
            searching_rid, RailVehicleController.COUPLER_END_FRONT if ahead else RailVehicleController.COUPLER_END_REAR, 80.0),
            "its track begins 90 m ahead, past the range")


func test_move_on_track_updates_direction_when_entering_track_end() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    _register_track(_curve(Vector3(20.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)))
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 5.0)

    _assert_vector_eq(_vehicle_forward(vehicle), Vector3.LEFT, "vehicle should follow reversed track orientation")
    _assert_vector_eq(vehicle.global_position, _rail_position(13.0, 0.0), "vehicle should continue through reversed track")


func test_move_on_track_uses_switch_common_route() -> void:
    _register_track(
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    _register_track(_curve(Vector3(10.0, 0.0, 10.0), Vector3(20.0, 0.0, 20.0)))
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 15.0)

    _assert_vector_eq(controller.get_world_position(), _rail_position(13.0, 0.0), "common switch route should continue on main track")
    _assert_vector_eq(vehicle.global_position, _rail_position(13.0, 0.0), "common switch route should use curve1")


func test_move_on_track_uses_switch_diverging_route() -> void:
    _register_track(
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    _register_track(_curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)))
    _register_track(_curve(Vector3(10.0, 0.0, 10.0), Vector3(20.0, 0.0, 20.0)))
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 19.1421)

    _assert_vector_eq(controller.get_world_position(), _rail_position(12.1213, 12.1213), "diverging route should continue after switch curve length")
    _assert_vector_eq(vehicle.global_position, _rail_position(12.1213, 12.1213), "diverging route should use curve2")


func test_move_on_track_forces_switch_diverging_when_entering_from_diverging_branch() -> void:
    var start_rid: RID = _register_track(
        _curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var start_offset: float = 12.0
    var move_distance: float = 5.0
    var distance_on_switch: float = move_distance - (TrackServer.track_get_length(start_rid) - start_offset)
    var fixture: Dictionary = await _create_vehicle("start", start_offset)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), move_distance)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_DIVERGING)
    _assert_vector_eq(
        controller.get_world_position(),
        _track_position(
            switch_rid,
            TrackServer.track_get_length(switch_rid, TrackServer.TRACK_DIVERGING) - distance_on_switch
        ),
        "diverging branch entry should force diverging switch route"
    )


func test_move_on_track_does_not_force_switch_before_entering_from_diverging_track() -> void:
    _register_track(
        _curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 14.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.1)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)


func test_move_on_track_does_not_force_switch_when_approaching_diverging_endpoint() -> void:
    var start_rid: RID = _register_track(
        _curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle(
        "start",
        TrackServer.track_get_length(start_rid) - 0.1
    )
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 0.05)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)


func test_move_on_track_forces_switch_common_when_entering_from_straight_branch() -> void:
    _register_track(
        _curve(Vector3(20.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 5.0)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)
    _assert_vector_eq(
        controller.get_world_position(),
        _track_position(switch_rid, TrackServer.track_get_length(switch_rid) - 3.0),
        "straight branch entry should force common switch route"
    )


func test_switch_change_does_not_move_vehicle_already_on_switch() -> void:
    _register_track(
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 5.0)
    var position_before_switch_change: Vector3 = controller.get_world_position()

    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)
    _assert_vector_eq(
        controller.get_world_position(),
        position_before_switch_change,
        "vehicle already on switch should stay on its current branch after switch state change"
    )
    _assert_vector_eq(
        vehicle.global_position,
        position_before_switch_change,
        "vehicle transform should not jump to another switch branch"
    )


func test_switch_change_then_reverse_keeps_vehicle_on_occupied_diverging_branch() -> void:
    _register_track(
        _curve(Vector3(-10.0, 0.0, 0.0), Vector3(0.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle("start", 8.0)
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 5.0)

    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)
    RailVehicleServer.vehicle_move(vehicle.get_rid(), -1.0)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)
    _assert_vector_eq(
        controller.get_world_position(),
        _switch_track_position(switch_rid, 2.0, TrackServer.TRACK_DIVERGING),
        "reversing after a switch change should keep the vehicle on the occupied diverging branch"
    )
    _assert_vector_eq(
        vehicle.global_position,
        _switch_track_position(switch_rid, 2.0, TrackServer.TRACK_DIVERGING),
        "vehicle transform should stay on the occupied diverging branch while reversing"
    )


func test_reversing_from_diverging_track_forces_switch_at_blade_boundary() -> void:
    var start_rid: RID = _register_track(
        _curve(Vector3(20.0, 0.0, 20.0), Vector3(10.0, 0.0, 10.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var switch_rid: RID = _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 10.0)),
        TrackServer.TRACK_SWITCH
    )
    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_DIVERGING)
    TrackServer.topology_rebuild()
    var start_length: float = TrackServer.track_get_length(start_rid)
    var boundary_margin: float = 0.02
    var fixture: Dictionary = await _create_vehicle(
        "start",
        start_length - boundary_margin,
        TrackServer.DIRECTION_NORMAL
    )
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    TrackServer.switch_set_active_track(switch_rid, TrackServer.TRACK_COMMON)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)

    RailVehicleServer.vehicle_move(vehicle.get_rid(), -0.01)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_COMMON)

    RailVehicleServer.vehicle_move(vehicle.get_rid(), -0.02)

    assert_eq(TrackServer.switch_get_active_track(switch_rid), TrackServer.TRACK_DIVERGING)


func test_move_on_track_forces_demo3d_second_switch_when_entering_from_diverging2() -> void:
    var topology: Dictionary = _register_demo3d_track_topology()
    var second_switch_rid: RID = topology["second_switch_rid"]
    TrackServer.switch_set_active_track(second_switch_rid, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle(
        "diverging2",
        TrackServer.track_get_length(TrackServer.track_get_rid_by_name("diverging2")) - 1.0
    )
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    RailVehicleServer.vehicle_move(vehicle.get_rid(), 2.0)

    assert_eq(TrackServer.switch_get_active_track(second_switch_rid), TrackServer.TRACK_DIVERGING)


func test_move_on_track_continues_after_entering_demo3d_second_switch() -> void:
    var topology: Dictionary = _register_demo3d_track_topology()
    var first_switch_rid: RID = topology["first_switch_rid"]
    var second_switch_rid: RID = topology["second_switch_rid"]
    TrackServer.switch_set_active_track(first_switch_rid, TrackServer.TRACK_DIVERGING)
    TrackServer.switch_set_active_track(second_switch_rid, TrackServer.TRACK_COMMON)
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle(
        "start",
        12.0,
        TrackServer.DIRECTION_NORMAL
    )
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]
    var second_switch_offset: float = 2.0
    var distance_to_second_switch: float = (
        12.0 + TrackServer.track_get_length(first_switch_rid, TrackServer.TRACK_DIVERGING) + second_switch_offset
    )

    RailVehicleServer.vehicle_move(vehicle.get_rid(), distance_to_second_switch)

    var second_switch_position: Vector3 = _track_position(second_switch_rid, second_switch_offset)
    _assert_vector_eq(
        controller.get_world_position(),
        second_switch_position,
        "controller should enter the second demo3d switch"
    )

    var position_before_followup_move: Vector3 = controller.get_world_position()
    RailVehicleServer.vehicle_move(vehicle.get_rid(), 10.0)

    assert_true(
        controller.get_world_position().distance_to(position_before_followup_move) > 1.0,
        "controller should keep moving after entering the second demo3d switch"
    )


func test_track_transform_applies_roll_and_direction_rotation() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0), 0.0, 10.0),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle(
        "start",
        5.0,
        TrackServer.DIRECTION_NORMAL
    )
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var aligned_forward: Vector3 = -vehicle.global_basis.z.normalized()
    var aligned_up: Vector3 = vehicle.global_basis.y.normalized()

    vehicle.start_direction = TrackServer.DIRECTION_REVERSED
    var opposite_forward: Vector3 = -vehicle.global_basis.z.normalized()

    _assert_vector_eq(aligned_forward, Vector3.RIGHT, "aligned vehicle forward should follow track tangent")
    assert_true(aligned_up.distance_to(Vector3.UP) > 0.01, "roll should tilt vehicle up vector")
    _assert_vector_eq(opposite_forward, Vector3.LEFT, "opposite direction should rotate vehicle by 180 degrees")


func test_start_track_properties_reapply_vehicle_track() -> void:
    _register_track(
        _curve(Vector3(0.0, 0.0, 0.0), Vector3(10.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    _register_track(
        _curve(Vector3(10.0, 0.0, 0.0), Vector3(20.0, 0.0, 0.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "next"
    )
    TrackServer.topology_rebuild()
    var fixture: Dictionary = await _create_vehicle(
        "start",
        2.0,
        TrackServer.DIRECTION_NORMAL
    )
    var vehicle: RailVehicle3D = fixture["vehicle"]
    var controller: VehicleController = fixture["controller"]

    vehicle.start_track_offset = 6.0
    await step(2)
    _assert_vector_eq(controller.get_world_position(), _rail_position(6.0, 0.0), "offset change should reapply placement")

    vehicle.set("start_direction", TrackServer.DIRECTION_REVERSED)
    await step(2)
    _assert_vector_eq(_vehicle_forward(vehicle), Vector3.LEFT, "direction change should reapply orientation")

    vehicle.start_track_name = "next"
    await step(2)
    _assert_vector_eq(controller.get_world_position(), _rail_position(16.0, 0.0), "track name change should reapply placement on the new track")


func _create_vehicle(
    track_name: String,
    offset: float,
    direction: TrackServer.Direction = TrackServer.DIRECTION_REVERSED,
) -> Dictionary:
    var physics_node: VehiclePhysicsNode = _create_vehicle_node()
    var vehicle: RailVehicle3D = RailVehicle3D.new()
    vehicle.start_track_name = track_name
    vehicle.start_track_offset = offset
    vehicle.start_direction = direction
    vehicle.controller_path = NodePath("../%s" % physics_node.name)
    add_child(vehicle)
    created_vehicles.append(vehicle)
    await step(2)
    return {
        "vehicle": vehicle,
        "controller": VehicleServer.vehicle_get_controller(physics_node.get_vehicle_rid()),
    }


## The vehicle's node, because RailVehicle3D is pointed at it by path - the controller it owns is
## not a node and has none.
func _create_vehicle_node() -> VehiclePhysicsNode:
    var physics_node: VehiclePhysicsNode = build_vehicle_node(
            "test_train_%d" % created_vehicle_nodes.size())
    created_vehicle_nodes.append(physics_node)
    return physics_node


func _register_track(
    curve1: TrackCurve,
    curve2: TrackCurve = null,
    type: int = TrackServer.TRACK_NORMAL,
    name: String = "",
) -> RID:
    var track_rid: RID = TrackServer.track_create()
    created_tracks.append(track_rid)
    TrackServer.track_update_curves(track_rid, curve1, curve2)
    TrackServer.track_update(track_rid, type, name, 1.435)
    return track_rid


func _register_demo3d_track_topology() -> Dictionary:
    _register_track(_demo_curve(Vector3(-25.8, 0.0, -136.0), Vector3(-25.8, 0.0, 136.0)))
    _register_track(
        _demo_curve(Vector3(-19.19, 0.0, -100.0), Vector3(-19.19, 0.0, 70.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "diverging"
    )
    _register_track(
        _demo_curve(Vector3(-25.8, 0.0, 236.0), Vector3(-25.8, 0.0, 536.0)),
        null,
        TrackServer.TRACK_NORMAL,
        "start"
    )
    var first_switch_rid: RID = _register_track(
        _demo_curve(Vector3(-25.8, 0.0, 236.0), Vector3(-25.8, 0.0, 136.0)),
        _demo_curve(
            Vector3(-25.8, 0.0, 236.0),
            Vector3(-19.19, 0.0, 130.0),
            Vector3(-0.305, 0.0, -21.41),
            Vector3(0.305, 0.0, 20.0)
        ),
        TrackServer.TRACK_SWITCH
    )
    var second_switch_rid: RID = _register_track(
        _demo_curve(Vector3(-19.19, 0.0, 130.0), Vector3(-19.19, 0.0, 70.0)),
        _demo_curve(
            Vector3(-19.19, 0.0, 130.0),
            Vector3(-12.53, 0.0, 80.0),
            Vector3(-0.513, 0.0, -8.065),
            Vector3(-0.823, 0.0, 6.27)
        ),
        TrackServer.TRACK_SWITCH
    )
    _register_track(
        _demo_curve(
            Vector3(-12.53, 0.0, -100.0),
            Vector3(-12.53, 0.0, 80.0),
            Vector3.ZERO,
            Vector3(1.61, 0.0, -3.35)
        ),
        null,
        TrackServer.TRACK_NORMAL,
        "diverging2"
    )
    return {
        "first_switch_rid": first_switch_rid,
        "second_switch_rid": second_switch_rid,
    }


func _curve(
    p1: Vector3,
    p2: Vector3,
    roll1: float = 0.0,
    roll2: float = 0.0,
) -> TrackCurve:
    var curve: TrackCurve = TrackCurve.new()
    curve.p1 = p1
    curve.p2 = p2
    curve.roll1 = roll1
    curve.roll2 = roll2
    return curve


func _demo_curve(
    p1: Vector3,
    p2: Vector3,
    c1: Vector3 = Vector3.ZERO,
    c2: Vector3 = Vector3.ZERO,
) -> TrackCurve:
    var curve: TrackCurve = _curve(p1, p2)
    curve.c1 = c1
    curve.c2 = c2
    return curve


func _assert_float_eq(actual: float, expected: float, message: String = "") -> void:
    assert_true(absf(actual - expected) <= 0.01, "%s expected %s, got %s" % [message, expected, actual])


func _rail_position(x: float, z: float) -> Vector3:
    return Vector3(x, TrackServer.rail_height, z)


func _track_position(track_rid: RID, offset: float) -> Vector3:
    var vehicle_rid: RID = VehicleServer.vehicle_create()
    RailVehicleServer.vehicle_attach(vehicle_rid)
    RailVehicleServer.vehicle_set_track(
        vehicle_rid,
        track_rid,
        offset,
        TrackServer.DIRECTION_NORMAL
    )
    var transform: Transform3D = RailVehicleServer.vehicle_get_transform(vehicle_rid)
    VehicleServer.vehicle_free(vehicle_rid)
    return transform.origin


func _switch_track_position(track_rid: RID, offset: float, switch_track: int) -> Vector3:
    var previous_active_track: int = TrackServer.switch_get_active_track(track_rid)
    TrackServer.switch_set_active_track(track_rid, switch_track)
    var position: Vector3 = _track_position(track_rid, offset)
    TrackServer.switch_set_active_track(track_rid, previous_active_track)
    return position

func _vehicle_forward(vehicle: RailVehicle3D) -> Vector3:
    return -vehicle.global_basis.z.normalized()


func _assert_vector_eq(actual: Vector3, expected: Vector3, message: String = "") -> void:
    assert_true(actual.distance_to(expected) <= 0.02, "%s expected %s, got %s" % [message, expected, actual])
