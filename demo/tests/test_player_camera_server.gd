extends MaszynaGutTest

## PlayerCameraServer holds the player's view and announces it, by the vehicles' handles - the
## player's node that follows it is not here (test_maszyna_player.gd).

const TRACK_NAME:String = "camera_server_test"
const TRACK_LENGTH:float = 200.0
const TRACK_OFFSET:float = 100.0
## Enough frames for the vehicle to take its controller and stand on its track
const SETTLE_FRAMES:int = 4
## camera_show_vehicle() stands at least this far from the vehicle [m] (PlayerCameraServer.cpp)
const SHOW_MIN_DISTANCE:float = 12.0
## Cosine of the angle within which a camera counts as looking at something
const LOOKING_AT:float = 0.99

var _track:RID
var _vehicle:RailVehicle3D
## What PlayerCameraServer announced, in order
var _announced:Array[StringName] = []
var _placed:Transform3D


## An AI driver that does nothing of its own: only who sits at the controls is tested
class IdleDriver extends DriverImplementation:
    pass


func before_each() -> void:
    _announced.clear()
    _track = build_track(TRACK_NAME, TRACK_LENGTH)
    _vehicle = build_rail_vehicle("CameraServerTest", TRACK_NAME, TRACK_OFFSET)
    PlayerCameraServer.camera_changed.connect(_on_camera_changed)
    PlayerCameraServer.camera_placed.connect(_on_camera_placed)


func after_each() -> void:
    PlayerCameraServer.camera_changed.disconnect(_on_camera_changed)
    PlayerCameraServer.camera_placed.disconnect(_on_camera_placed)
    PlayerServer.player_leave_vehicle()
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FREE)
    PlayerCameraServer.camera_set_target(RID())
    PlayerCameraServer.camera_set_follow_view(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    if is_instance_valid(_vehicle):
        free_rail_vehicle(_vehicle)
    TrackServer.track_free(_track)
    TrackServer.topology_rebuild()


func _on_camera_changed() -> void:
    _announced.append(&"camera_changed")


func _on_camera_placed(transform:Transform3D) -> void:
    _announced.append(&"camera_placed")
    _placed = transform


func test_f4_on_foot_without_a_vehicle_keeps_walking() -> void:
    PlayerCameraServer.camera_toggle_cabin()

    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)
    assert_eq(_announced, [] as Array[StringName], "nothing changed, nothing announced")


func test_the_cab_view_needs_a_vehicle_driven() -> void:
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_CABIN)

    assert_engine_error("The player drives no vehicle")
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)


## drivermode.cpp:1244-1277 - F4 steps out of the cab and back; the player drives the vehicle
## meanwhile, and lets it go only through PlayerServer
func test_a_vehicle_taken_over_is_looked_at_from_its_cab_and_f4_steps_out_and_in() -> void:
    await wait_idle_frames(SETTLE_FRAMES)

    PlayerServer.player_take_over_vehicle(_vehicle.get_rid())
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_CABIN)
    PlayerCameraServer.camera_toggle_cabin()
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)
    assert_eq(PlayerServer.player_get_vehicle(), _vehicle.get_rid(), "outside, the vehicle is still driven")
    PlayerCameraServer.camera_toggle_cabin()
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_CABIN)

    PlayerServer.player_leave_vehicle()
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE, "let go, the player steps out")


## drivermode.cpp:258-267 - F5 on the vehicle already driven takes the player back into its cab and
## the vehicle back from its driver
func test_taking_over_the_vehicle_driven_goes_back_into_its_cab() -> void:
    await wait_idle_frames(SETTLE_FRAMES)
    # the vehicle's driver rides in its rear cabin
    var driver:RID = PersonServer.person_create()
    RailVehicleServer.person_enter_rear_cabin(driver, _vehicle.get_rid(), VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER)
    attach_driver_implementation(driver, IdleDriver.new())
    PlayerServer.player_take_over_vehicle(_vehicle.get_rid())
    PlayerCameraServer.camera_toggle_cabin()
    # Shift+Q: handed to its driver
    PlayerServer.player_hand_over_vehicle()
    assert_eq(VehicleServer.person_get_role(PlayerServer.player_get_person()),
            VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER, "handed over, the player rides along")

    PlayerServer.player_take_over_vehicle(_vehicle.get_rid())

    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_CABIN)
    assert_eq(PlayerServer.player_get_vehicle(), _vehicle.get_rid())
    assert_eq(VehicleServer.person_get_role(PlayerServer.player_get_person()),
            VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER, "taken back by the player")
    assert_false(DriverServer.vehicle_is_control_active(_vehicle.get_rid()), "the driver only rides along")
    PersonServer.person_free(driver)


## Train.cpp:1088-1118 - Q (aidriverdisable) takes the controls back from the driver and only that:
## the player looking from outside goes on looking from outside (report 2026-10-05: the view jumped
## into the cab)
func test_q_takes_the_controls_back_and_the_view_stays_outside() -> void:
    await wait_idle_frames(SETTLE_FRAMES)
    var driver:RID = PersonServer.person_create()
    RailVehicleServer.person_enter_rear_cabin(driver, _vehicle.get_rid(), VehiclePersonRole.VEHICLE_PERSON_ROLE_OBSERVER)
    attach_driver_implementation(driver, IdleDriver.new())
    PlayerServer.player_take_over_vehicle(_vehicle.get_rid())
    PlayerServer.player_hand_over_vehicle()
    # Shift+F4: looking at the vehicle from outside
    PlayerCameraServer.camera_cycle_follow_view()
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FOLLOW)

    PlayerServer.player_take_back_vehicle()

    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FOLLOW, "the view stays outside")
    assert_eq(VehicleServer.person_get_role(PlayerServer.player_get_person()),
            VehiclePersonRole.VEHICLE_PERSON_ROLE_DRIVER, "the player drives again")
    assert_false(DriverServer.vehicle_is_control_active(_vehicle.get_rid()), "the driver only rides along")
    PersonServer.person_free(driver)


## drivermode.cpp:803-804 - Shift+F4 follows the player's vehicle, then steps through the views
func test_shift_f4_follows_the_vehicle_driven_then_cycles_the_views() -> void:
    await wait_idle_frames(SETTLE_FRAMES)
    PlayerServer.player_take_over_vehicle(_vehicle.get_rid())

    PlayerCameraServer.camera_cycle_follow_view()
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FOLLOW)
    assert_eq(PlayerCameraServer.camera_get_target(), _vehicle.get_rid())
    assert_eq(PlayerCameraServer.camera_get_follow_view(), PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    PlayerCameraServer.camera_cycle_follow_view()
    assert_eq(PlayerCameraServer.camera_get_follow_view(), PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)
    PlayerCameraServer.camera_cycle_follow_view()
    PlayerCameraServer.camera_cycle_follow_view()
    PlayerCameraServer.camera_cycle_follow_view()
    assert_eq(PlayerCameraServer.camera_get_follow_view(), PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT,
            "after the last view, the first")

    PlayerCameraServer.camera_toggle_cabin()
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_CABIN,
            "F4 goes from following back into the cab")


## The vehicle card's crosshair: the free camera beside the vehicle, looking at it - placed before
## the view is announced, so nothing that follows the view moves it elsewhere
func test_show_vehicle_places_the_free_camera_beside_it_looking_at_it() -> void:
    await wait_idle_frames(SETTLE_FRAMES)
    PlayerCameraServer.camera_set_target(_vehicle.get_rid())
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FOLLOW)
    _announced.clear()

    PlayerCameraServer.camera_show_vehicle(_vehicle.get_rid())

    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)
    assert_eq(_announced, [&"camera_placed", &"camera_changed"] as Array[StringName])
    assert_eq(_placed, PlayerCameraServer.camera_get_show_transform(_vehicle.get_rid()),
            "where a far follow jumps to (MaszynaPlayer) as well")
    var body:Vector3 = _vehicle.global_position
    var aside:Vector3 = _placed.origin - body
    assert_true(Vector2(aside.x, aside.z).length() >= SHOW_MIN_DISTANCE, "beside the vehicle, not in it: %s" % aside)
    var towards:Vector3 = Vector3(-aside.x, 0.0, -aside.z).normalized()
    assert_true((-_placed.basis.z).dot(towards) > LOOKING_AT, "looking at the vehicle")


func test_f4_following_without_a_vehicle_driven_walks() -> void:
    await wait_idle_frames(SETTLE_FRAMES)
    PlayerCameraServer.camera_set_target(_vehicle.get_rid())
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FOLLOW)

    PlayerCameraServer.camera_toggle_cabin()

    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)


func test_a_freed_target_is_not_followed() -> void:
    await wait_idle_frames(SETTLE_FRAMES)
    PlayerCameraServer.camera_set_target(_vehicle.get_rid())
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FOLLOW)

    free_rail_vehicle(_vehicle)

    assert_eq(PlayerCameraServer.camera_get_target(), RID())
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_FREE)
