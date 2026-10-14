extends RefCounted
class_name LegacyCabinPump

## A switch the original works as its fuel pump's (fuelpump_sw, oilpump_sw, waterpump_sw,
## motorblowersfront_sw, motorblowersrear_sw) - TTrain::OnCommand_fuelpumptoggle/enable/disable
## (Train.cpp:3871-3968) and their oil pump, water pump and motor blowers twins (Train.cpp:3970-4067,
## 4228-4325, 4750-4946). What it does depends on the kind of switch:
## * push (type: return) - the pump runs while it is held (Train.cpp:3914);
## * two-state - a press flips it, and switching off also sets the pump's off flag
##   (FuelPumpSwitchOff, Train.cpp:3937, 3963).

## The pumps and blowers are the driven vehicle's (OnCommand_fuelpump*/oilpump*/waterpump*/
## motorblowers*: mvControlled)
const TARGET:CabinState.Target = CabinState.Target.CONTROLLED
var _control:StringName
var _command:String
var _switch_off_command:String
var _enabled_state:String
var _button_type:CabinButton.ButtonType
var _cabin:RID


## control: the MMD label; command/switch_off_command: the device's vehicle commands;
## enabled_state: the state key of its switch (is_enabled)
func _init(control:StringName, command:String, switch_off_command:String, enabled_state:String,
        button_type:CabinButton.ButtonType) -> void:
    _control = control
    _command = command
    _switch_off_command = switch_off_command
    _enabled_state = enabled_state
    _button_type = button_type


func control_ids() -> Array[StringName]:
    return [_control]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, _control, _pump)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, _control, _pump)


func _pump(state:CabinState, action:StringName, value:Variant) -> Variant:
    if _button_type == CabinButton.ButtonType.PUSH:
        var held:bool = state.is_pressed(_control, action, value)
        state.set_value(_control, held)
        return state.send_vehicle_command(_command, held, null, TARGET)
    # two-state: only a press counts (Train.cpp:3889)
    if action == &"release":
        return null
    var enabled:bool = (not state.vehicle_state_value(_enabled_state, false, TARGET)
            if value == null or action == &"hold" else bool(value))
    state.set_value(_control, enabled)
    state.send_vehicle_command(_switch_off_command, not enabled, null, TARGET)
    return state.send_vehicle_command(_command, enabled, null, TARGET)
