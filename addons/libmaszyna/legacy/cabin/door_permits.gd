extends RefCounted
class_name LegacyCabinDoorPermits

## The door permit switches, ported from the original cab layer: TTrain::OnCommand_doorpermitleft/
## right (Train.cpp:7196-7294) and the door permit timers of TTrain::Update() (Train.cpp:8475-8487).
##
## Left and right are the driver's - from the rear cab they permit the other side of the vehicle.
## A vehicle with door permit presets ignores them, its preset switch permits the doors. What a
## press does depends on the kind of switch:
## * push - permits its side, and held for DoorsOpenWithPermitAfter opens those doors as well;
## * two-state - a press flips the permit of its side.

const LEFT_SWITCH:StringName = &"doorleftpermit_sw"
const RIGHT_SWITCH:StringName = &"doorrightpermit_sw"
## A permit timer that does not run (Train.cpp:7241)
const TIMER_STOPPED:float = -1.0
const PERMIT_COMMANDS:Dictionary[RailVehicleDoors.Side, String] = {
    RailVehicleDoors.SIDE_LEFT: "doors_left_permit",
    RailVehicleDoors.SIDE_RIGHT: "doors_right_permit",
}
const OPEN_COMMANDS:Dictionary[RailVehicleDoors.Side, String] = {
    RailVehicleDoors.SIDE_LEFT: "doors_left",
    RailVehicleDoors.SIDE_RIGHT: "doors_right",
}
## The cab's state the switches' lamps are lit by (m_doorspermitleft/right, Train.cpp:8514-8520)
const LAMP_KEYS:Dictionary[RailVehicleDoors.Side, String] = {
    RailVehicleDoors.SIDE_LEFT: "door_permit_lamp_left",
    RailVehicleDoors.SIDE_RIGHT: "door_permit_lamp_right",
}
## A permit lamp is lit on the even seconds when it blinks (wSecond % 2 < 1, Train.cpp:8515)
## state.data keys of m_doorpermittimers, by the side of the vehicle
const TIMERS:Dictionary[RailVehicleDoors.Side, String] = {
    RailVehicleDoors.SIDE_LEFT: "door_permit_timer_left",
    RailVehicleDoors.SIDE_RIGHT: "door_permit_timer_right",
}

var _left_button_type:CabinButton.ButtonType
var _right_button_type:CabinButton.ButtonType
var _vehicle_rid:RID
var _cabin:RID


func _init(left_button_type:CabinButton.ButtonType, right_button_type:CabinButton.ButtonType) -> void:
    _left_button_type = left_button_type
    _right_button_type = right_button_type


func control_ids() -> Array[StringName]:
    return [LEFT_SWITCH, RIGHT_SWITCH]


func register(vehicle_rid:RID, cabin:RID) -> void:
    _vehicle_rid = vehicle_rid
    _cabin = cabin
    CabinSystem.register_control(cabin, LEFT_SWITCH, _left_switch)
    CabinSystem.register_control(cabin, RIGHT_SWITCH, _right_switch)
    CabinSystem.register_process(cabin, _process)
    for side:RailVehicleDoors.Side in LAMP_KEYS:
        CabinSystem.state_computed_value_register(vehicle_rid, LAMP_KEYS[side], _lamp.bind(side))


func unregister() -> void:
    for side:RailVehicleDoors.Side in LAMP_KEYS:
        CabinSystem.state_computed_value_unregister(_vehicle_rid, LAMP_KEYS[side])
    CabinSystem.unregister_control(_cabin, LEFT_SWITCH, _left_switch)
    CabinSystem.unregister_control(_cabin, RIGHT_SWITCH, _right_switch)
    CabinSystem.unregister_process(_cabin, _process)


# Train.cpp:7208 - cab_to_end(): the rear cab is cab 2
func _left_switch(state:CabinState, action:StringName, value:Variant) -> Variant:
    var side:RailVehicleDoors.Side = (RailVehicleDoors.SIDE_RIGHT
            if RailVehicleServer.cabin_get_kind(_cabin) == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR else RailVehicleDoors.SIDE_LEFT)
    return _permit(state, LEFT_SWITCH, _left_button_type, side, action, value)


func _right_switch(state:CabinState, action:StringName, value:Variant) -> Variant:
    var side:RailVehicleDoors.Side = (RailVehicleDoors.SIDE_LEFT
            if RailVehicleServer.cabin_get_kind(_cabin) == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR else RailVehicleDoors.SIDE_RIGHT)
    return _permit(state, RIGHT_SWITCH, _right_button_type, side, action, value)


func _permit(state:CabinState, control:StringName, button_type:CabinButton.ButtonType,
        side:RailVehicleDoors.Side, action:StringName, value:Variant) -> Variant:
    # Train.cpp:7203 - the presets permit the doors
    if int(state.vehicle_state_value("doors_permit_preset_count", 0)) > 0:
        return null
    if button_type == CabinButton.ButtonType.PUSH:
        var pressed:bool = state.is_pressed(control, action, value)
        state.set_value(control, pressed)
        if not pressed:
            state.data[TIMERS[side]] = TIMER_STOPPED
            return null
        state.data[TIMERS[side]] = float(state.vehicle_state_value("doors_open_with_permit_after", TIMER_STOPPED))
        return state.send_vehicle_command(PERMIT_COMMANDS[side], true)
    # two-state: only a press counts, and flips the switch (Train.cpp:7225)
    if action == &"release":
        return null
    var permitted:bool = (not bool(state.get_value(control, false))
            if value == null or action == &"hold" else bool(value))
    state.set_value(control, permitted)
    return state.send_vehicle_command(PERMIT_COMMANDS[side], permitted)


# Train.cpp:8475-8487 - a push switch held long enough opens the doors of its side
func _process(state:CabinState, delta:float) -> void:
    for side:RailVehicleDoors.Side in TIMERS:
        var timer:float = state.data.get(TIMERS[side], TIMER_STOPPED)
        if timer < 0.0:
            continue
        timer -= delta
        state.data[TIMERS[side]] = timer
        if timer < 0.0:
            state.send_vehicle_command(OPEN_COMMANDS[side], true)


## m_doorspermitleft/right (Train.cpp:8514-8520): the permit of the lamp's side, lit through, or on
## the even seconds - and through while a door of that side of the train stands open, or only its
## door, as DoorsPermitLightBlinking says (IsAnyDoorOpen/IsAnyDoorOnlyOpen of the train, Driver.cpp:
## 6004-6016). The cab's left is the vehicle's left from cab 1, its right from the rear cab
## (cab_to_end()); a vehicle of the train standing the other way round has its sides swapped.
func _lamp(lamp:RailVehicleDoors.Side) -> bool:
    var doors:RailVehicleDoors = CabinSystem.vehicle_component(_vehicle_rid, VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors:
        return false
    var side:RailVehicleDoors.Side = (other_side(lamp)
            if RailVehicleServer.cabin_get_kind(_cabin) == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR else lamp)
    if not (doors.get_left_open_permit() if side == RailVehicleDoors.SIDE_LEFT else doors.get_right_open_permit()):
        return false
    var blinking:RailVehicleDoors.PermitLight = doors.permit_light_blinking
    var second:int = int(SimulationServer.time_of_day * LibMaszynaUnits.SECONDS_PER_HOUR) % LibMaszynaUnits.SECONDS_PER_MINUTE
    if second % 2 < 1 or blinking < RailVehicleDoors.PERMIT_LIGHT_FLASHING_ON_PERMISSION_WITH_STEP:
        return true
    return (blinking < RailVehicleDoors.PERMIT_LIGHT_FLASHING_ON_PERMISSION
                    and RailVehicleServer.trainset_get_doorway_open(_vehicle_rid, side)) \
            or (blinking < RailVehicleDoors.PERMIT_LIGHT_FLASHING_ALWAYS
                    and RailVehicleServer.trainset_get_door_open(_vehicle_rid, side))


static func other_side(side:RailVehicleDoors.Side) -> RailVehicleDoors.Side:
    return RailVehicleDoors.SIDE_RIGHT if side == RailVehicleDoors.SIDE_LEFT else RailVehicleDoors.SIDE_LEFT
