extends Node3D
class_name CabinIndicator3D

## The vehicle this element sits in, as the cabin root hands it down.
func set_vehicle_rid(vehicle_rid:RID) -> void:
    if _vehicle_rid == vehicle_rid:
        return
    _vehicle_rid = vehicle_rid
    _dirty = true


## The lamp came on or went out - its glow (mmd_cabin_instancer.gd) follows it
signal lit_changed(lit:bool)

## Which vehicle this cabin element sits in; every read of it goes through CabinSystem.
var _vehicle_rid:RID
## The cabin of the Cabin3D it sits in, taken when it enters the tree - a cab is rebuilt for
## another cabin (MaszynaDynamicTrainCabin)
var _cabin:RID
var _on_target:Node3D
var _off_target:Node3D
var _dirty:bool = false
var _update_elapsed:float = 0.0
## The element's time - the simulation's
var _clock:SimulationClock = SimulationClock.new()

## When the lamp is lit by state_property: a flag, or the sign of a number - the reverser's
## buttons light by the sign of DirActive (Train.cpp:8520)
enum LitCondition { TRUE, POSITIVE, ZERO, NEGATIVE }

@export var enabled:bool = false
@export var state_property:String = ""
## Lit by a light of the cab it sits in (CabinSystem's cab light signals) instead of state_property
@export var cab_light:CabinState.Light = CabinState.Light.NONE
## What that light of the cab is at: its level, or 1 for a lit instrument light
var _cab_light_level:float = 0.0
@export var lit_condition:LitCondition = LitCondition.TRUE
## Lit while state_property is false - an "inactive" lamp of the same state (Train.cpp:9196).
@export var invert_value:bool = false
@export_node_path("Node3D") var on_target_path:NodePath = "":
    set(value):
        on_target_path = value
        _on_target = null
        _dirty = true
@export_node_path("Node3D") var off_target_path:NodePath = "":
    set(value):
        off_target_path = value
        _off_target = null
        _dirty = true


func _enter_tree() -> void:
    _cabin = BaseCabinTool3D.cabin_of(self)
    match cab_light:
        CabinState.Light.CAB:
            CabinSystem.cabin_light_level_changed.connect(_on_cab_light_changed)
        CabinState.Light.INSTRUMENT:
            CabinSystem.cabin_instrument_light_changed.connect(_on_cab_light_changed)
        CabinState.Light.DASHBOARD:
            CabinSystem.cabin_dashboard_light_changed.connect(_on_cab_light_changed)
        CabinState.Light.TIMETABLE:
            CabinSystem.cabin_timetable_light_changed.connect(_on_cab_light_changed)


func _exit_tree() -> void:
    match cab_light:
        CabinState.Light.CAB:
            CabinSystem.cabin_light_level_changed.disconnect(_on_cab_light_changed)
        CabinState.Light.INSTRUMENT:
            CabinSystem.cabin_instrument_light_changed.disconnect(_on_cab_light_changed)
        CabinState.Light.DASHBOARD:
            CabinSystem.cabin_dashboard_light_changed.disconnect(_on_cab_light_changed)
        CabinState.Light.TIMETABLE:
            CabinSystem.cabin_timetable_light_changed.disconnect(_on_cab_light_changed)


func _on_cab_light_changed(cabin:RID, value:Variant) -> void:
    if cabin == _cabin:
        _cab_light_level = float(value)
        _update_state()


func _process(delta:float) -> void:
    if _dirty:
        _process_dirty()
        _dirty = false

    _update_elapsed += _clock.advance(delta)
    if _update_elapsed > 0.1:
        _update_elapsed = 0.0
        _update_state()


func _process_dirty() -> void:
    if not _on_target and on_target_path:
        _on_target = get_node_or_null(on_target_path)
    if not _off_target and off_target_path:
        _off_target = get_node_or_null(off_target_path)
    if _vehicle_rid:
        # the light of the cab this element sits in, as the cab holds it now
        match cab_light:
            CabinState.Light.CAB:
                _cab_light_level = CabinSystem.cabin_get_light_level(_cabin)
            CabinState.Light.INSTRUMENT:
                _cab_light_level = float(CabinSystem.cabin_get_instrument_light_enabled(_cabin))
            CabinState.Light.DASHBOARD:
                _cab_light_level = float(CabinSystem.cabin_get_dashboard_light_enabled(_cabin))
            CabinState.Light.TIMETABLE:
                _cab_light_level = float(CabinSystem.cabin_get_timetable_light_enabled(_cabin))
    _update_state()


func _update_state() -> void:
    if _vehicle_rid and (state_property or not cab_light == CabinState.Light.NONE):
        var state:Variant = (_cab_light_level if not cab_light == CabinState.Light.NONE
                else CabinSystem.vehicle_state_value(_vehicle_rid, state_property, false))
        var value:bool = false
        match lit_condition:
            LitCondition.TRUE:
                value = true if state else false
            LitCondition.POSITIVE:
                value = float(state) > 0.0
            LitCondition.ZERO:
                value = float(state) == 0.0
            LitCondition.NEGATIVE:
                value = float(state) < 0.0
        # a lamp of the vehicle's state is dark without the low voltage (Button.cpp:126)
        var lit:bool = (not value if invert_value else value) \
                and (not cab_light == CabinState.Light.NONE or CabinSystem.vehicle_has_low_voltage(_vehicle_rid))
        if not lit == enabled:
            enabled = lit
            lit_changed.emit(lit)
    if _on_target:
        _on_target.visible = enabled
    if _off_target:
        _off_target.visible = not enabled
