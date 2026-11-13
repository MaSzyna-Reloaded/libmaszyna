extends MaszynaGutTest

## The external views (libreloaded's FollowView) on a trainset: the vehicle view and the bogie view
## stand at whichever vehicle is chosen, looking the way the trainset goes; the rear view stands at
## the trainset's last vehicle looking at its rear, goes to a vehicle chosen keeping its offsets and
## to the last vehicle again from the cab; the drive-by point stands while the vehicle passes and is
## set ahead of it once it is far; the views keep their offsets while the player stays in one
## trainset. The views of one vehicle alone are libreloaded's tests.

const FIXTURES_GAME_DIR:String = "res://tests/fixtures"
const PLAYER_SCENE:PackedScene = preload("res://addons/libreloaded/player/player.tscn")
## A loco and three wagons on a straight track
const SCENERY:String = "trainsets_repeated_ids.scn"
## Frames for the view to settle where its offsets put it
const SETTLE_FRAMES:int = 60
## How fast the view follows in the tests (1/s), so it settles within SETTLE_FRAMES
const RESPONSE:float = 50.0
const ZOOM_STEPS:int = 10
## The farthest a view goes from what it looks at, in the drive-by test, and how far the wagons are
## moved back from the drive-by camera, away from the loco: first within it, then beyond it [m]
const DRIVEBY_MAX_DISTANCE:float = 80.0
## Mouse motions aside the drive-by camera is given
const WALK_FRAMES:int = 60
## A jump of the drive-by camera in one frame, no arrow moves it as far: it was set again [m]
const SET_AGAIN_JUMP:float = 5.0
## The most a camera moved aside may go deeper, as the share of the way it went
const ASIDE_SHARE:float = 0.01
## Frames a key is held to set the drive-by camera elsewhere
const SET_FRAMES:int = 10
const DRIVEBY_NEAR_MOVE:float = -20.0
const DRIVEBY_FAR_MOVE:float = -40.0
## How fast the view follows in the game (1/s), FollowView's own, for a flight to be watched
const GAME_RESPONSE:float = 2.0
## Frames a flight at GAME_RESPONSE is watched
const FLIGHT_FRAMES:int = 30
## Frames the right arrow is held, moving the view aside
const KEY_FRAMES:int = 30
## Mouse motion of one orbit step [px], and the steps of a small orbit
const ORBIT_STEP_PX:float = 20.0
const SMALL_ORBIT_STEPS:int = 3
## The most seconds the arrow is held with Shift to slide the view along the trainset to its last vehicle
const SLIDE_SECONDS:float = 10.0
## How much nearer the first vehicle a small orbit may take the view slid away from it, at most, and
## how far the point of the trainset it aims at may go [m]
const PULL_BACK:float = 1.0
## How far the loco drives each frame, and for how many frames [m]
const DRIVE_STEP:float = 0.5
const DRIVE_FRAMES:int = 120
## The most the view may turn from one frame to the next while it flies to another vehicle [rad]
const FLIGHT_FRAME_TURN:float = deg_to_rad(2.0)
## Distances and angles equal to this [m, rad]
const TOLERANCE:float = 0.05

var _previous_game_dir:String = ""
var _scenery:MaszynaSceneryNode
var _loco:RID
var _wagons:Array[RID] = []
var _view:FollowView


func before_all() -> void:
    _previous_game_dir = MaszynaGameDirectory.get_game_dir()
    MaszynaGameDirectory.save_game_dir(FIXTURES_GAME_DIR)
    _scenery = MaszynaSceneryNode.new()
    _scenery.autoload = false
    add_child(_scenery)
    _scenery.filename = SCENERY
    await _scenery.load()
    var vehicles:Array[RID] = []
    vehicles.assign(RailVehicleServer.trainset_get_vehicles(_scenery.get_trainsets()[0]))
    _loco = vehicles[0]
    _wagons.assign(vehicles.slice(1))
    # the loco a trainset of its own, the three wagons another one, headed by the first of them
    (VehicleServer.vehicle_get_controller(_loco) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)


func after_all() -> void:
    _scenery.free()
    MaszynaGameDirectory.save_game_dir(_previous_game_dir)
    super.after_all()


func before_each() -> void:
    _view = FollowView.new()
    _view.response = RESPONSE
    add_child(_view)


func after_each() -> void:
    PlayerServer.player_leave_vehicle()
    _view.free()


func _follow(p_vehicle:RID, p_view:PlayerCameraServer.CameraFollowView) -> void:
    _view.view = p_view
    _view.activate(p_vehicle, _view.global_transform)
    await wait_idle_frames(SETTLE_FRAMES)


func _origin(p_vehicle:RID) -> Vector3:
    return VehicleServer.vehicle_get_transform(p_vehicle).origin


# where the view stands and looks in the vehicle's own frame
func _local_view(p_vehicle:RID) -> Transform3D:
    return VehicleServer.vehicle_get_transform(p_vehicle).affine_inverse() * _view.global_transform


func _assert_same_local_view(p_view:Transform3D, p_expected:Transform3D, p_what:String) -> void:
    assert_almost_eq(p_view.origin.distance_to(p_expected.origin), 0.0, TOLERANCE, "%s: where it stands" % p_what)
    assert_almost_eq((-p_view.basis.z).angle_to(-p_expected.basis.z), 0.0, TOLERANCE, "%s: where it looks" % p_what)


# the vehicle nearest the view
func _nearest() -> RID:
    var nearest:RID = _loco
    for wagon:RID in _wagons:
        if _view.global_position.distance_to(_origin(wagon)) < _view.global_position.distance_to(_origin(nearest)):
            nearest = wagon
    return nearest


# the way from the wagons' last to their head
func _to_head() -> Vector3:
    return (_origin(_wagons[0]) - _origin(_wagons[_wagons.size() - 1])).normalized()


func _wheel(p_steps:int) -> void:
    for _step:int in p_steps:
        var event:InputEventMouseButton = InputEventMouseButton.new()
        event.button_index = MOUSE_BUTTON_WHEEL_DOWN
        event.pressed = true
        Input.parse_input_event(event)
        await wait_idle_frames(1)


func test_the_vehicle_view_stands_at_each_vehicle_chosen_looking_the_way_the_trainset_goes() -> void:
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var first:Transform3D = _local_view(_wagons[0])
    assert_gt((-_view.global_transform.basis.z).dot(_to_head()), 0.0, "it looks to the head")

    for wagon:RID in _wagons:
        await _follow(wagon, PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
        assert_eq(_nearest(), wagon, "it stands at the vehicle chosen")
        _assert_same_local_view(_local_view(wagon), first, "as at the first one")


func test_the_vehicle_view_keeps_its_zoom_on_another_vehicle_chosen() -> void:
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _wheel(ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    var zoomed:Transform3D = _local_view(_wagons[0])

    await _follow(_wagons[_wagons.size() - 1], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)

    _assert_same_local_view(_local_view(_wagons[_wagons.size() - 1]), zoomed, "the last vehicle")


func test_choosing_another_vehicle_flies_the_view_there_without_a_jump() -> void:
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    # moved aside as well: the move goes with the view to the vehicle chosen
    var event:InputEventKey = InputEventKey.new()
    event.physical_keycode = KEY_RIGHT
    event.pressed = true
    Input.parse_input_event(event)
    await wait_idle_frames(KEY_FRAMES)
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await wait_idle_frames(SETTLE_FRAMES)
    _view.response = GAME_RESPONSE

    _view.activate(_wagons[_wagons.size() - 1], _view.global_transform)
    var steps:Array[float] = []
    for frame:int in FLIGHT_FRAMES:
        var forward:Vector3 = -_view.global_transform.basis.z
        var position:Vector3 = _view.global_position
        await wait_idle_frames(1)
        assert_lt(forward.angle_to(-_view.global_transform.basis.z), FLIGHT_FRAME_TURN,
                "frame %d turns smoothly" % frame)
        steps.append(position.distance_to(_view.global_position))
    assert_lt(steps[0], steps[1] + TOLERANCE, "the first frame moves no farther than the next")


# how far along the trainset, from its first vehicle, the middle of the screen aims: the point of
# the line through the vehicles nearest the line the view looks along
func _aimed() -> float:
    var start:Vector3 = _origin(_wagons[0])
    var along:Vector3 = (_origin(_wagons[_wagons.size() - 1]) - start).normalized()
    var forward:Vector3 = -_view.global_transform.basis.z
    var to_start:Vector3 = start - _view.global_position
    var across:float = 1.0 - pow(along.dot(forward), 2.0)
    return (along.dot(forward) * forward.dot(to_start) - along.dot(to_start)) / across


# the vehicle nearest the middle of the screen
func _in_the_middle() -> RID:
    var forward:Vector3 = -_view.global_transform.basis.z
    var middle:RID = _loco
    for wagon:RID in _wagons:
        var to_wagon:Vector3 = _origin(wagon) - _view.global_position
        if forward.angle_to(to_wagon) < forward.angle_to(_origin(middle) - _view.global_position):
            middle = wagon
    return middle


func _press(p_key:Key, p_pressed:bool) -> void:
    var event:InputEventKey = InputEventKey.new()
    event.physical_keycode = p_key
    event.keycode = p_key
    event.pressed = p_pressed
    Input.parse_input_event(event)


# the mouse going right for p_steps, left for less than none
func _orbit(p_steps:int) -> void:
    for _step:int in absi(p_steps):
        var event:InputEventMouseMotion = InputEventMouseMotion.new()
        event.relative = Vector2(ORBIT_STEP_PX * signi(p_steps), 0.0)
        Input.parse_input_event(event)
        await wait_idle_frames(1)


func test_slid_along_the_trainset_an_orbit_keeps_the_view_at_the_vehicle_slid_to() -> void:
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE)
    var grab:InputEventMouseButton = InputEventMouseButton.new()
    grab.button_index = MOUSE_BUTTON_RIGHT
    grab.pressed = true
    Input.parse_input_event(grab)
    await wait_idle_frames(1)
    if not Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
        pending("the display server cannot capture the mouse")
        return
    # across the trainset - the mouse going right turns the view to the left - then along it to its
    # last vehicle
    var along:Vector3 = (_origin(_wagons[_wagons.size() - 1]) - _origin(_wagons[0])).normalized()
    var to_track:Vector3 = _origin(_wagons[0]) - _view.global_position
    var across:Vector3 = (to_track - along * along.dot(to_track)).normalized()
    var forward:Vector3 = -_view.global_transform.basis.z
    var turn:float = angle_difference(atan2(forward.x, forward.z), atan2(across.x, across.z))
    await _orbit(roundi(-turn / deg_to_rad(ORBIT_STEP_PX * _view.sensitivity)))
    await wait_idle_frames(SETTLE_FRAMES)
    var last:RID = _wagons[_wagons.size() - 1]
    var toward_last:bool = _view.global_transform.basis.x.dot(_origin(last) - _origin(_wagons[0])) > 0.0
    var arrow:Key = KEY_RIGHT if toward_last else KEY_LEFT
    _press(KEY_SHIFT, true)
    _press(arrow, true)
    var started:int = Time.get_ticks_msec()
    while not _in_the_middle() == last and Time.get_ticks_msec() - started < SLIDE_SECONDS * 1000.0:
        await wait_idle_frames(1)
    _press(arrow, false)
    _press(KEY_SHIFT, false)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_eq(_in_the_middle(), last, "slid to the last vehicle")
    var from_first:float = _view.global_position.distance_to(_origin(_wagons[0]))
    var aimed:float = _aimed()

    await _orbit(SMALL_ORBIT_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_aimed(), aimed, PULL_BACK, "the orbit keeps it aiming where it aimed")
    assert_gt(_view.global_position.distance_to(_origin(_wagons[0])), from_first - PULL_BACK,
            "nothing pulls it back to the first bogie")
    grab = grab.duplicate()
    grab.pressed = false
    Input.parse_input_event(grab)


func test_a_shot_moved_aside_stays_with_its_view() -> void:
    PlayerServer.player_take_over_vehicle(_wagons[0])
    var others:Array[PlayerCameraServer.CameraFollowView] = [PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE,
            PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY]
    var own:Array[Transform3D] = []
    for other:PlayerCameraServer.CameraFollowView in others:
        await _follow(_wagons[0], other)
        own.append(_view.global_transform)
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var event:InputEventKey = InputEventKey.new()
    event.physical_keycode = KEY_RIGHT
    event.pressed = true
    Input.parse_input_event(event)
    await wait_idle_frames(KEY_FRAMES)
    event = event.duplicate()
    event.pressed = false
    Input.parse_input_event(event)
    await wait_idle_frames(SETTLE_FRAMES)

    for index:int in others.size():
        _view.view = others[index]
        await wait_idle_frames(SETTLE_FRAMES)
        assert_almost_eq(_view.global_position.distance_to(own[index].origin), 0.0, TOLERANCE,
                "view %d stands as its own" % others[index])
        assert_almost_eq((-_view.global_transform.basis.z).angle_to(-own[index].basis.z), 0.0, TOLERANCE,
                "view %d looks as its own" % others[index])


func test_driving_the_vehicle_view_keeps_its_place_at_the_vehicle() -> void:
    await _follow(_loco, PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var standing:Transform3D = _local_view(_loco)

    for frame:int in DRIVE_FRAMES:
        RailVehicleServer.vehicle_move(_loco, DRIVE_STEP)
        await wait_idle_frames(1)
        _assert_same_local_view(_local_view(_loco), standing, "frame %d" % frame)
    RailVehicleServer.vehicle_move(_loco, -DRIVE_STEP * DRIVE_FRAMES)


func test_the_bogie_view_stands_at_each_vehicle_chosen() -> void:
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE)
    var first:Transform3D = _local_view(_wagons[0])
    assert_gt((-_view.global_transform.basis.z).dot(_to_head()), 0.0, "it looks to the head")

    for wagon:RID in _wagons:
        await _follow(wagon, PlayerCameraServer.CAMERA_FOLLOW_VIEW_BOGIE)
        var dimensions:Vector3 = VehicleServer.vehicle_get_dimensions(wagon)
        assert_lt(_view.global_position.distance_to(_origin(wagon)), dimensions.z, "it stands by the vehicle chosen")
        _assert_same_local_view(_local_view(wagon), first, "as at the first one")


func test_the_rear_view_of_the_players_vehicle_stands_at_the_last_vehicle_looking_at_the_rear() -> void:
    PlayerServer.player_take_over_vehicle(_wagons[0])

    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    assert_eq(_nearest(), _wagons[_wagons.size() - 1], "it stands at the last vehicle")
    assert_gt((-_view.global_transform.basis.z).dot(-_to_head()), 0.0, "it looks at the rear")


func test_the_rear_view_of_another_trainset_chosen_stands_at_its_last_vehicle() -> void:
    PlayerServer.player_take_over_vehicle(_loco)
    await _follow(_loco, PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    assert_eq(_nearest(), _wagons[_wagons.size() - 1], "it stands at the last vehicle of the trainset chosen")


func test_the_rear_view_goes_to_a_vehicle_chosen_keeping_its_offsets() -> void:
    PlayerServer.player_take_over_vehicle(_wagons[0])
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)
    await _wheel(ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    var at_last:Transform3D = _local_view(_wagons[_wagons.size() - 1])

    await _follow(_wagons[1], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    _assert_same_local_view(_local_view(_wagons[1]), at_last, "as at the last one")


func test_the_rear_view_from_the_cab_again_stands_at_the_last_vehicle() -> void:
    PlayerServer.player_take_over_vehicle(_wagons[0])
    await _follow(_wagons[1], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    # back in the cab, the next external view follows the player's vehicle
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    assert_eq(_nearest(), _wagons[_wagons.size() - 1])


func test_the_driveby_point_stands_while_the_vehicle_passes_and_is_set_ahead_once_it_is_far() -> void:
    var max_distance:Variant = ProjectSettings.get_setting(FollowView.MAX_DISTANCE_SETTING)
    ProjectSettings.set_setting(FollowView.MAX_DISTANCE_SETTING, DRIVEBY_MAX_DISTANCE)
    var head:RID = _wagons[0]
    await _follow(head, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY)
    var point:Vector3 = _view.global_position

    RailVehicleServer.vehicle_move(head, DRIVEBY_NEAR_MOVE)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_almost_eq(_view.global_position.distance_to(point), 0.0, TOLERANCE, "it stands")
    var to_head:Vector3 = (_origin(head) - _view.global_position).normalized()
    assert_gt((-_view.global_transform.basis.z).dot(to_head), 1.0 - TOLERANCE, "it follows the vehicle")

    RailVehicleServer.vehicle_move(head, DRIVEBY_FAR_MOVE)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_gt(_view.global_position.distance_to(point), TOLERANCE,
            "farther from the camera than a view goes, it is set again")
    assert_lt(_view.global_position.distance_to(_origin(head)), point.distance_to(_origin(head)), "nearer the vehicle")
    ProjectSettings.set_setting(FollowView.MAX_DISTANCE_SETTING, max_distance)


func test_the_driveby_view_stands_where_the_arrows_and_the_mouse_set_it_looking_at_the_vehicle() -> void:
    var head:RID = _wagons[0]
    await _follow(head, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY)
    var grab:InputEventMouseButton = InputEventMouseButton.new()
    grab.button_index = MOUSE_BUTTON_RIGHT
    grab.pressed = true
    Input.parse_input_event(grab)
    await wait_idle_frames(1)
    if not Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
        pending("the display server cannot capture the mouse")
        return
    var standing:Vector3 = _view.global_position

    for _step:int in WALK_FRAMES:
        var aside:InputEventMouseMotion = InputEventMouseMotion.new()
        aside.relative = Vector2(-ORBIT_STEP_PX, 0.0)
        Input.parse_input_event(aside)
        await wait_idle_frames(1)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_almost_eq(_view.global_position.distance_to(standing), 0.0, TOLERANCE, "the mouse aside moves it not")
    var left:Vector3 = -_view.global_transform.basis.x
    _press(KEY_LEFT, true)
    await wait_idle_frames(KEY_FRAMES)
    _press(KEY_LEFT, false)
    await wait_idle_frames(SETTLE_FRAMES)
    var moved:Vector3 = _view.global_position - standing
    assert_gt(moved.dot(left), TOLERANCE, "the left arrow moves it left on the screen")
    assert_almost_eq(moved.y, 0.0, TOLERANCE, "not up nor down")
    standing = _view.global_position
    for _step:int in SMALL_ORBIT_STEPS:
        var down:InputEventMouseMotion = InputEventMouseMotion.new()
        down.relative = Vector2(0.0, ORBIT_STEP_PX)
        Input.parse_input_event(down)
        await wait_idle_frames(1)
    await wait_idle_frames(SETTLE_FRAMES)
    assert_gt(_view.global_position.y, standing.y + TOLERANCE, "the mouse going down sets it higher")
    assert_almost_eq(Vector2(_view.global_position.x, _view.global_position.z).distance_to(
            Vector2(standing.x, standing.z)), 0.0, TOLERANCE, "straight up")
    var set:Vector3 = _view.global_position
    RailVehicleServer.vehicle_move(head, DRIVEBY_NEAR_MOVE)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_almost_eq(_view.global_position.distance_to(set), 0.0, TOLERANCE, "it stands there as the vehicle drives")
    var to_head:Vector3 = (_origin(head) - _view.global_position).normalized()
    assert_gt((-_view.global_transform.basis.z).dot(to_head), 1.0 - TOLERANCE, "looking at the vehicle")
    RailVehicleServer.vehicle_move(head, -DRIVEBY_NEAR_MOVE)
    grab = grab.duplicate()
    grab.pressed = false
    Input.parse_input_event(grab)


func test_the_driveby_view_held_aside_goes_only_aside_as_the_vehicle_drives_away_and_is_set_again() -> void:
    var max_distance:Variant = ProjectSettings.get_setting(FollowView.MAX_DISTANCE_SETTING)
    ProjectSettings.set_setting(FollowView.MAX_DISTANCE_SETTING, DRIVEBY_MAX_DISTANCE)
    var head:RID = _wagons[0]
    await _follow(head, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY)
    var start:Vector3 = _view.global_position
    var deep:Vector3 = -_view.global_transform.basis.z
    deep = Vector3(deep.x, 0.0, deep.z).normalized()

    # held aside while the vehicle drives away: aside only, until the vehicle is farther than a view
    # goes - then, the arrow still held, the camera is set again ahead of it
    _press(KEY_LEFT, true)
    var set_again:bool = false
    var driven:float = 0.0
    for frame:int in 2 * DRIVE_FRAMES:
        RailVehicleServer.vehicle_move(head, -DRIVE_STEP)
        driven += DRIVE_STEP
        var before:Vector3 = _view.global_position
        await wait_idle_frames(1)
        if before.distance_to(_view.global_position) > SET_AGAIN_JUMP:
            set_again = true
            break
        var moved:Vector3 = _view.global_position - start
        assert_lt(absf(moved.dot(deep)), maxf(TOLERANCE, ASIDE_SHARE * moved.length()),
                "frame %d: aside only, never deeper after the vehicle" % frame)
    _press(KEY_LEFT, false)

    assert_true(set_again, "the vehicle farther than a view goes, it is set again, the arrow held")
    assert_lt(_view.global_position.distance_to(_origin(head)), DRIVEBY_MAX_DISTANCE, "near the vehicle again")
    RailVehicleServer.vehicle_move(head, driven)
    ProjectSettings.set_setting(FollowView.MAX_DISTANCE_SETTING, max_distance)


func test_the_driveby_view_set_ahead_again_stands_to_the_vehicle_as_the_player_set_it() -> void:
    var max_distance:Variant = ProjectSettings.get_setting(FollowView.MAX_DISTANCE_SETTING)
    ProjectSettings.set_setting(FollowView.MAX_DISTANCE_SETTING, DRIVEBY_MAX_DISTANCE)
    var head:RID = _wagons[0]
    await _follow(head, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY)
    var own:Vector3 = _view.global_position - _origin(head)
    for key:Key in [KEY_PAGEUP, KEY_LEFT]:
        _press(key, true)
        await wait_idle_frames(SET_FRAMES)
        _press(key, false)
    await wait_idle_frames(SETTLE_FRAMES)
    var set:Vector3 = _view.global_position - _origin(head)
    assert_gt(set.distance_to(own), TOLERANCE, "the player set it elsewhere")

    var driven:float = 0.0
    var set_again:bool = false
    for _frame:int in 2 * DRIVE_FRAMES:
        RailVehicleServer.vehicle_move(head, -DRIVE_STEP)
        driven += DRIVE_STEP
        var before:Vector3 = _view.global_position
        await wait_idle_frames(1)
        if before.distance_to(_view.global_position) > SET_AGAIN_JUMP:
            set_again = true
            break
    assert_true(set_again, "the vehicle farther than a view goes, it is set again")
    assert_almost_eq((_view.global_position - _origin(head)).distance_to(set), 0.0, TOLERANCE,
            "standing to the vehicle as the player set it")

    _view.view = PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT
    await wait_idle_frames(SETTLE_FRAMES)
    _view.view = PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY
    await wait_idle_frames(SETTLE_FRAMES)
    assert_almost_eq((_view.global_position - _origin(head)).distance_to(own), 0.0, TOLERANCE,
            "chosen anew, at its own point")
    RailVehicleServer.vehicle_move(head, driven)
    ProjectSettings.set_setting(FollowView.MAX_DISTANCE_SETTING, max_distance)


func test_the_driveby_view_never_looks_up_at_the_vehicle() -> void:
    var head:RID = _wagons[0]
    await _follow(head, PlayerCameraServer.CAMERA_FOLLOW_VIEW_DRIVEBY)
    var center:float = _origin(head).y + 0.5 * VehicleServer.vehicle_get_dimensions(head).y
    assert_lt((-_view.global_transform.basis.z).y, TOLERANCE, "set, it looks across or down")
    var grab:InputEventMouseButton = InputEventMouseButton.new()
    grab.button_index = MOUSE_BUTTON_RIGHT
    grab.pressed = true
    Input.parse_input_event(grab)
    await wait_idle_frames(1)
    if not Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
        pending("the display server cannot capture the mouse")
        return

    for _step:int in 2 * SMALL_ORBIT_STEPS:
        var up:InputEventMouseMotion = InputEventMouseMotion.new()
        up.relative = Vector2(0.0, -ORBIT_STEP_PX)
        Input.parse_input_event(up)
        await wait_idle_frames(1)
    _press(KEY_PAGEDOWN, true)
    await wait_idle_frames(KEY_FRAMES)
    _press(KEY_PAGEDOWN, false)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_gt(_view.global_position.y, center - TOLERANCE, "it is never lower than the vehicle's center")
    assert_lt((-_view.global_transform.basis.z).y, TOLERANCE, "it never looks up at it")
    grab = grab.duplicate()
    grab.pressed = false
    Input.parse_input_event(grab)


func test_in_an_external_view_f5_takes_the_vehicle_followed() -> void:
    var player:Player = PLAYER_SCENE.instantiate()
    add_child(player)
    PlayerCameraServer.camera_set_target(_wagons[1])
    PlayerCameraServer.camera_set_mode(PlayerCameraServer.CAMERA_MODE_FOLLOW)

    var f5:InputEventKey = InputEventKey.new()
    f5.keycode = KEY_F5
    f5.pressed = true
    Input.parse_input_event(f5)
    await wait_idle_frames(1)
    f5 = f5.duplicate()
    f5.pressed = false
    Input.parse_input_event(f5)
    await wait_idle_frames(1)

    assert_eq(PlayerServer.player_get_vehicle(), _wagons[1], "the player drives the vehicle followed")
    assert_eq(PlayerCameraServer.camera_get_mode(), PlayerCameraServer.CAMERA_MODE_CABIN, "in its cab")
    PlayerServer.player_leave_vehicle()
    PlayerCameraServer.camera_set_target(RID())
    player.free()


func test_a_vehicle_of_another_trainset_chosen_is_shown_anew() -> void:
    PlayerCameraServer.camera_set_target(_wagons[0])
    PlayerCameraServer.camera_set_follow_view(PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)
    PlayerCameraServer.camera_set_target(_wagons[1])
    assert_eq(PlayerCameraServer.camera_get_follow_view(), PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR,
            "another vehicle of the trainset keeps the view")
    PlayerCameraServer.camera_set_target(_loco)
    assert_eq(PlayerCameraServer.camera_get_follow_view(), PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT,
            "another trainset from the first view")
    PlayerCameraServer.camera_set_target(RID())

    await _follow(_loco, PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var own:Transform3D = _local_view(_loco)
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    await _wheel(ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    await _follow(_loco, PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)

    _assert_same_local_view(_local_view(_loco), own, "the shot set back")


func test_offsets_stay_within_the_players_trainset_and_reset_for_another() -> void:
    PlayerServer.player_take_over_vehicle(_wagons[0])
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_FRONT)
    var default_view:Transform3D = _local_view(_wagons[0])
    await _wheel(ZOOM_STEPS)
    await wait_idle_frames(SETTLE_FRAMES)
    var zoomed:Transform3D = _local_view(_wagons[0])

    PlayerServer.player_take_over_vehicle(_wagons[1])
    await wait_idle_frames(SETTLE_FRAMES)
    _assert_same_local_view(_local_view(_wagons[0]), zoomed, "another vehicle of the trainset")

    PlayerServer.player_take_over_vehicle(_loco)
    await wait_idle_frames(SETTLE_FRAMES)
    _assert_same_local_view(_local_view(_wagons[0]), default_view, "another trainset")


## Last: it leaves the wagons two trainsets
func test_uncoupled_the_rear_view_goes_to_the_new_last_vehicle() -> void:
    PlayerServer.player_take_over_vehicle(_wagons[0])
    await _follow(_wagons[0], PlayerCameraServer.CAMERA_FOLLOW_VIEW_TRAINSET_REAR)

    (VehicleServer.vehicle_get_controller(_wagons[1]) as RailVehicleController).uncouple(
            RailVehicleController.COUPLER_END_REAR)
    await wait_idle_frames(SETTLE_FRAMES)

    assert_eq(_nearest(), _wagons[1])
