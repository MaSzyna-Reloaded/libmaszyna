extends BaseCabinTool3D
class_name CabinGauge

@export var value = 0.0:
    set(x):
        if not value == x:
            value = x
            _dirty = true

@export var max_value = 1.0:
    set(x):
        max_value = x
        _dirty = true

@export var max_angle = 270.0:
    set(x):
        max_angle = x
        _dirty = true
@export var mesh_rotation = Vector3.ZERO:
    set(x):
        mesh_rotation = x
        _dirty = true

@export var mesh_rotation_offset = Vector3.ZERO:
    set(x):
        mesh_rotation_offset = x
        _dirty = true

## A gauge that slides instead of turning - MMD "mov" (TGauge gt_Move, Gauge.cpp:466-469): the
## full-scale shift and the shift at zero
@export var mesh_position = Vector3.ZERO:
    set(x):
        mesh_position = x
        _dirty = true

@export var mesh_position_offset = Vector3.ZERO:
    set(x):
        mesh_position_offset = x
        _dirty = true

## How the gauge shows its value (TGaugeAnimation, Gauge.cpp:212-221)
enum AnimationType {
    ## `rot`, `rotvar`: turns about its axis
    ROTATE,
    ## `mov`, `movvar`: slides along its axis
    MOVE,
    ## `dgt`: a counter - each child named by a digit d turns to the d-th digit of the value
    ## (gt_Digital, Gauge.cpp:481-497)
    DIGITAL,
}
@export var animation_type:AnimationType = AnimationType.ROTATE
## `rotvar`/`movvar`: the value at which the scale has become `variable_end_scale` times the
## starting one, lerped between (GetScaledValue(), Gauge.cpp:448-456); 0 keeps the scale fixed
@export var variable_end_value:float = 0.0
@export var variable_end_scale:float = 1.0
## `dgt`: the counter shows floor(value * digital_scale + digital_offset)
@export var digital_scale:float = 1.0
@export var digital_offset:float = 0.0

## A drum turns by a tenth of a turn per digit (Gauge.cpp:493)
const DIGIT_ANGLE:float = -36.0
## The digits a counter shows at most (Gauge.cpp:489)
const DIGITAL_DIGITS:int = 10

## Needle smoothing rate; 0 moves the needle instantly (e.g. the jumping Hasler needle).
@export var animation_speed = 4.0
#@export var start_angle = 270.0

@export var state_property:String = ""
@export var max_state_property:String = ""
@export var max_config_property:String = ""

@export var target_mesh_path:NodePath:
    set(x):
        target_mesh_path = x
        _mesh = null
        _setup_phase = true

#var _current = 0.0;
#var _target = 0.0;
var _current_rotation:Vector3 = Vector3.ZERO
var _target_mesh_rotation:Vector3 = Vector3.ZERO
var _current_position:Vector3 = Vector3.ZERO
var _target_mesh_position:Vector3 = Vector3.ZERO
var _mesh_original_position:Vector3 = Vector3.ZERO
var _mesh:Node3D = null
var _mesh_original_basis:Basis
#var _base_rot:Vector3 = Vector3.ZERO
var _t = 0.0
var _setup_phase:bool = true
## A counter's digit drums: the child of the mesh and the digit it shows (DIGITAL)
var _digit_drums:Array[Node3D] = []
var _digit_drum_bases:Array[Basis] = []
var _digit_drum_places:PackedInt32Array = []

func _ready():
    vehicle_rid_changed.connect(_do_setup)

func _enter_tree():
    _setup_phase = true

func _do_setup():
    _on_state_update_timer_timeout()
    _aim()

func _process_dirty(delta):
    _aim()

    if not _mesh and target_mesh_path:
        _mesh = get_node_or_null(target_mesh_path)
        if _mesh:
            _mesh_original_basis = _mesh.basis
            _mesh_original_position = _mesh.position
            global_position = _mesh.global_position
            _take_wiper_chain(_mesh)
            _take_digit_drums()

func _process_tool(_delta):
    _t += _delta
    if _t > 0.1:
        _t = 0.0
        _on_state_update_timer_timeout()

    if _setup_phase and target_mesh_path and not _mesh:
        _dirty = true

    if _setup_phase and _mesh and value:
        _current_rotation = _target_mesh_rotation
        _current_position = _target_mesh_position
        _setup_phase = false
    else:
        var weight:float = friction_weight(_delta, animation_speed)
        _current_rotation = _current_rotation.lerp(_target_mesh_rotation, weight)
        _current_position = _current_position.lerp(_target_mesh_position, weight)

    if not is_instance_valid(_mesh):
        return
    if animation_type == AnimationType.DIGITAL:
        _pose_digit_drums()
        return
    var rotation_basis:Basis = Basis(Vector3.RIGHT, deg_to_rad(_current_rotation.x)) \
            * Basis(Vector3.UP, deg_to_rad(_current_rotation.y)) \
            * Basis(Vector3.FORWARD, deg_to_rad(_current_rotation.z))
    _mesh.transform.basis = _mesh_original_basis * rotation_basis
    _mesh.position = _mesh_original_position + _current_position
    _pose_wiper_chain(rotation_basis)


## Where the needle goes for the value: GetScaledValue() (Gauge.cpp:448-456) - a `rotvar`/`movvar`
## scale runs from the starting one to variable_end_scale times it as the value reaches
## variable_end_value
func _aim() -> void:
    if max_value == 0.0:
        return
    var share:float = value / max_value
    if variable_end_value > 0.0:
        share *= lerpf(1.0, variable_end_scale, clampf(value / variable_end_value, 0.0, 1.0))
    _target_mesh_rotation = mesh_rotation_offset + share * mesh_rotation
    _target_mesh_position = mesh_position_offset + share * mesh_position


## The children of a counter named by a digit, each with the place of the digit it shows
func _take_digit_drums() -> void:
    _digit_drums.clear()
    _digit_drum_bases.clear()
    _digit_drum_places.clear()
    if not animation_type == AnimationType.DIGITAL:
        return
    for child:Node in _mesh.get_children():
        var child_name:String = String(child.name)
        if child is Node3D and child_name and child_name[0].is_valid_int():
            _digit_drums.append(child)
            _digit_drum_bases.append((child as Node3D).basis)
            _digit_drum_places.append(int(child_name[0]))


## Every drum at its digit of floor(value * digital_scale + digital_offset), the units on drum 0
## (Gauge.cpp:486-495)
func _pose_digit_drums() -> void:
    var shown:int = int(floorf(value * digital_scale + digital_offset))
    for index:int in range(_digit_drums.size()):
        var digit:int = (shown / int(pow(10.0, _digit_drum_places[index]))) % 10 if shown >= 0 else 0
        _digit_drums[index].basis = _digit_drum_bases[index] * Basis(Vector3.UP, deg_to_rad(DIGIT_ANGLE * digit))

func _on_state_update_timer_timeout():
    if _vehicle_rid and max_config_property:
        max_value = _vehicle_config().get(max_state_property, 0.0)
    elif _vehicle_rid and max_state_property:
        max_value = _vehicle_state_value(max_state_property, 0.0)
    if _vehicle_rid and state_property:
        value = _vehicle_state_value(state_property, 0.0)
