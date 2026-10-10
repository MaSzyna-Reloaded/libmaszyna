extends RefCounted
class_name LegacyCabinSpringBrakeShutOff

## The spring brake's shut-off valve, a keyboard-only control of the original cab layer:
## TTrain::OnCommand_springbrakeshutofftoggle (Train.cpp:6836) - no cab models the valve. A control
## of its own, so the AI turns it the way a player does.

const CONTROL:StringName = &"spring_brake_shut_off_toggle"
const ACTION:StringName = &"spring_brake_shut_off_toggle"

var _cabin:RID


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _toggle)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _toggle)


func _toggle(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if not action == &"hold":
        return null
    # the command takes "enabled", the opposite of the valve, so the valve's state is the new value
    return state.send_vehicle_command(
            "set_spring_brake_enabled", bool(state.vehicle_state_value("spring_brake/shut_off", false)))
