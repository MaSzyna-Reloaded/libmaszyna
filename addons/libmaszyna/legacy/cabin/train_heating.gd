extends RefCounted
class_name LegacyCabinTrainHeating

## Train heating switch (trainheating_sw) - TTrain::OnCommand_heatingtoggle/enable/disable
## (Train.cpp:6658-6717). A press flips HeatingAllow; a push-type switch (type: return,
## dynamic/pkp/e186_v2) springs back when released and switches nothing then. A cab without the
## gauge takes no key for it (MmdSemanticCatalog `requires_gauge`, Train.cpp:6661).

const CONTROL:StringName = &"trainheating_sw"

var _cabin:RID


func control_ids() -> Array[StringName]:
    return [CONTROL]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _heating)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _heating)


func _heating(state:CabinState, action:StringName, value:Variant) -> Variant:
    if action == &"release":
        state.set_value(CONTROL, false)
        return null
    if not action in [&"hold", &"toggle", &"set"]:
        return null
    # Train.cpp:6674 - a press allows the heating when it is not allowed, and the other way round
    var allowed:bool = (not state.vehicle_state_value("heating_allowed", false)
            if value == null or action == &"hold" else bool(value))
    state.set_value(CONTROL, true if action == &"hold" else allowed)
    return state.send_vehicle_command("heating", allowed)
