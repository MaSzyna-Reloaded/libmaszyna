extends RefCounted
class_name MaszynaCabinManualWipers

## Wipers driven by hand (RailVehicleWipers.DRIVE_MANUAL - a FIZ of no WiperList,
## MaszynaRailVehicle3DInstancer.apply_wiper_count()): the wiper switch's keys move every wiper
## across the glass and back, smoothly while held, as the knob CONTROL - the glass is wiped by them
## even in a cab of no handle to animate. A cab whose model has the handles turns each of them by
## the mouse as well (a game's MaszynaCabControlPatch, HANDLE_LABEL_PREFIX<N>), its wiper with it.
## A cab with the valves of the wipers' air motors (VALVE_LABEL_PREFIX<N>, an SM42's, an EU07's) has
## each opened by the mouse and all of them by the keys, CONTROL_VALVES in place of CONTROL.

const CONTROL:StringName = &"reloaded_manualwipers"
## The label of wiper N's handle, from 1 (MmdSemanticCatalog)
const HANDLE_LABEL_PREFIX:String = "reloaded_manualwiper"
## The first wiper's handle, whose way the keys follow (MaszynaCabinControls.manual_wipers_keys_reversed)
const FIRST_HANDLE_LABEL:String = HANDLE_LABEL_PREFIX + "1"
## How fast a held key moves the wipers: their sweep a simulated second
const KEY_SPEED:float = 1.0
## The knob the keys hold in a cab with valves
const CONTROL_VALVES:StringName = &"reloaded_wipervalves"
## The label of wiper N's valve, from 1 (MmdSemanticCatalog)
const VALVE_LABEL_PREFIX:String = "reloaded_wipervalve"
## The valve whose being in a cab has the keys open the valves
const FIRST_VALVE_LABEL:StringName = &"reloaded_wipervalve1"
## How fast a held key opens the valves, their whole range a simulated second
const VALVE_KEY_SPEED:float = 1.0
## The knob the keys hold in a cab with valves (MaszynaCabinLogic._bind()): ] opens them, [ closes them
const VALVE_FIELDS:Dictionary = {
    "value_min": 0.0,
    "value_max": 1.0,
    "step": VALVE_KEY_SPEED,
    "state_property": "wipers_valve_0",
    "action_increase": "wipers_switch_increase",
    "action_decrease": "wipers_switch_decrease",
}

## The knob the keys hold (MaszynaCabinLogic._bind()): wipers_switch_increase (]) moves the wipers to
## the driver's right, as the first wiper's handle is dragged - its sweep's way the other, the two
## keys swapped (reversed)
static func fields(reversed:bool) -> Dictionary:
    return {
        "value_min": 0.0,
        "value_max": 1.0,
        "step": KEY_SPEED,
        "state_property": "wipers_manual_position_0",
        "action_increase": "wipers_switch_decrease" if reversed else "wipers_switch_increase",
        "action_decrease": "wipers_switch_increase" if reversed else "wipers_switch_decrease",
    }

var _cabin:RID
## The handles' and the valves' controls and what each moves
var _handles:Dictionary[StringName, Callable] = {}


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _wipers)
    CabinSystem.register_control(cabin, CONTROL_VALVES, _valves)
    for wiper:int in RailVehicleWipers.MANUAL_WIPERS_MAX:
        var control_id:StringName = StringName(HANDLE_LABEL_PREFIX + str(wiper + 1))
        _handles[control_id] = _handle.bind(wiper)
        CabinSystem.register_control(cabin, control_id, _handles[control_id])
        var valve_id:StringName = StringName(VALVE_LABEL_PREFIX + str(wiper + 1))
        _handles[valve_id] = _valve.bind(wiper)
        CabinSystem.register_control(cabin, valve_id, _handles[valve_id])


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _wipers)
    CabinSystem.unregister_control(_cabin, CONTROL_VALVES, _valves)
    for control_id:StringName in _handles:
        CabinSystem.unregister_control(_cabin, control_id, _handles[control_id])
    _handles.clear()


func _wipers(state:CabinState, action:StringName, value:Variant) -> Variant:
    if action == &"set":
        return state.send_vehicle_command("wipers_manual_set_all", value)
    return null


func _handle(state:CabinState, action:StringName, value:Variant, wiper:int) -> Variant:
    if action == &"set":
        return state.send_vehicle_command("wipers_manual_set_%d" % wiper, value)
    return null


func _valves(state:CabinState, action:StringName, value:Variant) -> Variant:
    if action == &"set":
        return state.send_vehicle_command("wipers_valve_set_all", value)
    return null


func _valve(state:CabinState, action:StringName, value:Variant, wiper:int) -> Variant:
    if action == &"set":
        return state.send_vehicle_command("wipers_valve_set_%d" % wiper, value)
    return null
