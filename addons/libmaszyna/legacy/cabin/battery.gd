extends RefCounted
class_name LegacyCabinBattery

## Battery switch (battery_sw), with or without its gauge - TTrain::OnCommand_batterytoggle/enable/
## disable (Train.cpp:2891-3070). A press switches the battery; the gauge only follows, so the
## battery stays switchable from the keyboard and CabinSystem in a cab that has none. A push-type
## switch (type: return) springs back to neutral when released (Train.cpp:2929) and switches
## nothing then. The Mover's own impulse battery button (isBatteryButtonImpulse) is not in the
## vendored Mover.

const CONTROL:StringName = &"battery_sw"
## Two push buttons some cabs have instead (Train.cpp:11928-11929, OnCommand_batteryenable/disable)
const CONTROL_ON:StringName = &"batteryon_sw"
const CONTROL_OFF:StringName = &"batteryoff_sw"
## The pose of an impulse switch: up on, down off, midway at rest (Train.cpp:2918, 2929, 3003)
const SWITCH_OFF:float = 0.0
const SWITCH_ON:float = 1.0
const SWITCH_REST:float = 0.5

var _cabin:RID
var _battery_on:Callable = _battery_button.bind(CONTROL_ON, true)
var _battery_off:Callable = _battery_button.bind(CONTROL_OFF, false)


func control_ids() -> Array[StringName]:
    return [CONTROL, CONTROL_ON, CONTROL_OFF]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _battery)
    CabinSystem.register_control(cabin, CONTROL_ON, _battery_on)
    CabinSystem.register_control(cabin, CONTROL_OFF, _battery_off)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _battery)
    CabinSystem.unregister_control(_cabin, CONTROL_ON, _battery_on)
    CabinSystem.unregister_control(_cabin, CONTROL_OFF, _battery_off)


func _battery(state:CabinState, action:StringName, value:Variant) -> Variant:
    if action == &"release":
        state.set_value(CONTROL, SWITCH_REST)
        return null
    if not action in [&"hold", &"toggle", &"set"]:
        return null
    # Train.cpp:2895 - a press turns the battery on when the 24V circuit is not powered
    var enabled:bool = (not state.vehicle_state_value("power24_available", false)
            if value == null or action == &"hold" else bool(value))
    # an impulse switch is pushed up to switch on and down to switch off, then rests midway
    state.set_value(CONTROL, (SWITCH_ON if enabled else SWITCH_OFF) if action == &"hold" else enabled)
    return state.send_vehicle_command("battery", enabled)


## batteryon_sw / batteryoff_sw: the press switches the battery, the release lets the button back
## (Train.cpp:2939-2960 and their disable twins)
func _battery_button(state:CabinState, action:StringName, value:Variant, control:StringName, enabled:bool) -> Variant:
    if action == &"release":
        state.set_value(control, false)
        return null
    if not action in [&"hold", &"toggle"]:
        return null
    state.set_value(control, true)
    return state.send_vehicle_command("battery", enabled)
