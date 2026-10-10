extends RefCounted
class_name LegacyCabinJointController

## Joint master controller (MMD "jointctrl:", e.g. SM42's nastawnik): moving it forward adjusts
## power, backward past the no-power position applies the local brake. Ported from the cab layer:
## TTrain::OnCommand_mastercontrollerincrease/decrease (Train.cpp:1094-1146) with the joint
## controller branches of OnCommand_independentbrakeincrease/decrease (Train.cpp:1449-1510).
## The handle shows controller_joint_position (Train.cpp:7699-7714).

## The master controller is the driven vehicle's, the local brake the cab's own (OnCommand_jointcontrollerset)
const TARGET:CabinState.Target = CabinState.Target.CONTROLLED
const CONTROL:StringName = &"jointctrl"

var _cabin:RID


func control_ids() -> Array[StringName]:
    return [CONTROL]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _joint_controller)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _joint_controller)


func _joint_controller(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if action == &"increase":
        # Train.cpp:1098 - an applied local brake is released first
        if float(state.vehicle_state_value("brake_local_position_normalized", 0.0)) > 0.0:
            return state.send_vehicle_command("local_brake_decrease")
        return state.send_vehicle_command("main_controller_increase", null, null, TARGET)
    if action == &"decrease":
        # Train.cpp:1138 - below the no-power position the handle applies the local brake
        if state.vehicle_state_value("main_no_power_pos", false, TARGET):
            return state.send_vehicle_command("local_brake_increase")
        return state.send_vehicle_command("main_controller_decrease", null, null, TARGET)
    return null
