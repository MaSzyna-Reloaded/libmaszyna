extends Camera3D
class_name ExternalCamera3D

## External views of a vehicle (PlayerCameraServer.camera_set_follow_view(), cycled with Shift+F4) -
## driver_mode::ExternalView() (drivermode.cpp:916-1037). The camera flies to the selected view like
## a drone and always looks at the vehicle. Dragging with the right mouse button orbits the view around
## the vehicle, the left/right arrows and PageUp/PageDown pan the view in the screen plane, the
## up/down arrows and the mouse wheel bring it closer or farther. It is active while it is the
## current camera.

enum View {TRAINSET_FRONT, TRAINSET_REAR, BOGIE, DRIVEBY}

## Distance scale per mouse wheel step towards the vehicle
const ZOOM_STEP:float = 0.9
## How fast the up/down arrows change the distance - e-fold per second, with Shift held
const ZOOM_KEY_RATE:float = 1.0
const ZOOM_KEY_RATE_FAST:float = 3.0
## Closest and farthest the zoom takes the view from what it looks at (m)
const MIN_DISTANCE_SETTING:StringName = &"maszyna/camera/external_view_min_distance"
const MIN_DISTANCE_DEFAULT:float = 1.0
const MAX_DISTANCE_SETTING:StringName = &"maszyna/camera/external_view_max_distance"
const MAX_DISTANCE_DEFAULT:float = 200.0

## Where the view looks at first, ahead of the camera it takes over from [m]
const LOOK_AHEAD:float = 10.0

## How fast the drone follows its target (1/s)
@export var response:float = 2.0
## Mouse orbit sensitivity (degrees per pixel)
@export var sensitivity:float = 0.25
## Speed of panning the view with the left/right arrows and PageUp/PageDown (m/s)
@export var move_speed:float = 5.0
## The same speed with Shift held (m/s)
@export var move_speed_fast:float = 20.0

## The vehicle followed (VehicleServer's)
var vehicle:RID
var view:View = View.TRAINSET_FRONT:
    set(x):
        if not view == x:
            view = x
            _dirty = true
## How fast the drone flies [m/s], for a camera taking over from it
var velocity:Vector3 = Vector3.ZERO

var _dirty:bool = false
# the vehicle the view is attached to (none for the drive-by point) and its local offset
var _view_vehicle:RID
var _view_offset:Vector3 = Vector3.ZERO
# the bogie view looks at the bogie of _view_vehicle instead of the vehicle center
var _bogie_look_offset:Vector3 = Vector3.ZERO
var _look_target:Vector3 = Vector3.ZERO
var _orbit:Vector2 = Vector2.ZERO
# screen-plane shift of the view in metres (x right, y up) and scale of its distance to the target
var _pan:Vector2 = Vector2.ZERO
var _zoom:float = 1.0
# the view's own distance from its target before the zoom, for the zoom to keep within the limits
var _view_distance:float = 1.0
## The zoom's limits, as their settings are now
var _min_distance:float = MIN_DISTANCE_DEFAULT
var _max_distance:float = MAX_DISTANCE_DEFAULT
# the vehicle and the view the orbit, pan and zoom were set for
var _offset_vehicle:RID
var _offset_view:View = View.TRAINSET_FRONT


## Starts the flight from p_from to the selected view of the vehicle, the view applied in full. The
## selected view is kept between activations, like m_externalviewmode of the original.
func _enter_tree() -> void:
    ProjectSettings.settings_changed.connect(_on_project_settings_changed)
    _on_project_settings_changed()


func _exit_tree() -> void:
    ProjectSettings.settings_changed.disconnect(_on_project_settings_changed)


func _on_project_settings_changed() -> void:
    _min_distance = ProjectSettings.get_setting(MIN_DISTANCE_SETTING, MIN_DISTANCE_DEFAULT)
    _max_distance = ProjectSettings.get_setting(MAX_DISTANCE_SETTING, MAX_DISTANCE_DEFAULT)


func activate(p_vehicle:RID, p_from:Transform3D) -> void:
    vehicle = p_vehicle
    global_transform = p_from
    _look_target = p_from.origin - p_from.basis.z * LOOK_AHEAD
    _dirty = true
    make_current()


func _input(event:InputEvent) -> void:
    if not current:
        return
    if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_RIGHT:
        Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED if event.pressed else Input.MOUSE_MODE_VISIBLE)
    elif event is InputEventMouseMotion and Input.get_mouse_mode() == Input.MOUSE_MODE_CAPTURED:
        _orbit.x -= deg_to_rad(event.relative.x * sensitivity)
        _orbit.y = clampf(_orbit.y - deg_to_rad(event.relative.y * sensitivity), -1.4, 1.4)


# the wheel is left to a HUD under the cursor first (the vehicle card scrolls with it)
func _unhandled_input(event:InputEvent) -> void:
    if not current or not event is InputEventMouseButton or not event.pressed:
        return
    if event.button_index == MOUSE_BUTTON_WHEEL_UP:
        _zoom_by(ZOOM_STEP)
    elif event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
        _zoom_by(1.0 / ZOOM_STEP)


func _process(delta:float) -> void:
    if not current or not vehicle.is_valid():
        return
    if _dirty:
        _dirty = false
        _process_dirty()
    _move_view(delta)

    var view_transform:Transform3D = RailVehicleRenderingServer.vehicle_get_transform(_view_vehicle)
    var look_target:Vector3 = view_transform * _bogie_look_offset if view == View.BOGIE else _get_vehicle_center(vehicle)
    var view_position:Vector3 = view_transform * _view_offset if _view_vehicle.is_valid() else _view_offset
    var arm:Vector3 = (view_position - look_target).rotated(Vector3.UP, _orbit.x)
    var pitch_axis:Vector3 = Vector3.UP.cross(arm)
    if not pitch_axis.is_zero_approx():
        arm = arm.rotated(pitch_axis.normalized(), _orbit.y)
    _view_distance = arm.length()
    arm *= _zoom
    # the pan moves the target and the camera together along the screen axes, so the view slides
    var forward:Vector3 = -arm.normalized()
    var right:Vector3 = forward.cross(Vector3.UP).normalized()
    look_target += right * _pan.x + right.cross(forward) * _pan.y

    var weight:float = 1.0 - exp(-response * delta)
    var previous_position:Vector3 = global_position
    global_position = global_position.lerp(look_target + arm, weight)
    if delta > 0.0:
        velocity = (global_position - previous_position) / delta
    _look_target = _look_target.lerp(look_target, weight)
    if not global_position.is_equal_approx(_look_target):
        look_at(_look_target, Vector3.UP)


## Sets the view up once per selection, like the default view setup of ExternalView(). The wheel's
## and the keys' orbit, pan and zoom belong to one vehicle's one view: another vehicle or view starts
## from the view itself - the original keeps them per view instead (m_externalviewconfigs,
## drivermode.cpp:590-596, 942-946).
func _process_dirty() -> void:
    if not (vehicle == _offset_vehicle and view == _offset_view):
        _offset_vehicle = vehicle
        _offset_view = view
        _orbit = Vector2.ZERO
        _pan = Vector2.ZERO
        _zoom = 1.0
    var controller:RailVehicleController = VehicleServer.vehicle_get_controller(vehicle) as RailVehicleController
    # the cab driven from, as the original numbers it (CabOccupied): the front 1, the rear -1, the
    # machine room or none 0 (Train.cpp:8684)
    var cabin_occupied:int = 0
    match RailVehicleServer.cabin_get_kind(RailVehicleServer.vehicle_get_driver_cabin(vehicle)):
        RailVehicleCabinKind.RAIL_VEHICLE_CABIN_FRONT:
            cabin_occupied = 1
        RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR:
            cabin_occupied = -1
    var direction:int = controller.get_direction()
    var cab:int = 1 if cabin_occupied == 0 else cabin_occupied
    # Godot vehicles face -Z; MaSzyna's vehicle frame is (left, up, front)
    var body:Transform3D = RailVehicleRenderingServer.vehicle_get_transform(vehicle)
    var front:Vector3 = -body.basis.z.normalized()
    var left:Vector3 = -body.basis.x.normalized()

    if view == View.DRIVEBY:
        # driver_mode::DistantView(false) (drivermode.cpp:904-907) - a fixed point 50 m ahead of the cab
        _view_vehicle = RID()
        _view_offset = (
            body.origin
            + front * cabin_occupied * 50.0
            + Vector3(-10.0 * left.x * cab, 1.6, -10.0 * left.z * cab))
        return

    # drivermode.cpp:951-1019 - offsetflip from the occupied cab and the active direction
    var flip:float = cab * (VehicleController.DIRECTION_FORWARD if direction == VehicleController.DIRECTION_NEUTRAL else direction)
    if view == View.TRAINSET_REAR:
        flip = -flip

    # Mechanik->Vehicle(end::front / end::rear) - the last vehicle of the trainset on that side
    _view_vehicle = RailVehicleServer.vehicle_get_coupled(
            vehicle, RailVehicleController.COUPLER_END_FRONT if flip > 0.0 else RailVehicleController.COUPLER_END_REAR,
            RailVehicleController.COUPLING_FLAG_COUPLER)[0]
    var dimensions:Vector3 = VehicleServer.vehicle_get_dimensions(_view_vehicle)
    var width:float = dimensions.x
    var height:float = dimensions.y
    var length:float = dimensions.z

    var offset:Vector3
    match view:
        View.TRAINSET_FRONT:
            offset = Vector3(1.5 * width * flip, maxf(5.0, 1.25 * height), -0.4 * length * flip)
        View.TRAINSET_REAR:
            offset = Vector3(1.5 * width * flip, maxf(5.0, 1.25 * height), 0.2 * length * flip)
        View.BOGIE:
            offset = Vector3(-0.65 * width * flip, 0.9, 0.15 * length * flip)
            # the original looks ahead along the vehicle side at bogie height (drivermode.cpp:1021),
            # so aim at the front bogie area (Godot local, -Z ahead)
            _bogie_look_offset = Vector3(0.0, 0.9, -0.35 * length * flip)
    # the MaSzyna (left, up, front) frame is yawed by 180 degrees in Godot
    _view_offset = Vector3(-offset.x, offset.y, -offset.z)


## The left/right arrows and PageUp/PageDown pan the view in the screen plane, the up/down arrows
## zoom it. The original moves the owner offset relative to the camera heading instead
## (TCamera::Update(), Camera.cpp:191-213).
func _move_view(delta:float) -> void:
    var pan:Vector2 = Vector2(
        float(Input.is_physical_key_pressed(KEY_RIGHT)) - float(Input.is_physical_key_pressed(KEY_LEFT)),
        float(Input.is_physical_key_pressed(KEY_PAGEUP)) - float(Input.is_physical_key_pressed(KEY_PAGEDOWN)))
    var zoom:float = float(Input.is_physical_key_pressed(KEY_DOWN)) - float(Input.is_physical_key_pressed(KEY_UP))
    if pan.is_zero_approx() and zoom == 0.0:
        return
    var fast:bool = Input.is_key_pressed(KEY_SHIFT)
    _pan += pan.normalized() * (move_speed_fast if fast else move_speed) * delta
    _zoom_by(exp(zoom * (ZOOM_KEY_RATE_FAST if fast else ZOOM_KEY_RATE) * delta))


## Scales the view's distance, kept within the min/max distance settings
func _zoom_by(p_factor:float) -> void:
    _zoom = clampf(_zoom * p_factor, _min_distance / _view_distance, _max_distance / _view_distance)


func _get_vehicle_center(p_vehicle:RID) -> Vector3:
    var body:Transform3D = RailVehicleRenderingServer.vehicle_get_transform(p_vehicle)
    return body.origin + body.basis.y.normalized() * 0.5 * VehicleServer.vehicle_get_dimensions(p_vehicle).y
