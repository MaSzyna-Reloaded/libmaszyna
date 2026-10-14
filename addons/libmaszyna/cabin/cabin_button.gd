extends BaseCabinTool3D
class_name CabinButton

signal pushed_changed()
signal button_pushed()

enum ControllerMode { OnOff, On, Off }
## What kind of switch this is, as the MMD's `type:` says - the original's TGaugeType
## (Gauge.h:24), bit for bit: a TOGGLE stays where it is put, a PUSH springs back, DELAYED acts on
## release. Set by the MMD factory (Gauge.cpp:243). Data about the switch, not logic: the cabin
## behaviour that owns the control branches on it, as TTrain's handlers branch on ggX.type().
enum ButtonType {
    TOGGLE = 1,
    PUSH = 2,
    PUSH_TOGGLE = 3,
    DELAYED = 4,
    PUSH_DELAYED = 6,
    PUSH_TOGGLE_DELAYED = 7,
}

@export var pushed:bool = false:
    set(x):
        if not x == pushed:
            pushed = x
            value = 1.0 if pushed else value_rest
            emit_signal("pushed_changed")

## The pose shown, as TGauge's value: value * scale + offset (Gauge.cpp:456). It follows `pushed`
## (1 pushed, value_rest released) unless the cabin logic sets one of its own - an impulse lever
## doing two things is pushed up to 1 or down to 0 (Train.cpp:3773, 3815).
@export var value:float = 0.0:
    set(x):
        value = x
        _update_mesh_target()
## Where the control rests released: 0, or 0.5 for an impulse lever with a neutral position
## midway (battery_sw, main_sw, pantselected_sw - Train.cpp:11342, 11348, 3455)
@export var value_rest:float = 0.0:
    set(x):
        value_rest = x
        if not pushed:
            value = value_rest

@export var monostable:bool = false
@export var button_type:ButtonType = ButtonType.TOGGLE
@export_node_path("MeshInstance3D") var mesh_path:NodePath = "":
    set(x):
        mesh_path = x
        _mesh = null
        _dirty = true

@export var command = ""
## Fixed leading argument sent before `pushed`, for commands that take a selector as their first
## parameter (e.g. RailVehicleEnginePowerSource::pantograph(PantographSelector, bool)) - unset (null) for
## every single-argument command, which keeps existing widgets (fuelpump_sw, battery_sw, ...)
## sending exactly the same single-argument call as before.
@export var command_param:Variant
@export var state_property = ""
@export var controller_mode:ControllerMode = ControllerMode.OnOff
@export var mesh_position:Vector3 = Vector3.ZERO:
    set(x):
        mesh_position = x
        _update_mesh_target()
@export var mesh_rotation:Vector3 = Vector3.ZERO:
    set(x):
        mesh_rotation = x
        _update_mesh_target()
## Pose of the released button, the MMD offset (value * scale + offset, Gauge.cpp:456). A model
## authored in its pushed pose rests displaced from it, e.g. E186's vigilance pedal
## (pedal_sifa rot 0.008 -0.008).
@export var mesh_position_offset:Vector3 = Vector3.ZERO:
    set(x):
        mesh_position_offset = x
        _update_mesh_target()
@export var mesh_rotation_offset:Vector3 = Vector3.ZERO:
    set(x):
        mesh_rotation_offset = x
        _update_mesh_target()
## How fast the control follows its value - 1 / the MMD's friction; 0 moves it at once
## (TGauge::Update(), Gauge.cpp:364-375)
@export var animation_speed = 10.0
## The cab's sound player and the events of its bank this button plays, filled by whoever builds
## the cab (MmdCabinInstancer)
@export var sound_player:SfxPlayer3D
@export var sound_on_event:StringName
@export var sound_off_event:StringName

## A push button's state under its caption (a toggle shows STATE_ON/STATE_OFF)
const STATE_PUSHED:String = "pressed"
const STATE_RELEASED:String = "released"

var _mesh:Node3D
var _mesh_original_basis:Basis
var _mesh_original_position:Vector3 = Vector3.ZERO
var _target_mesh_rotation:Vector3 = Vector3.ZERO
var _target_mesh_position:Vector3 = Vector3.ZERO
var _current_rotation:Vector3 = Vector3.ZERO
var _current_position:Vector3 = Vector3.ZERO
var _t:float = 0.0
var _setup_phase:bool = true

func _ready():
    connect("pushed_changed", self._on_pushed_changed)
    vehicle_rid_changed.connect(_update_state)

func _enter_tree():
    _setup_phase = true

func _update_state():
    if state_property and _vehicle_rid:
        pushed = _vehicle_state_value(state_property, pushed)
        return
    # A control with no vehicle state behind it (universalN) shows what the cab holds for it: a cab
    # rebuilt with it left on shows it on, and the next press turns it off
    var held:Variant = null
    if _cabin and control_id:
        held = CabinSystem.get_control(_cabin, control_id)
    if held == null:
        pushed = false
        return
    _apply_control_value(held)

func _update_mesh_target() -> void:
    _target_mesh_position = mesh_position_offset + mesh_position * value
    _target_mesh_rotation = mesh_rotation_offset + mesh_rotation * value

func _process_dirty(delta):
    if not _mesh and mesh_path:
        _mesh = get_node_or_null(mesh_path)
        if _mesh:
            global_position = _mesh.global_position
            _mesh_original_basis = _mesh.transform.basis
            _mesh_original_position = _mesh.position
            _take_wiper_chain(_mesh)
            _set_mouse_control(_mesh, hint_actions, press, release, Callable(), Callable(), Vector3.ZERO, Vector3.ZERO)
            _set_mouse_state(_mouse_state())
    _update_state()

func _process_tool(delta):
    _t += delta
    if _t > 0.1:
        if _setup_phase and mesh_path and not _mesh:
            _dirty = true

    if _setup_phase and _mesh:
        _setup_phase = false
        _current_position = _target_mesh_position
        _current_rotation = _target_mesh_rotation
    else:
        var weight:float = friction_weight(delta, animation_speed)
        _current_rotation = _current_rotation.lerp(_target_mesh_rotation, weight)
        _current_position = _current_position.lerp(_target_mesh_position, weight)

    if is_instance_valid(_mesh):
        var rotation_basis:Basis = Basis(Vector3.RIGHT, deg_to_rad(_current_rotation.x)) \
                * Basis(Vector3.UP, deg_to_rad(_current_rotation.y)) \
                * Basis(Vector3.FORWARD, deg_to_rad(_current_rotation.z))
        _mesh.transform.basis = _mesh_original_basis * rotation_basis
        _mesh.position = _mesh_original_position + _current_position
        _pose_wiper_chain(rotation_basis)

## A flag from the cabin logic presses or releases the control; a number is the pose itself
## (0, value_rest or 1), shown as it is.
func _apply_control_value(p_value:Variant) -> void:
    if p_value is bool:
        pushed = p_value
        return
    pushed = not is_equal_approx(float(p_value), value_rest)
    value = float(p_value)

func _play_sound():
    var event:StringName = sound_on_event if pushed else sound_off_event
    if sound_player and event:
        sound_player.play(event)

## The state under the caption.
func _mouse_state() -> String:
    if monostable:
        return STATE_PUSHED if pushed else STATE_RELEASED
    return STATE_ON if pushed else STATE_OFF

func _on_pushed_changed():
    _set_mouse_state(_mouse_state())
    if pushed:
        button_pushed.emit()
    _play_sound()
