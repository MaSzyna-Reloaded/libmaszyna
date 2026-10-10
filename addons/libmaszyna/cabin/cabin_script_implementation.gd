class_name CabinScriptImplementation
extends ScenarioScriptCabinImplementation

## The scenario scripts' way to the cabs (maszyna.cabin): a script's manipulation goes to
## CabinSystem.act(), as the driver's hand and the AI driver's do, and what changes in a cab goes
## back to the scripts that subscribed to it.


## No disconnect: the engine drops a connection to an object that is freed, and at
## NOTIFICATION_PREDELETE this script's methods are already out of reach
func _init() -> void:
    CabinSystem.control_changed.connect(_on_control_changed)


func _on_control_changed(cabin:RID, control_id:StringName, value:Variant) -> void:
    control_changed.emit(cabin, control_id, value)


func _act(cabin:RID, control_id:StringName, action:StringName, value:Variant) -> Variant:
    return CabinSystem.act(cabin, control_id, action, value)


func _get_control(cabin:RID, control_id:StringName) -> Variant:
    return CabinSystem.get_control(cabin, control_id)


func _get_controls(cabin:RID) -> Array:
    return CabinSystem.get_controls(cabin)
