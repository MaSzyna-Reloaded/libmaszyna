extends BaseCabinTool3D
class_name CabinSwitch

signal switch_position_changed(previous_position, new_position)
signal switch_pushed()
signal switch_released()

enum ControllerMode { OnOff, On, Off }

## What the positions are called, position -> msgid (the reverser's
## "forward"/"backward"), from the MMD catalog; a position without a name shows its number
@export var position_names:Dictionary = {}

@export var switch_position:int = 0:
    set(x):
        x = clampi(x, switch_min_position, switch_max_position)
        if not x == switch_position:
            _dirty = true
            var previous_position:int = switch_position
            switch_position = x
            emit_signal("switch_position_changed", previous_position, switch_position)


@export var switch_min_position:int = 0:
    set(x):
        switch_min_position = x
        _dirty = true

@export var switch_max_position:int = 1:
    set(x):
        switch_max_position = x
        _dirty = true

@export_node_path("MeshInstance3D") var mesh_path:NodePath = "":
    set(x):
        mesh_path = x
        _mesh = null
        _dirty = true


@export var command_increase = ""
@export var command_decrease = ""
@export var command_set = ""

@export var state_property = ""

@export var mesh_position:Vector3 = Vector3.ZERO:
    set(x):
        mesh_position = x if x else Vector3.ZERO
        _dirty = true

@export var mesh_rotation:Vector3 = Vector3.ZERO:
    set(x):
        mesh_rotation = x if x else Vector3.ZERO
        _dirty = true

## Pose at position 0, the MMD offset (value * scale + offset, Gauge.cpp:456) - e.g. E186's master
## controller rests turned back from its model's pose (nastawnik_mocy rot 0.0263 -0.0788)
@export var mesh_position_offset:Vector3 = Vector3.ZERO:
    set(x):
        mesh_position_offset = x
        _dirty = true
@export var mesh_rotation_offset:Vector3 = Vector3.ZERO:
    set(x):
        mesh_rotation_offset = x
        _dirty = true

## Some state domains don't start at the switch's own visual rest position - e.g. radio_channel's
## real range is 1..10 (channel 0 isn't valid), but the physical knob's first notch is still the
## visual "zero" position, so switch_position=1 must render as ONE STEP from rest, not two.
## Subtracted from switch_position before computing mesh rotation/position only - command
## dispatch and clamping (switch_min/max_position) still use the real, unshifted switch_position.
@export var value_offset:int = 0:
    set(x):
        value_offset = x
        _dirty = true

## How fast the control follows its value - 1 / the MMD's friction; 0 moves it at once
## (TGauge::Update(), Gauge.cpp:364-375)
@export var animation_speed = 10.0
## The cab's sound player and the events of its bank this switch plays, filled by whoever builds
## the cab (MmdCabinInstancer). sound_override_events[N - 1] belongs to position N,
## sound_override_negative_events[N - 1] to position -N; an empty name leaves the position silent.
@export var sound_player:SfxPlayer3D
@export var sound_increase_event:StringName
@export var sound_decrease_event:StringName
@export var sound_neutral_position_event:StringName
@export var sound_override_events:Array[StringName]
@export var sound_override_negative_events:Array[StringName]

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
    self.switch_position_changed.connect(self._on_switch_position_changed)

    vehicle_rid_changed.connect(_on_vehicle_rid_changed)
    vehicle_rid_changing.connect(_on_vehicle_rid_changing)

func _on_vehicle_rid_changing() -> void:
    if CabinSystem.vehicle_command_received.is_connected(_on_command_received):
        CabinSystem.vehicle_command_received.disconnect(_on_command_received)

func _on_vehicle_rid_changed() -> void:
    CabinSystem.vehicle_command_received.connect(_on_command_received)
    _update_state()

func _update_state() -> void:
    if state_property and _vehicle_rid:
        switch_position = int(_vehicle_state_value(state_property, switch_position))
    _update_mesh_target()


func _update_mesh_target() -> void:
    _target_mesh_position = mesh_position_offset + (switch_position - value_offset) * mesh_position
    _target_mesh_rotation = mesh_rotation_offset + (switch_position - value_offset) * mesh_rotation

func _on_command_received(vehicle_rid:RID, p_command:String, p_p1:Variant, _p_p2:Variant) -> void:
    # the command goes to the vehicle the control drives - an EMU's master controller to its motor car
    if not _vehicle_rid or not vehicle_rid == CabinState.vehicle_of(_vehicle_rid, target):
        return
    if command_set and p_command == command_set:
        switch_position = int(p_p1) if p_p1 else 0
    elif (p_command == command_increase or p_command == command_decrease):
        _update_state()

func _enter_tree():
    _setup_phase = true

func _process_dirty(delta):
    if not _mesh and mesh_path:
        _mesh = get_node_or_null(mesh_path)
        if _mesh:
            global_position = _mesh.global_position
            _mesh_original_basis = _mesh.transform.basis
            _mesh_original_position = _mesh.position
            _set_mouse_control(_mesh, hint_actions, press, release,
                    increase, decrease, mesh_rotation, mesh_position)
            _set_mouse_state(_mouse_state())

func _process_tool(delta):
    _t += delta
    if _t > 0.05:
        _t = 0.0
        _update_mesh_target()

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
        var new_basis = _mesh_original_basis
        new_basis *= Basis(Vector3.RIGHT, deg_to_rad(_current_rotation.x))
        new_basis *= Basis(Vector3.UP, deg_to_rad(_current_rotation.y))
        new_basis *= Basis(Vector3.FORWARD, deg_to_rad(_current_rotation.z))
        _mesh.transform.basis = new_basis
        _mesh.position = _mesh_original_position + _current_position

func _apply_control_value(p_value:Variant) -> void:
    switch_position = int(p_value)

## The position under the caption: its name, on/off for a two-state switch, else its number.
func _mouse_state() -> String:
    if position_names.has(switch_position):
        return position_names[switch_position]
    if switch_min_position == 0 and switch_max_position == 1:
        return STATE_ON if switch_position else STATE_OFF
    return str(switch_position)

func _on_switch_position_changed(previous, current):
    _set_mouse_state(_mouse_state())
    var event:StringName = &""
    if current == 0 and sound_neutral_position_event:
        event = sound_neutral_position_event
    elif current > 0 and current <= sound_override_events.size():
        event = sound_override_events[current-1]
    elif current < 0 and -current <= sound_override_negative_events.size():
        event = sound_override_negative_events[-current-1]
    elif not current == 0:
        event = sound_increase_event if current > previous else sound_decrease_event

    if sound_player and event:
        sound_player.play(event)
