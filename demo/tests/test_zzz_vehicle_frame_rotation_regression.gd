extends MaszynaGutTest

## Regression test for SM42 (and every MaSzyna-data vehicle) being drawn reversed. The original
## draws the exterior, low-poly interior, passengers and cab all under one vehicle-local frame
## with +Z = direction of travel (TDynamicObject::mMatrix, DynObj.cpp:2506-2508); RailVehicle3D
## uses Godot's -Z forward. MaszynaRailVehicle3DInstancer converts all of them with the same 180
## degree yaw (MASZYNA_VEHICLE_FRAME) - these tests spawn a fabricated vehicle whose front parts
## sit at the MaSzyna front (+Z, demo/tests/fixtures/dynamic/test/synthetic_v1) and check every
## part ends up in the same frame, facing the vehicle's own forward. The models are drawn as nodes
## under the vehicle (RailVehicleRenderingServer), found there by their submodels' names.

const PLAYER_SCENE:PackedScene = preload("res://addons/libmaszyna/player/player.tscn")

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## The driver sits in the exterior's cab shell, not just on its side of the vehicle [m]
const DRIVER_TO_CAB_SHELL_MAX_DISTANCE:float = 2.0
const FORWARD_MIN_DOT:float = 0.99
## How far the player walks and turns away from the seat before sitting back down [m], [rad]
const WALK_AWAY:Vector3 = Vector3(0.3, 0.0, 0.5)
const TURN_AWAY:float = 1.0

var _previous_game_dir:String
var vehicle:RailVehicle3D
var player:MaszynaPlayer


func before_each() -> void:
    _previous_game_dir = UserSettings.get_maszyna_game_dir()
    UserSettings.save_maszyna_game_dir(FIXTURES_GAME_DIR)


func after_each() -> void:
    # out of the cab before the vehicle goes: the player's cab camera is in it
    PlayerServer.player_leave_vehicle()
    # the cab interior is freed at the end of the frame
    await wait_idle_frames(1)
    if is_instance_valid(vehicle):
        vehicle.free()
    if is_instance_valid(player):
        player.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func _spawn_vehicle() -> bool:
    vehicle = await spawn_maszyna_vehicle("dynamic/test/synthetic_v1", "synthetic", "", "test_vehicle_frame")
    if not await wait_detailed(vehicle):
        return false
    var loaded:bool = RailVehicleRenderingServer.vehicle_get_model(vehicle.get_rid()).is_valid()
    assert_true(loaded, "the vehicle's exterior model should be built")
    return loaded


func _exterior_cab_z() -> float:
    var cab:Node3D = vehicle.find_child("budka_maszynisty", true, false) as Node3D
    assert_not_null(cab, "the exterior model should contain its cab shell (budka_maszynisty)")
    return vehicle.to_local(cab.global_position).z if cab else 0.0


func test_exterior_nose_faces_vehicle_forward() -> void:
    if not await _spawn_vehicle():
        return
    var nose:Node3D = vehicle.find_child("nos01", true, false) as Node3D
    assert_not_null(nose, "the exterior model should contain a nos01 submodel")
    if not nose:
        return
    var nose_local:Vector3 = vehicle.to_local(nose.global_position)
    assert_true(nose_local.z < 0.0, "nos01 should sit on the forward (-Z) side of the vehicle, got %s" % nose_local)


func test_low_poly_interior_shares_exterior_frame() -> void:
    if not await _spawn_vehicle():
        return
    var cab_mesh:MeshInstance3D = vehicle.find_child("cab1", true, false) as MeshInstance3D
    assert_not_null(cab_mesh, "the low-poly interior should contain cab1")
    if not cab_mesh or not cab_mesh.mesh:
        return
    var cab_center:Vector3 = vehicle.to_local(cab_mesh.global_transform * cab_mesh.mesh.get_aabb().get_center())
    var exterior_cab_z:float = _exterior_cab_z()
    assert_true(
        signf(cab_center.z) == signf(exterior_cab_z),
        "low-poly cab (z=%s) should sit inside the exterior's cab shell (z=%s)" % [cab_center.z, exterior_cab_z],
    )


func test_cabin_camera_sits_in_exterior_cab_and_looks_forward() -> void:
    if not await _spawn_vehicle():
        return
    assert_not_null(vehicle.get_controller(), "the vehicle's FIZ controller should be built")
    if not vehicle.get_controller():
        return

    player = PLAYER_SCENE.instantiate()
    add_child(player)
    PlayerServer.player_take_over_vehicle(vehicle.get_rid())
    await wait_idle_frames(3)

    var camera:FreeCamera3D = get_viewport().get_camera_3d() as FreeCamera3D
    assert_true(camera.get_parent() is Cabin3D, "camera should have moved into the cabin")
    var camera_local:Vector3 = vehicle.to_local(camera.global_position)
    var exterior_cab_z:float = _exterior_cab_z()
    assert_true(
        signf(camera_local.z) == signf(exterior_cab_z)
                and absf(camera_local.z - exterior_cab_z) < DRIVER_TO_CAB_SHELL_MAX_DISTANCE,
        "driver camera (z=%s) should sit inside the exterior's cab shell (z=%s)" % [camera_local.z, exterior_cab_z],
    )
    var camera_forward:Vector3 = -camera.global_basis.z
    var vehicle_forward:Vector3 = -vehicle.global_basis.z
    assert_true(
        camera_forward.dot(vehicle_forward) > FORWARD_MIN_DOT,
        "driver camera should look along the vehicle's forward, got %s vs %s" % [camera_forward, vehicle_forward],
    )


## drivermode.cpp:541 CabView() - Ctrl and the right button sit the driver back down, after walking
## and looking around the cab
func test_ctrl_and_the_right_button_sit_the_driver_back_down() -> void:
    if not await _spawn_vehicle():
        return
    player = PLAYER_SCENE.instantiate()
    add_child(player)
    PlayerServer.player_take_over_vehicle(vehicle.get_rid())
    await wait_idle_frames(3)
    var camera:FreeCamera3D = get_viewport().get_camera_3d() as FreeCamera3D
    var seat:Transform3D = camera.global_transform
    camera.position += WALK_AWAY
    camera.rotate_y(TURN_AWAY)

    var click:InputEventMouseButton = InputEventMouseButton.new()
    click.button_index = MOUSE_BUTTON_RIGHT
    click.ctrl_pressed = true
    click.pressed = true
    Input.parse_input_event(click)
    await wait_idle_frames(2)

    assert_true(camera.global_transform.is_equal_approx(seat),
            "the camera should be back at the seat: %s vs %s" % [camera.global_transform, seat])
    assert_eq(Input.mouse_mode, Input.MOUSE_MODE_VISIBLE, "Ctrl with the right button should not start the look")
    click.pressed = false
    Input.parse_input_event(click)
