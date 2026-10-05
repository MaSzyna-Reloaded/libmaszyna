extends RefCounted
class_name LegacyCabinPantographsDropAll

## Lower-all-pantographs switch (pantalloff_sw) - TTrain::OnCommand_pantographlowerall
## (Train.cpp:3334). A two-state switch flips PantAllDown on a press; any other kind holds the
## pantographs down while it is held. A cab without the gauge takes no key for it
## (MmdSemanticCatalog `requires_gauge`, Train.cpp:3342).

## All pantographs down is the carrier's (OnCommand_pantographlowerall: mvPantographUnit)
const TARGET:CabinState.Target = CabinState.Target.PANTOGRAPH_UNIT
const CONTROL:StringName = &"pantalloff_sw"

var _button_type:CabinButton.ButtonType
var _cabin:RID


func _init(button_type:CabinButton.ButtonType) -> void:
    _button_type = button_type


func control_ids() -> Array[StringName]:
    return [CONTROL]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _drop_all)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _drop_all)


func _drop_all(state:CabinState, action:StringName, value:Variant) -> Variant:
    if _button_type == CabinButton.ButtonType.TOGGLE:
        if action == &"release":
            return null
        var dropped:bool = (not state.vehicle_state_value("current_collector/pantographs_dropped", false, TARGET)
                if value == null or action == &"hold" else bool(value))
        state.set_value(CONTROL, dropped)
        return state.send_vehicle_command("pantographs_drop_all", dropped, null, TARGET)
    var held:bool = state.is_pressed(CONTROL, action, value)
    state.set_value(CONTROL, held)
    return state.send_vehicle_command("pantographs_drop_all", held, null, TARGET)
