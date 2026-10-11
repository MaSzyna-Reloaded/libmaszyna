extends MaszynaGutTest

## A vehicle whose MMD declares cab0definition: without a cab0model: (SU46, dynamic/pkp/su46_v2,
## does) - the original then has no hi-fi cab (Train.cpp:8692, mdKabina stays nullptr) and keeps
## every low-poly "cabN" submodel visible (DynObj.cpp:1214), so the machine room is the low-poly
## interior's cab0. The fabricated vehicle (demo/tests/fixtures/dynamic/test/synthetic_v1) has a
## hi-fi cab 1 and a machine room like that.

const PLAYER_SCENE:PackedScene = preload("res://addons/libmaszyna/player/player.tscn")

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
## cab0, cab1 and cab2 of the low-poly interior
const LOW_POLY_CABS:int = 3

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


func _low_poly_cab_visible(cab_index:int) -> bool:
    # the low-poly interior is drawn as nodes under the vehicle with its exterior
    var cab_node:Node3D = vehicle.find_child("cab%d" % cab_index, true, false) as Node3D
    assert_not_null(cab_node, "the low-poly interior should contain cab%d" % cab_index)
    return cab_node.visible if cab_node else false


func test_machine_room_without_cab_model_shows_low_poly_interior() -> void:
    vehicle = await spawn_maszyna_vehicle("dynamic/test/synthetic_v1", "synthetic", "", "test_machine_room",
            MaszynaDynamicData.DriverType.DRIVER_HEAD)
    if not await wait_detailed(vehicle):
        return
    var controller:VehicleController = vehicle.get_controller()
    assert_not_null(controller, "the vehicle's FIZ controller should be built")
    if not controller:
        return

    player = PLAYER_SCENE.instantiate()
    add_child(player)
    PlayerServer.player_take_over_vehicle(vehicle.get_rid())
    await wait_idle_frames(3)
    assert_false(_low_poly_cab_visible(1), "hi-fi cab 1 hides its low-poly counterpart")

    RailVehicleServer.person_change_cabin(PlayerServer.player_get_person(), RailVehicleServer.CABIN_CHANGE_BACKWARD)
    await wait_idle_frames(3)

    var cabin:Cabin3D = get_viewport().get_camera_3d().get_parent() as Cabin3D
    assert_not_null(cabin, "camera should stay in the cabin in the machine room")
    if not cabin:
        return
    assert_eq(RailVehicleServer.cabin_get_kind(cabin.get_cabin()), RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE)
    assert_false(cabin.has_cab_model, "cab0definition: has no cab0model:")
    for cab_index:int in range(LOW_POLY_CABS):
        assert_true(_low_poly_cab_visible(cab_index), "low-poly cab%d should be visible in the machine room" % cab_index)
