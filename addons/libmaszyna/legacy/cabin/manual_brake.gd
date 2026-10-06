extends RefCounted
class_name LegacyCabinManualBrake

## Manual brake of a cabin whose MMD has no manualbrake gauge. The original cab layer handles
## TTrain::OnCommand_manualbrakeincrease/decrease (Train.cpp:1809-1837) regardless of the gauge,
## so the manual brake stays operable from the keyboard and CabinSystem.

const CONTROL:StringName = &"manualbrake"
const ACTION_INCREASE:StringName = &"manual_brake_increase"
const ACTION_DECREASE:StringName = &"manual_brake_decrease"

var _cabin:RID


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _manual_brake)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _manual_brake)


func _manual_brake(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if action == &"increase":
        return state.send_vehicle_command("manual_brake_increase")
    if action == &"decrease":
        return state.send_vehicle_command("manual_brake_decrease")
    return null
