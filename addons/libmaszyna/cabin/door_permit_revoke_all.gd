extends RefCounted
class_name MaszynaCabinDoorPermitRevoke

## Force-revoking both door permits at once, a keyboard-only control with no original: closing the
## doors already revokes a permit (Mover.cpp:8745-8749), and a push permit switch only grants
## (Train.cpp:7213-7220) - this gives every vehicle one key that always clears both sides, and a
## hint with an actual keycap where the per-side revoking hints have none (MaszynaDriverHints).

const CONTROL:StringName = &"doors_permit_revoke_all"
const ACTION:StringName = &"doors_permit_revoke_all"

var _cabin:RID


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _revoke)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _revoke)


func _revoke(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if not action == &"hold":
        return null
    # Train.cpp:7203 - the presets permit the doors, manual permit controls do nothing
    if int(state.vehicle_state_value("doors_permit_preset_count", 0)) > 0:
        return null
    state.send_vehicle_command("doors_left_permit", false)
    return state.send_vehicle_command("doors_right_permit", false)
