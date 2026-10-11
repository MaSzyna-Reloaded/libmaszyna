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


## TGauge::Update() (Gauge.cpp:364-376): no friction, or a step of half of it or more, sets an
## element outright; a shorter step moves it by dt / friction
func test_friction_weight_follows_the_original() -> void:
    assert_eq(BaseCabinTool3D.friction_weight(0.5, 0.0), 1.0, "no friction: at once")
    assert_eq(BaseCabinTool3D.friction_weight(0.1, 20.0), 1.0, "a step of twice the friction: at once")
    assert_almost_eq(BaseCabinTool3D.friction_weight(0.01, 20.0), 0.2, 0.000001, "a short step: dt / friction")


## The operator's spinning needles (Stary Jawor, SU45, 2026-10-09): a gauge of a small friction
## given one long step - a slow frame, a fast simulation - was thrown past its value by the step
## times the speed; it stands at the value
func test_a_long_step_never_throws_the_needle_past_its_value() -> void:
    gauge.animation_speed = 20.0
    gauge.value = 0.5
    SimulationServer.simulation_advance(0.25)
    await wait_idle_frames(2)

    assert_almost_eq(absf(_needle_angle()), 50.0, 0.01)
