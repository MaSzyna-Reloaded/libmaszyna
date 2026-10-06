extends RefCounted
class_name LegacyCabinMotorBlowersAllOff

## The switch holding every traction motor blower off (motorblowersalloff_sw) - TTrain::
## OnCommand_motorblowersdisableall (Train.cpp:4948-4996). A push switch holds both ends' blowers
## off while it is held; a two-state one flips them on a press.

## The blowers are the driven vehicle's (mvControlled->MotorBlowersSwitchOff())
const TARGET:CabinState.Target = CabinState.Target.CONTROLLED
const CONTROL:StringName = &"motorblowersalloff_sw"
## The two ends' "off" commands (MotorBlowersSwitchOff(..., end::front/rear))
const SWITCH_OFF_COMMANDS:Array[String] = ["motor_blowers_front_switch_off", "motor_blowers_rear_switch_off"]

var _button_type:CabinButton.ButtonType
var _cabin:RID


func _init(button_type:CabinButton.ButtonType) -> void:
    _button_type = button_type


func control_ids() -> Array[StringName]:
    return [CONTROL]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _all_off)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _all_off)


func _all_off(state:CabinState, action:StringName, value:Variant) -> Variant:
    var held_off:bool
    if _button_type == CabinButton.ButtonType.PUSH:
        held_off = state.is_pressed(CONTROL, action, value)
    else:
        # two-state: only a press counts, and it flips the switch (Train.cpp:4977-4995)
        if action == &"release":
            return null
        held_off = not state.get_value(CONTROL, false) if value == null or action == &"hold" else bool(value)
    state.set_value(CONTROL, held_off)
    var result:Variant = null
    for command:String in SWITCH_OFF_COMMANDS:
        result = state.send_vehicle_command(command, held_off, null, TARGET)
    return result
