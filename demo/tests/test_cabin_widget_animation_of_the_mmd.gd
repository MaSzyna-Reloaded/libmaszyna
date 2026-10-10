extends MaszynaGutTest

## What the MMD says about a control's movement reaches every widget (TGauge, Gauge.cpp): a gauge
## that slides ("mov", gt_Move, Gauge.cpp:466-469) and a control without friction, which moves at
## once (Gauge.cpp:364-375).

const FULL_SHIFT:float = 0.2
## How often a switch takes its target (_process_tool(), cabin_switch.gd)
const SWITCH_REFRESH_SECONDS:float = 0.05


func test_a_sliding_gauge_moves_its_mesh() -> void:
    var bar:MeshInstance3D = add_child_autofree(MeshInstance3D.new())
    var gauge:CabinGauge = CabinGauge.new()
    gauge.mesh_position = Vector3(0.0, 0.0, FULL_SHIFT)
    gauge.max_value = 1.0
    gauge.animation_speed = 0.0
    add_child_autofree(gauge)
    gauge.target_mesh_path = gauge.get_path_to(bar)
    gauge.value = 0.5
    await wait_idle_frames(3)
    assert_almost_eq(bar.position.z, FULL_SHIFT * 0.5, 0.001)


func test_a_switch_without_friction_moves_at_once() -> void:
    var lever:MeshInstance3D = add_child_autofree(MeshInstance3D.new())
    var switch:CabinSwitch = CabinSwitch.new()
    switch.switch_max_position = 2
    switch.mesh_position = Vector3(0.0, 0.0, FULL_SHIFT)
    switch.animation_speed = 0.0
    add_child_autofree(switch)
    switch.mesh_path = switch.get_path_to(lever)
    await wait_idle_frames(3)
    switch.switch_position = 2
    # the switch takes its target on its own refresh, and moves at once without friction
    if not await wait_simulated_until(func() -> bool: return is_equal_approx(lever.position.z, FULL_SHIFT * 2.0),
            SWITCH_REFRESH_SECONDS + TICK, "the lever at its target"):
        return
    assert_almost_eq(lever.position.z, FULL_SHIFT * 2.0, 0.001)
