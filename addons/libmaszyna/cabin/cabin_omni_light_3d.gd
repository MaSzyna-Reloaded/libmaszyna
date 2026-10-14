extends OmniLight3D
class_name CabinOmniLight3D

## The vehicle this element sits in, as the cabin root hands it down.
func set_vehicle_rid(vehicle_rid:RID) -> void:
    if _vehicle_rid == vehicle_rid:
        return
    _vehicle_rid = vehicle_rid
    _dirty = true


## Which vehicle this cabin element sits in; every read of it goes through CabinSystem.
var _vehicle_rid:RID
## The cabin of the Cabin3D it sits in, taken when it enters the tree - a cab is rebuilt for
## another cabin (MaszynaDynamicTrainCabin)
var _cabin:RID

var _dirty:bool = false
var _t = 0.0
## The element's time - the simulation's
var _clock:SimulationClock = SimulationClock.new()
var _target_light_energy = 0.0

@export var enabled:bool = false

@export var state_property = ""
## Lit by a light of the cab it sits in (CabinSystem's cab light signals) instead of state_property
@export var cab_light:CabinState.Light = CabinState.Light.NONE
## What that light of the cab is at: its level, or 1 for a lit instrument light
var _cab_light_level:float = 0.0
@export var light_energy_on = 1.0
@export var light_energy_off = 0.0
@export var animation_speed = 20.0
var _setup_phase:bool = true

func _ready():
    pass

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

func _update_state():
    var level:float = 1.0
    if not cab_light == CabinState.Light.NONE:
        level = _cab_light_level
        enabled = level > 0.0
    elif _vehicle_rid and state_property:
        # a bool state or a 0..1 level
        level = float(CabinSystem.vehicle_state_value(_vehicle_rid, state_property, false))
        enabled = level > 0.0

    _target_light_energy = lerpf(light_energy_off, light_energy_on, level) if enabled else light_energy_off

func _process(frame_delta):
    var delta:float = _clock.advance(frame_delta)
    if _dirty:
        _dirty = false
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
            _setup_phase = true
            _update_state()

    _t += delta
    if _t > 0.1:
        _t = 0.0
        _update_state()

    if _setup_phase:
        light_energy = _target_light_energy
        _setup_phase = false
    else:
        if visible and not light_energy:
            visible = false
        elif not visible and light_energy:
            visible = true
        light_energy = lerpf(light_energy, _target_light_energy, BaseCabinTool3D.friction_weight(delta, animation_speed))
