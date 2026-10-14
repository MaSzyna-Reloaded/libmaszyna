extends RefCounted
class_name LegacyCabinCabActivation

## Cab activation (cabactivation_sw), with or without its gauge (dynamic/pkp/e186_v2 has none) -
## TTrain::OnCommand_cabactivationtoggle/enable/disable (Train.cpp:3075-3150). A press activates
## the cab when none is active and deactivates it otherwise; the gauge only follows. A push-type
## switch springs back to neutral when released (Train.cpp:3115) and switches nothing then.

const CONTROL:StringName = &"cabactivation_sw"
## The pose of an impulse switch: up on, down off, midway at rest (Train.cpp:3100, 3115, 3129)
const SWITCH_OFF:float = 0.0
const SWITCH_ON:float = 1.0
const SWITCH_REST:float = 0.5

var _cabin:RID


func control_ids() -> Array[StringName]:
    return [CONTROL]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _cab_activation)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _cab_activation)


func _cab_activation(state:CabinState, action:StringName, value:Variant) -> Variant:
    if action == &"release":
        state.set_value(CONTROL, SWITCH_REST)
        return null
    if not action in [&"hold", &"toggle", &"set"]:
        return null
    # Train.cpp:3083 - a press activates the cab when none is active
    var enabled:bool = (state.vehicle_state_value("cabin", 0) == 0
            if value == null or action == &"hold" else bool(value))
    # an impulse switch is pushed up to switch on and down to switch off, then rests midway
    state.set_value(CONTROL, (SWITCH_ON if enabled else SWITCH_OFF) if action == &"hold" else enabled)
    return state.send_vehicle_command("cab_activation", enabled)
