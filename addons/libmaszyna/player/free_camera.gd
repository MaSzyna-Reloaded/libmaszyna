# FreeCamera3D
#
# Based on https://github.com/adamviola/simple-free-look-camera (MIT license)
# FIXME: may not be optimized

extends Camera3D
class_name FreeCamera3D

enum AccelMode {NORMAL, MEDIUM, FAST}

@export var sensitivity:float = 0.25
@export var key_forward = KEY_UP
@export var key_backward = KEY_DOWN
@export var key_left = KEY_LEFT
@export var key_right = KEY_RIGHT
@export var key_up = KEY_PAGEUP
@export var key_down= KEY_PAGEDOWN

@export var enabled:bool = true
@export var acceleration:float = 10:
    set(x):
        acceleration = x

@export var acceleration_medium:float = 100
@export var acceleration_fast:float = 1000
## How fast a glide handed over by glide() slows down [m/s²]
@export var deceleration = 4.0:
    set(x):
        deceleration = clampf(x, 0, 1000)
@export var velocity_multiplier:float = 1.0

@export var bound_min = Vector3.ZERO
@export var bound_max = Vector3.ZERO
@export var bound_enabled:bool = false

const MOUSE_SMOOTHING: float = 24.0
const MOVEMENT_SMOOTHING: float = 20.0

# Movement state
var _direction: Vector3 = Vector3.ZERO
var _smoothed_direction: Vector3 = Vector3.ZERO
var _pending_mouse_delta: Vector2 = Vector2.ZERO
var _mouse_look_velocity: Vector2 = Vector2.ZERO
# Looking around while the right button is held - not whenever the mouse is captured, which a cab
# lever drag does as well (CabinHUDMouseSystem)
var _looking: bool = false
# The velocity the camera keeps after a hand-over, until deceleration stops it [m/s]
var _glide_velocity: Vector3 = Vector3.ZERO
# The keys have steered the camera since the hand-over: the glide only fades out
var _glide_steered: bool = false

# Keyboard state
var _w: float = 0.0
var _s: float = 0.0
var _a: float = 0.0
var _d: float = 0.0
var _q: float = 0.0
var _e: float = 0.0
var accel_mode: AccelMode = AccelMode.NORMAL

func _ready():
    Console.console_toggled.connect(_on_console_toggle)

func _on_console_toggle(console_visible):
    enabled = not console_visible

func _input(event):
    if not enabled:
        return

    var hovered_control := get_viewport().gui_get_hovered_control()

    # Receives mouse button input
    if event is InputEventMouseButton:
        match event.button_index:
            MOUSE_BUTTON_RIGHT: # Only allows rotation if right click down
                # the right button with a modifier is another action, not the look (cabin_sit_down)
                if event.pressed and (event.ctrl_pressed or event.shift_pressed or event.alt_pressed):
                    return
                _looking = event.pressed
                Input.set_mouse_mode(Input.MOUSE_MODE_CAPTURED if event.pressed else Input.MOUSE_MODE_VISIBLE)
            MOUSE_BUTTON_WHEEL_UP:
                if hovered_control:
                    return
                #velocity_multiplier = clamp(velocity_multiplier * 1.1, 0.2, 20)
                fov = clamp(fov - 1, 1, 100)
            MOUSE_BUTTON_WHEEL_DOWN:
                if hovered_control:
                    return
                fov = clamp(fov + 1, 1, 100)

    # Receives key input
    if event is InputEventKey:
        if event.is_command_or_control_pressed():
            accel_mode = AccelMode.FAST
        elif event.shift_pressed:
            accel_mode = AccelMode.MEDIUM
        else:
            accel_mode = AccelMode.NORMAL

        match event.physical_keycode:
            key_forward:
                _w = float(event.pressed)
            key_backward:
                _s = float(event.pressed)
            key_left:
                _a = float(event.pressed)
            key_right:
                _d = float(event.pressed)
            key_down:
                _q = float(event.pressed)
            key_up:
                _e = float(event.pressed)

    if _looking and event is InputEventMouseMotion:
        _pending_mouse_delta += event.relative

## The camera stands still where it is put next: the look, the keys' and a glide's motion left over
## from before do not carry it on
func stop_motion() -> void:
    _looking = false
    _pending_mouse_delta = Vector2.ZERO
    _mouse_look_velocity = Vector2.ZERO
    _smoothed_direction = Vector3.ZERO
    _glide_velocity = Vector3.ZERO


## Keeps the camera moving at the velocity [m/s] another camera had when it handed over, slowing
## down by deceleration until it stops - or, once the keys steer it, fading into their speed
func glide(p_velocity:Vector3) -> void:
    _glide_velocity = p_velocity
    _glide_steered = false


# Updates mouselook and movement every frame
func _process(delta: float) -> void:
    _update_acceleration_mode()
    _update_movement(delta)
    _update_mouselook(delta)

# Updates camera movement
func _update_movement(delta: float) -> void:
    var smoothing_weight: float = _get_smoothing_weight(MOVEMENT_SMOOTHING, delta)

    # Computes desired direction from key states
    _direction = Vector3(_d - _a, _e - _q, _s - _w)
    if not _direction.is_zero_approx():
        _direction = _direction.normalized()

    if not _glide_velocity.is_zero_approx():
        # once the keys steer, the glide fades as fast as their own speed builds up, so the one
        # blends into the other and is not counted after; left alone, it slows down by the
        # deceleration
        _glide_steered = _glide_steered or not _direction.is_zero_approx()
        if _glide_steered:
            _glide_velocity = _glide_velocity.lerp(Vector3.ZERO, smoothing_weight)
        else:
            _glide_velocity = _glide_velocity.move_toward(Vector3.ZERO, deceleration * delta)
        global_position += _glide_velocity * delta

    _smoothed_direction = _smoothed_direction.lerp(_direction, smoothing_weight)

    if _smoothed_direction.is_zero_approx():
        _smoothed_direction = Vector3.ZERO
        return

    var accel: float = 0.0
    
    match accel_mode:
        AccelMode.FAST:
            accel = acceleration_fast
        AccelMode.MEDIUM:
            accel = acceleration_medium
        _:
            accel = acceleration
        
    var velocity: Vector3 = _smoothed_direction * accel * velocity_multiplier

    var new_position: Vector3 = transform.translated_local(velocity * delta).origin
    if bound_enabled:
        new_position.x = clamp(new_position.x, bound_min.x, bound_max.x)
        new_position.y = clamp(new_position.y, bound_min.y, bound_max.y)
        new_position.z = clamp(new_position.z, bound_min.z, bound_max.z)
    position = new_position

# Updates mouse look
func _update_mouselook(delta: float) -> void:
    var smoothing_weight: float = _get_smoothing_weight(MOUSE_SMOOTHING, delta)

    _mouse_look_velocity += _pending_mouse_delta
    _pending_mouse_delta = Vector2.ZERO

    var mouse_delta: Vector2 = _mouse_look_velocity * smoothing_weight
    _mouse_look_velocity -= mouse_delta

    if mouse_delta.is_zero_approx():
        _mouse_look_velocity = Vector2.ZERO
        return

    # Adjust mouse delta by sensitivity
    var yaw_delta: float = mouse_delta.x * sensitivity
    var pitch_delta: float = mouse_delta.y * sensitivity

    # Apply yaw rotation to the parent or self (horizontal rotation)
    rotate_y(deg_to_rad(-yaw_delta))

    # Get the current pitch from the camera's rotation
    var current_pitch: float = rotation_degrees.x - pitch_delta
    current_pitch = clamp(current_pitch, -90, 90)

    # Apply the clamped pitch directly
    rotation_degrees.x = current_pitch


func _update_acceleration_mode() -> void:
    if Input.is_key_pressed(KEY_CTRL) or Input.is_key_pressed(KEY_META):
        accel_mode = AccelMode.FAST
    elif Input.is_key_pressed(KEY_SHIFT):
        accel_mode = AccelMode.MEDIUM
    else:
        accel_mode = AccelMode.NORMAL


func _get_smoothing_weight(response: float, delta: float) -> float:
    return clampf(1.0 - exp(-maxf(response, 0.001) * delta), 0.0, 1.0)
