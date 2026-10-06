extends MaszynaGutTest

## A cab button is a view of its control: it shows the vehicle's state and its mouse is the cab
## logic's (the keys are tested in test_legacy_cabin_keys.gd).

const SM42:VehicleController = preload("res://tests/fixtures/sm42_vehicle.tres")
const SHOWN_CONTROL:StringName = &"test_shown_control"
## Time constants of a widget's exponential approach (delta * animation_speed a frame, cabin_button.gd)
## to its pose: e^-15 of the way is left, below is_equal_approx's tolerance
const POSE_TIME_CONSTANTS:float = 15.0


## The vehicle's cab logic, registered for its driver's cabin, with one cab-only button, wired as
## LegacyCabinForwardCommands wires one - the widget's hand is the logic's press()
func _attach_logic(vehicle:RID) -> void:
    var controls:LegacyCabinControls = LegacyCabinControls.new()
    controls.add_control(SHOWN_CONTROL, CabinButton, {})
    CabinSystem.vehicle_attach_cab_logic(
            vehicle, LegacyCabinLogic.new(func(_cabin:RID) -> LegacyCabinControls: return controls))


## E186's vigilance pedal (pedal_sifa rot 0.008 -0.008) is modelled pushed: released it rests at
## the MMD offset, pushed it returns to the model's own pose.
func test_rotation_offset_is_the_released_pose():
    const ROTATION_DEGREES:float = 0.008 * 360.0
    var cab:Node3D = Node3D.new()
    var pedal:MeshInstance3D = MeshInstance3D.new()
    pedal.name = "Pedal"
    cab.add_child(pedal)
    var widget:CabinButton = CabinButton.new()
    widget.mesh_rotation = Vector3(0.0, ROTATION_DEGREES, 0.0)
    widget.mesh_rotation_offset = Vector3(0.0, -ROTATION_DEGREES, 0.0)
    cab.add_child(widget)
    widget.mesh_path = NodePath("../Pedal")
    add_child_autofree(cab)
    await wait_idle_frames(3)

    var released:Basis = Basis(Vector3.UP, deg_to_rad(-ROTATION_DEGREES))
    assert_true(pedal.transform.basis.is_equal_approx(released), "released pedal should rest at the offset")

    widget.pushed = true
    if not await wait_simulated_until(func() -> bool: return pedal.transform.basis.is_equal_approx(Basis.IDENTITY),
            POSE_TIME_CONSTANTS / widget.animation_speed, "the pushed pedal's pose"):
        return

    assert_true(pedal.transform.basis.is_equal_approx(Basis.IDENTITY), "pushed pedal should reach the modelled pose")

## FINDINGS.md 2026-09-29: a cab built on a running vehicle showed each button's state by setting
## `pushed`, which acted on the vehicle - it lowered 3E/1-42's pantograph and opened its line
## breaker. Showing the vehicle's state acts on nothing; only the hand does.
func test_showing_the_vehicle_state_does_not_act():
    var physics_node:VehiclePhysicsNode = build_vehicle_node("CabinButtonTest", SM42, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    # freed before the node it is driven by, as test_driver_route_table.gd does
    var vehicle_node:RailVehicle3D = RailVehicle3D.new()
    add_child(vehicle_node)
    vehicle_node.controller_path = vehicle_node.get_path_to(physics_node)
    await wait_idle_frames(2)
    var vehicle:RID = vehicle_node.get_rid()
    _attach_logic(vehicle)
    var cabin:RID = RailVehicleServer.vehicle_get_driver_cabin(vehicle)
    var widget:CabinButton = CabinButton.new()
    widget.control_id = SHOWN_CONTROL
    widget.state_property = "main_switch_enabled"
    widget.pushed = true
    add_child_autofree(widget)
    await wait_idle_frames(1)

    widget.set_vehicle_rid(vehicle)
    await wait_idle_frames(2)

    assert_false(widget.pushed, "the button shows the open line breaker")
    assert_null(CabinSystem.get_control(cabin, SHOWN_CONTROL), "and acts on nothing to show it")
    widget.press()
    assert_not_null(CabinSystem.get_control(cabin, SHOWN_CONTROL), "the hand acts")
    CabinSystem.vehicle_attach_cab_logic(vehicle, null)
    remove_child(vehicle_node)
    vehicle_node.queue_free()

## A cab rebuilt on a vehicle (the player back in it, the other cab occupied) builds its buttons
## anew. One with no vehicle state behind it - E186's universal1, the screen's pantograph page -
## showed itself off while the cab still held it on, and the first press did nothing.
func test_rebuilt_button_shows_what_the_cab_holds():
    var physics_node:VehiclePhysicsNode = build_vehicle_node("CabinButtonRebuilt", SM42, 0.0, MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var vehicle_node:RailVehicle3D = RailVehicle3D.new()
    add_child(vehicle_node)
    vehicle_node.controller_path = vehicle_node.get_path_to(physics_node)
    await wait_idle_frames(2)
    var vehicle:RID = vehicle_node.get_rid()
    _attach_logic(vehicle)
    var cabin:RID = RailVehicleServer.vehicle_get_driver_cabin(vehicle)
    # the buttons stand in the interior of the driver's cabin, as a built cab's do
    var interior:Cabin3D = Cabin3D.new()
    interior.set_cabin(cabin)
    add_child_autofree(interior)
    var first:CabinButton = CabinButton.new()
    first.control_id = SHOWN_CONTROL
    interior.add_child(first)
    first.set_vehicle_rid(vehicle)
    await wait_idle_frames(2)
    first.press()
    assert_true(CabinSystem.get_control(cabin, SHOWN_CONTROL), "the press is held by the cab")

    var rebuilt:CabinButton = CabinButton.new()
    rebuilt.control_id = SHOWN_CONTROL
    interior.add_child(rebuilt)
    rebuilt.set_vehicle_rid(vehicle)
    await wait_idle_frames(2)

    assert_true(rebuilt.pushed, "the rebuilt button shows the control on")
    rebuilt.press()
    assert_false(CabinSystem.get_control(cabin, SHOWN_CONTROL), "and the first press turns it off")
    CabinSystem.vehicle_attach_cab_logic(vehicle, null)
    remove_child(vehicle_node)
    vehicle_node.queue_free()
