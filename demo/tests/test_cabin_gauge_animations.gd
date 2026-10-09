extends MaszynaGutTest

## The gauge animations of the MMD beside rot and mov (TGauge, Gauge.cpp:448-497): a counter's digit
## drums (dgt), the two submodels a wiper turns with it (wip) and a scale running to its end scale
## (rotvar/movvar).

const TOLERANCE:float = 0.001


func _mesh_with_children(names:PackedStringArray) -> Node3D:
    var mesh := Node3D.new()
    mesh.name = "Gauge"
    for child_name:String in names:
        var child := Node3D.new()
        child.name = child_name
        mesh.add_child(child)
    return mesh


func _yaw_degrees(node:Node3D) -> float:
    return rad_to_deg(node.basis.get_euler().y)


func _gauge(mesh:Node3D) -> CabinGauge:
    var root := Node3D.new()
    add_child_autofree(root)
    root.add_child(mesh)
    var gauge := CabinGauge.new()
    root.add_child(gauge)
    gauge.animation_speed = 0.0
    gauge.target_mesh_path = gauge.get_path_to(mesh)
    return gauge


func test_a_counter_turns_each_drum_to_its_digit() -> void:
    var mesh:Node3D = _mesh_with_children(["0", "1", "2", "frame"])
    var gauge:CabinGauge = _gauge(mesh)
    gauge.animation_type = CabinGauge.AnimationType.DIGITAL
    gauge.digital_scale = 1.0
    gauge.digital_offset = 0.0
    gauge.value = 472.0
    await wait_idle_frames(3)
    assert_almost_eq(_yaw_degrees(mesh.get_node("0")), CabinGauge.DIGIT_ANGLE * 2.0, TOLERANCE, "units")
    assert_almost_eq(_yaw_degrees(mesh.get_node("1")), CabinGauge.DIGIT_ANGLE * 7.0 + 360.0, TOLERANCE, "tens")
    assert_almost_eq(_yaw_degrees(mesh.get_node("2")), CabinGauge.DIGIT_ANGLE * 4.0, TOLERANCE, "hundreds")
    assert_almost_eq(_yaw_degrees(mesh.get_node("frame")), 0.0, TOLERANCE, "no digit, no drum")


func test_a_wiper_turns_two_submodels_below_it() -> void:
    var mesh:Node3D = _mesh_with_children(["arm"])
    var blade := Node3D.new()
    blade.name = "blade"
    mesh.get_node("arm").add_child(blade)
    var gauge:CabinGauge = _gauge(mesh)
    gauge.wiper_chain = true
    gauge.mesh_rotation = Vector3(0.0, 30.0, 0.0)
    gauge.value = 1.0
    await wait_idle_frames(3)
    assert_almost_eq(_yaw_degrees(mesh), 30.0, TOLERANCE)
    assert_almost_eq(_yaw_degrees(mesh.get_node("arm")), 30.0, TOLERANCE, "its first submodel")
    assert_almost_eq(_yaw_degrees(blade), 30.0, TOLERANCE, "and that one's")


func test_a_variable_scale_runs_to_its_end_scale() -> void:
    var mesh:Node3D = _mesh_with_children([])
    var gauge:CabinGauge = _gauge(mesh)
    gauge.mesh_rotation = Vector3(0.0, 1.0, 0.0)
    gauge.variable_end_value = 100.0
    gauge.variable_end_scale = 0.5
    gauge.value = 100.0
    await wait_idle_frames(3)
    assert_almost_eq(_yaw_degrees(mesh), 50.0, TOLERANCE, "100 at half the scale")

