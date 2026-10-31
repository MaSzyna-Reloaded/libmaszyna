extends RefCounted
class_name MaszynaCabinManualWipers

## Wipers driven by hand (RailVehicleWipers.DRIVE_MANUAL - a FIZ of no WiperList,
## MaszynaRailVehicle3DInstancer.apply_wiper_count()): the wiper switch's keys move every wiper
## across the glass and back, smoothly while held, as the knob CONTROL - the glass is wiped by them
## even in a cab of no handle to animate. A cab whose model has the handles turns each of them by
## the mouse as well (MmdManualWiperHandles, reloaded_manualwiper<N>), its wiper with it.

const CONTROL:StringName = &"reloaded_manualwipers"
## How fast a held key moves the wipers: their sweep a simulated second
const KEY_SPEED:float = 1.0
## The knob the keys hold (MaszynaCabinLogic._bind())
const FIELDS:Dictionary = {
    "value_min": 0.0,
    "value_max": 1.0,
    "step": KEY_SPEED,
    "state_property": "wipers_manual_position_0",
    "action_increase": "wipers_switch_increase",
    "action_decrease": "wipers_switch_decrease",
}

var _cabin:RID
## The handles' controls and what each moves
var _handles:Dictionary[StringName, Callable] = {}


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _wipers)
    for wiper:int in RailVehicleWipers.MANUAL_WIPERS_MAX:
        var control_id:StringName = StringName(MmdManualWiperHandles.LABEL_PREFIX + str(wiper + 1))
        _handles[control_id] = _handle.bind(wiper)
        CabinSystem.register_control(cabin, control_id, _handles[control_id])


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _wipers)
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
