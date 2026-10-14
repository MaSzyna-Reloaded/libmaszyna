extends BaseCabinTool3D
class_name CabinKnob

signal value_changed()

enum ControllerMode { OnOff, On, Off }

@export var value:float = 0.0:
    set(x):
        if not x == value:
            value = x
            emit_signal("value_changed")

@export var value_min:float = 0.0;
@export var value_max:float = 1.0;

@export_node_path("MeshInstance3D") var mesh_path:NodePath = "":
    set(x):
        mesh_path = x
        _mesh = null
        _dirty = true

@export var command = ""
@export var state_property = ""
@export var config_min_property = ""
@export var config_max_property = ""

@export var mesh_position:Vector3 = Vector3.ZERO:
    set(x):
        mesh_position = x
        _target_mesh_position = x
        _dirty = true

@export var mesh_rotation:Vector3 = Vector3.ZERO:
    set(x):
        mesh_rotation = x
        _target_mesh_rotation = x
        _dirty = true

@export var mesh_position_offset:Vector3 = Vector3.ZERO:
    set(x):
        mesh_position_offset = x
        _dirty = true

@export var mesh_rotation_offset:Vector3 = Vector3.ZERO:
    set(x):
        mesh_rotation_offset = x
        _dirty = true

## How fast the control follows its value - 1 / the MMD's friction; 0 moves it at once
## (TGauge::Update(), Gauge.cpp:364-375)
@export var animation_speed = 10.0

## The positions the value range spans - Handle->GetPos(bh_MIN)..GetPos(bh_MAX) for a brake valve,
## whose whole positions are the rows of its brake pressure table (BCPN). Equal, the default, for
## a knob without positions: its value is then its position.
@export var position_min:float = 0.0
@export var position_max:float = 0.0
## The cab's sound player whose bank holds the events below, filled by whoever builds the cab
## (MmdCabinInstancer)
@export var sound_player:SfxPlayer3D
## An event per whole position (MMD soundN:/sound-N:), played when the knob comes to stand on it,
## and for a move that ends between positions (TGauge::UpdateValue, Gauge.cpp:302-343)
@export var sound_position_events:Dictionary[int, StringName] = {}
## What the whole positions are called, position -> msgid (a brake
## valve's "drive", "cutoff", ...), from the MMD catalog and the vehicle's handle
@export var position_names:Dictionary = {}
@export var sound_increase_event:StringName
@export var sound_decrease_event:StringName

## Mouse travel, in pixels, that takes the knob across its whole range
const MOUSE_PIXELS_PER_RANGE:float = 400.0
## A dragged knob with positions stops on each whole position, like a notch of the real handle,
## and leaves it only when the mouse pulls this far against it...
const DETENT_BREAKAWAY_PIXELS:float = 20.0
## ...quicker than the pull relaxes: a slow push never frees it, a jerk does. Seconds for the pull
## to fall to about a third.
const DETENT_RELAX_TIME:float = 0.15
## How close to a whole position the knob stands on it - the original compares hundredths
## (Gauge.cpp:305)
const POSITION_TOLERANCE:float = 0.005

var _mesh:Node3D
var _mesh_original_rotation:Vector3 = Vector3.ZERO
var _mesh_original_basis:Basis
var _mesh_original_position:Vector3 = Vector3.ZERO
var _target_mesh_rotation:Vector3 = Vector3.ZERO
var _target_mesh_position:Vector3 = Vector3.ZERO
var _current_rotation:Vector3 = Vector3.ZERO
var _current_position:Vector3 = Vector3.ZERO

## The value the last sound was chosen for
var _sounded_value:float = 0.0
## How hard, in pixels of signed travel, the mouse pulls against the notch the knob stands in
var _detent_pull:float = 0.0
var _detent_pull_time:int = 0
## The hand holds the knob: its position is the hand's, and the vehicle's state - a step behind the
## commands the drag sends - does not pull it back until let go
var _held:bool = false
var _t = 0.0
var _value_normalized = 0.0
var _setup_phase:bool = true

func _enter_tree():
    _setup_phase = true

func _update_state():
    if _vehicle_rid:
        if config_min_property:
            value_min = _vehicle_config().get(config_min_property, value_min)
        if config_max_property:
            value_max = _vehicle_config().get(config_max_property, value_max)
        if state_property and not _held:
            value = _vehicle_state_value(state_property, value)
        _value_normalized = value / (value_max - value_min)
        _target_mesh_position = mesh_position_offset + mesh_position * _value_normalized
        _target_mesh_rotation = mesh_rotation_offset + mesh_rotation * _value_normalized

func _ready():
    value_changed.connect(_on_value_changed)
    vehicle_rid_changed.connect(_update_state)

func _process_tool(delta):
    _t += delta
    if _t > 0.05:
        _t = 0.0
        _update_state()

        if _setup_phase and mesh_path and not _mesh:
            _dirty = true

    if _setup_phase and _mesh:
        _setup_phase = false
        _current_position = _target_mesh_position
        _current_rotation = _target_mesh_rotation
        _sounded_value = value
    else:
        var weight:float = friction_weight(delta, animation_speed)
        _current_rotation = _current_rotation.lerp(_target_mesh_rotation, weight)
        _current_position = _current_position.lerp(_target_mesh_position, weight)

    if not _setup_phase and not value == _sounded_value:
        _play_sound(_sounded_value)
        _sounded_value = value

    if is_instance_valid(_mesh):
        var new_basis = _mesh_original_basis
        new_basis *= Basis(Vector3.RIGHT, deg_to_rad(_current_rotation.x))
        new_basis *= Basis(Vector3.UP, deg_to_rad(_current_rotation.y))
        new_basis *= Basis(Vector3.FORWARD, deg_to_rad(_current_rotation.z))
        _mesh.transform.basis = new_basis
        _mesh.position = _mesh_original_position + _current_position


func _apply_control_value(p_value:Variant) -> void:
    value = float(p_value)

## The knob's state under its caption: the position it stands at among its positions, else how far
## along its range it is.
func _on_value_changed() -> void:
    if position_max > position_min:
        var position:float = _position()
        var whole:int = roundi(position)
        if absf(position - whole) < POSITION_TOLERANCE and position_names.has(whole):
            _set_mouse_state(position_names[whole])
            return
        _set_mouse_state("%.1f" % position)
    else:
        _set_mouse_state("%d%%" % roundi((value - value_min) / (value_max - value_min) * 100.0))

## The hand takes the knob (mouse button down on it) and lets it go; its key is the cab logic's
func grab() -> void:
    _held = true
    _detent_pull = 0.0

func let_go() -> void:
    _held = false

## The knob dragged by `travel` pixels of mouse movement: smoothly, up to the next whole position,
## where it holds until jerked free (see DETENT_BREAKAWAY_PIXELS). The original has no such
## notches - its mouse sets the handle directly (drivermouseinput.cpp:98) - they are how this
## wrapper's mouse lets the hand find the positions the MMD gives sounds to.
func drag(travel:float) -> void:
    if not position_max > position_min:
        _set_value_from_input(value + travel / MOUSE_PIXELS_PER_RANGE * (value_max - value_min))
        return
    var position:float = _position()
    if absf(position - roundf(position)) < POSITION_TOLERANCE:
        var now:int = Time.get_ticks_msec()
        _detent_pull *= exp(-(now - _detent_pull_time) / 1000.0 / DETENT_RELAX_TIME)
        _detent_pull_time = now
        _detent_pull += travel
        if absf(_detent_pull) < DETENT_BREAKAWAY_PIXELS:
            return
        travel = _detent_pull - signf(_detent_pull) * DETENT_BREAKAWAY_PIXELS
        _detent_pull = 0.0
    var wanted:float = position + travel / MOUSE_PIXELS_PER_RANGE * (position_max - position_min)
    # the next notch ahead stops the move
    if travel > 0.0:
        wanted = minf(wanted, floorf(position + POSITION_TOLERANCE) + 1.0)
    else:
        wanted = maxf(wanted, ceilf(position - POSITION_TOLERANCE) - 1.0)
    _set_position_from_input(wanted)

## Where the knob stands in its positions.
func _position() -> float:
    if not position_max > position_min:
        return value
    return position_min + (value - value_min) / (value_max - value_min) * (position_max - position_min)

func _set_position_from_input(p_position:float) -> void:
    _set_value_from_input(
            value_min + (p_position - position_min) / (position_max - position_min) * (value_max - value_min))

## The sound of a whole position the knob now stands on, else of the move (Gauge.cpp:310-340); a
## move sound already playing is not restarted - the original's exclusive mode for a knob that
## moves continuously.
func _play_sound(previous_value:float) -> void:
    if not sound_player:
        return
    var position:float = _position()
    var whole:int = roundi(position)
    if absf(position - whole) < POSITION_TOLERANCE and sound_position_events.has(whole):
        sound_player.play(sound_position_events[whole])
        return
    var event:StringName = sound_increase_event if value > previous_value else sound_decrease_event
    if event and not sound_player.is_playing(event):
        sound_player.play(event)

func _set_value_from_input(p_value:float) -> void:
    var new_value:float = clampf(p_value, value_min, value_max)
    # a drag sets the value itself, the pixels-to-value step being the widget's
    if not new_value == value and _cabin and control_id:
        CabinSystem.act(_cabin, control_id, &"set", new_value)
    value = new_value

func _process_dirty(delta):
    if not _mesh and mesh_path:
        _mesh = get_node_or_null(mesh_path)
        if _mesh:
            _mesh_original_rotation = _mesh.rotation_degrees
            _mesh_original_basis = _mesh.transform.basis
            _mesh_original_position = _mesh.position
            # the drag's direction is that of a one-pixel move
            _set_mouse_control(_mesh, hint_actions, grab, let_go, Callable(),
                    Callable(), mesh_rotation / MOUSE_PIXELS_PER_RANGE, mesh_position / MOUSE_PIXELS_PER_RANGE,
                    drag)
            _on_value_changed()
