extends RefCounted
class_name LegacyCabinDoorStep

## The door step switch, ported from the original cab layer: TTrain::OnCommand_doorsteptoggle
## (Train.cpp:7692-7720) - a press flips the step permit, a delayed switch on its release; a push
## switch springs back, a two-state one shows the permit. Its lamp is the step permit
## (stategauges, Train.cpp:12018; MmdSemanticCatalog).

const SWITCH:StringName = &"doorstep_sw"

var _button_type:CabinButton.ButtonType
var _cabin:RID
var _handler:Callable = _toggle


func _init(button_type:CabinButton.ButtonType) -> void:
    _button_type = button_type


func control_ids() -> Array[StringName]:
    return [SWITCH]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, SWITCH, _handler)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, SWITCH, _handler)


func _toggle(state:CabinState, action:StringName, _value:Variant) -> Variant:
    # TGauge::is_delayed()/is_push(): the bits of the kind (Gauge.h)
    var delayed:bool = not int(_button_type) & int(CabinButton.ButtonType.DELAYED) == 0
    var push:bool = not int(_button_type) & int(CabinButton.ButtonType.PUSH) == 0
    var released:bool = action == &"release"
    var result:Variant = null
    if delayed == released:
        result = state.send_vehicle_command(
                "doors_permit_step", not bool(state.vehicle_state_value("doors_step_enabled", false)))
    if released:
        if push:
            state.set_value(SWITCH, false)
    else:
        state.set_value(SWITCH, push or bool(state.vehicle_state_value("doors_step_enabled", false)))
    return result
