extends RefCounted
class_name LegacyCabinTempomat

## The cruise control switch, ported from the original cab layer: TTrain::OnCommand_tempomattoggle
## (Train.cpp:1486-1549) - on, it moves the controlled vehicle's second controller to its first
## position, off it takes it back two. A push switch springs back on release and, off, shows on
## the off switch where the cab has one; a two-state switch shows whether it is on. Its lamp is
## the speed control's active state (stategauges, Train.cpp:11999-12000; MmdSemanticCatalog).

const SWITCH:StringName = &"tempomat_sw"
const OFF_SWITCH:StringName = &"tempomatoff_sw"
## The second controller's steps a press takes it (IncScndCtrl(1), DecScndCtrl(2), Train.cpp:1510-1516)
const ON_STEPS:int = 1
const OFF_STEPS:int = 2

var _button_type:CabinButton.ButtonType
var _has_off_switch:bool
var _cabin:RID
var _handler:Callable = _toggle


func _init(button_type:CabinButton.ButtonType, has_off_switch:bool) -> void:
    _button_type = button_type
    _has_off_switch = has_off_switch


func control_ids() -> Array[StringName]:
    return [SWITCH, OFF_SWITCH]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, SWITCH, _handler)
    CabinSystem.register_control(cabin, OFF_SWITCH, _handler)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, SWITCH, _handler)
    CabinSystem.unregister_control(_cabin, OFF_SWITCH, _handler)


func _toggle(state:CabinState, action:StringName, _value:Variant) -> Variant:
    # TGauge::is_push(): the push bit of the kind (Gauge.h)
    var push:bool = not int(_button_type) & int(CabinButton.ButtonType.PUSH) == 0
    if push and action == &"release":
        state.set_value(SWITCH, false)
        state.set_value(OFF_SWITCH, false)
        return null
    if action == &"release":
        return null
    var off:bool = int(state.vehicle_state_value(
            "controller_second_position", 0, CabinState.Target.CONTROLLED)) > 0
    var result:Variant = state.send_vehicle_command(
            "second_controller_decrease" if off else "second_controller_increase",
            OFF_STEPS if off else ON_STEPS, null, CabinState.Target.CONTROLLED)
    if push:
        state.set_value(OFF_SWITCH if off and _has_off_switch else SWITCH, true)
    else:
        state.set_value(SWITCH, not off)
    return result
