extends RefCounted
class_name LegacyCabinOccupiedCouplerDisconnect

## Uncoupling at the occupied cab's end, a keyboard-only control of the original cab layer:
## TTrain::OnCommand_occupiedcarcouplingdisconnect (Train.cpp:6285) uncouples at the end the cab
## faces (cab_to_end(), Train.h:216), with or without a couplingdisconnect_sw: gauge. A control of
## its own, so the AI uncouples the way a player does. The machine room (cab 0) faces no end.

const CONTROL:StringName = &"coupler_disconnect_occupied"
const ACTION:StringName = &"coupler_disconnect_occupied"

var _cabin:RID


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CONTROL, _disconnect)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CONTROL, _disconnect)


func _disconnect(state:CabinState, action:StringName, _value:Variant) -> Variant:
    var kind:RailVehicleCabinKind.Kind = RailVehicleServer.cabin_get_kind(state.cabin)
    if not action == &"hold" or kind == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_MACHINE:
        return null
    return state.send_vehicle_command("coupler_disconnect",
            RailVehicleController.COUPLER_END_REAR if kind == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR
            else RailVehicleController.COUPLER_END_FRONT)
