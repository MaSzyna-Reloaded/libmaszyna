extends MaszynaGutTest

## Cab switching on the EP07 (the EP07's own .fiz and .mmd, demo/tests/fixtures): a cab change
## backward moves the driver from cab 1 to the machine room (cab0definition:, Train.cpp:8908 - its own
## instruments up to the end of the MMD) and then to cab 2. The camera looks along VectorFront *
## CabOccupied (drivermode.cpp:1071), i.e. backward from cab 2. The fixtures carry no models, so
## the low-poly interior's cab visibility (DynObj.cpp:1211-1219) is not checked here.

const PLAYER_SCENE:PackedScene = preload("res://addons/libmaszyna/player/player.tscn")

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"

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
    await step(1)
    if is_instance_valid(vehicle):
        vehicle.free()
    if is_instance_valid(player):
        player.free()
    UserSettings.save_maszyna_game_dir(_previous_game_dir)


func test_cab_change_moves_camera_to_rear_cab_facing_backward() -> void:
    vehicle = await spawn_maszyna_vehicle("dynamic/pkp/303e_v1", "303e-ep-tv", "303e-ep-tv-424-hist", "test_ep07_cab_change",
            MaszynaDynamicData.DriverType.DRIVER_HEAD)
    var bound:bool = vehicle.get_rid().is_valid()
    assert_true(bound, "EP07's FIZ controller should be built")
    if not bound:
        return
    var vehicle_rid:RID = vehicle.get_rid()

    player = PLAYER_SCENE.instantiate()
    add_child(player)
    PlayerServer.player_take_over_vehicle(vehicle_rid)
    await step(3)

    var camera:FreeCamera3D = get_viewport().get_camera_3d() as FreeCamera3D
    var vehicle_forward:Vector3 = -vehicle.global_basis.z
    var cab1_z:float = vehicle.to_local(camera.global_position).z
    assert_true((-camera.global_basis.z).dot(vehicle_forward) > 0.99, "cab 1 camera should look forward")

    RailVehicleServer.person_change_cabin(PlayerServer.player_get_person(), RailVehicleServer.CABIN_CHANGE_BACKWARD)
    await step(3)

    var machine_room:MaszynaDynamicTrainCabin = camera.get_parent() as MaszynaDynamicTrainCabin
    assert_eq(RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(vehicle_rid)),
            RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE)
    assert_not_null(machine_room, "camera should stay in the cabin in the machine room")
    if not machine_room:
        return
    assert_eq(RailVehicleServer.cabin_get_kind(machine_room.get_cabin()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE,
            "cabin should be rebuilt as the machine room")
    assert_true(
        absf(vehicle.to_local(camera.global_position).z) < absf(cab1_z),
        "machine room camera should sit between the cabs",
    )
    for diagnostic:Dictionary in machine_room.get_diagnostics():
        assert_false(diagnostic["code"] == "MMD_INVALID_CAB_DEFINITION", "EP07 declares cab0definition:")

    RailVehicleServer.person_change_cabin(PlayerServer.player_get_person(), RailVehicleServer.CABIN_CHANGE_BACKWARD)
    await step(3)

    var cabin:Cabin3D = camera.get_parent() as Cabin3D
    assert_eq(RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(vehicle_rid)),
            RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR)
    assert_not_null(cabin, "camera should stay in the cabin after cab change")
    if not cabin:
        return
    assert_eq(RailVehicleServer.cabin_get_kind(cabin.get_cabin()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR,
            "cabin should be rebuilt as cab 2")
    var cab2_z:float = vehicle.to_local(camera.global_position).z
    assert_true(
        signf(cab2_z) == -signf(cab1_z),
        "cab 2 camera (z=%s) should sit at the other end than cab 1 (z=%s)" % [cab2_z, cab1_z],
    )
    assert_true((-camera.global_basis.z).dot(vehicle_forward) < -0.99, "cab 2 camera should look backward")
