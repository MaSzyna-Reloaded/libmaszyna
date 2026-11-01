extends RefCounted
class_name MaszynaCabinManualWipers

## Wipers driven by hand (RailVehicleWipers.DRIVE_MANUAL - a FIZ of no WiperList,
## MaszynaRailVehicle3DInstancer.apply_wiper_count()): the wiper switch's keys move every wiper
## across the glass and back, smoothly while held, as the knob CONTROL - the glass is wiped by them
## even in a cab of no handle to animate. A cab whose model has the handles turns each of them by
## the mouse as well (a game's MaszynaCabControlPatch, HANDLE_LABEL_PREFIX<N>), its wiper with it.

const CONTROL:StringName = &"reloaded_manualwipers"
## The label of wiper N's handle, from 1 (MmdSemanticCatalog)
const HANDLE_LABEL_PREFIX:String = "reloaded_manualwiper"
## The first wiper's handle, whose way the keys follow (MaszynaCabinControls.manual_wipers_keys_reversed)
const FIRST_HANDLE_LABEL:String = HANDLE_LABEL_PREFIX + "1"
## How fast a held key moves the wipers: their sweep a simulated second
const KEY_SPEED:float = 1.0
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
## The handles' controls and what each moves
var _handles:Dictionary[StringName, Callable] = {}


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _wipers)
    for wiper:int in RailVehicleWipers.MANUAL_WIPERS_MAX:
        var control_id:StringName = StringName(HANDLE_LABEL_PREFIX + str(wiper + 1))
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
