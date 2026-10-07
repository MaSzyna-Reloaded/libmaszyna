extends MaszynaGutTest

## CabinGauge.animation_speed 0 jumps straight to the value (the Hasler needle, tachometer:);
## any other value keeps the smooth needle animation.

var gauge:CabinGauge
var needle:MeshInstance3D


func before_each() -> void:
    needle = MeshInstance3D.new()
    add_child(needle)
    gauge = CabinGauge.new()
    gauge.mesh_rotation = Vector3(0.0, 0.0, 100.0)
    gauge.max_value = 1.0
    add_child(gauge)
    gauge.target_mesh_path = gauge.get_path_to(needle)
    # the first non-zero value is always shown instantly (setup phase) - get past it
    gauge.value = 0.1
    await wait_idle_frames(2)


func after_each() -> void:
    gauge.free()
    needle.free()


func _needle_angle() -> float:
    return rad_to_deg(needle.basis.get_euler().z)


func test_zero_friction_jumps_to_value() -> void:
    gauge.animation_speed = 0.0
    gauge.value = 0.5
    await wait_idle_frames(2)

    assert_almost_eq(absf(_needle_angle()), 50.0, 0.01)


func test_friction_smooths_needle() -> void:
    gauge.animation_speed = 1.0
    gauge.value = 0.5
    await wait_idle_frames(2)

    assert_true(absf(_needle_angle()) < 49.0, "needle should still be moving, got %s" % _needle_angle())
