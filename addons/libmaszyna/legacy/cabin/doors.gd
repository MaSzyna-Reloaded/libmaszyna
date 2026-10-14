extends RefCounted
class_name LegacyCabinDoors

## The cab's door controls, ported from the original cab layer: TTrain::OnCommand_doorlocktoggle,
## doortoggleleft/right, dooropenleft/right, doorcloseleft/right, dooropenall, doorcloseall,
## doormodetoggle (Train.cpp:7087-7724) and departureannounce (Train.cpp:7899-7929), with the
## cab's door lamps (Train.cpp:8509, 9167-9172, 11642-11684, 11753-11757).
##
## Every handler takes a press and a release, as the original's commands do. Left and right are
## the driver's: opening and closing a side work the other side of the vehicle from the rear cab
## (cab_to_end()), the toggles and the gauges stay the cab's. A control the cab has no gauge for
## does nothing, as TTrain checks ggX.SubModel - a toggle key still works with either gauge of its
## side, the dedicated open and close buttons included.

const LEFT_TOGGLE:StringName = &"door_left_sw"
const RIGHT_TOGGLE:StringName = &"door_right_sw"
const LEFT_OPEN:StringName = &"doorlefton_sw"
const RIGHT_OPEN:StringName = &"doorrighton_sw"
const LEFT_CLOSE:StringName = &"doorleftoff_sw"
const RIGHT_CLOSE:StringName = &"doorrightoff_sw"
const ALL_OPEN:StringName = &"doorallon_sw"
const ALL_CLOSE:StringName = &"dooralloff_sw"
const REMOTE_MODE:StringName = &"doormode_sw"
const LOCK:StringName = &"door_signalling_sw"
const DEPARTURE_SIGNAL:StringName = &"departure_signal_bt"
const CONTROLS:Array[StringName] = [
    LEFT_TOGGLE, RIGHT_TOGGLE, LEFT_OPEN, RIGHT_OPEN, LEFT_CLOSE, RIGHT_CLOSE, ALL_OPEN, ALL_CLOSE,
    REMOTE_MODE, LOCK, DEPARTURE_SIGNAL,
]
const TOGGLES:Dictionary[RailVehicleDoors.Side, StringName] = {
    RailVehicleDoors.SIDE_LEFT: LEFT_TOGGLE,
    RailVehicleDoors.SIDE_RIGHT: RIGHT_TOGGLE,
}
const OPEN_BUTTONS:Dictionary[RailVehicleDoors.Side, StringName] = {
    RailVehicleDoors.SIDE_LEFT: LEFT_OPEN,
    RailVehicleDoors.SIDE_RIGHT: RIGHT_OPEN,
}
const CLOSE_BUTTONS:Dictionary[RailVehicleDoors.Side, StringName] = {
    RailVehicleDoors.SIDE_LEFT: LEFT_CLOSE,
    RailVehicleDoors.SIDE_RIGHT: RIGHT_CLOSE,
}
const COMMANDS:Dictionary[RailVehicleDoors.Side, String] = {
    RailVehicleDoors.SIDE_LEFT: "doors_left",
    RailVehicleDoors.SIDE_RIGHT: "doors_right",
}
## A toggle shows its side open above half way (GetDesiredValue() > 0.5, Train.cpp:7127)
const OPEN_THRESHOLD:float = 0.5
## The cab's door lamps (autolights and lights, Train.cpp:11642-11684, 11753-11756)
const DOORS_OPEN_LAMP:String = "doors_lamp"
const SIDE_OPEN_LAMPS:Dictionary[RailVehicleDoors.Side, String] = {
    RailVehicleDoors.SIDE_LEFT: "doors_left_lamp",
    RailVehicleDoors.SIDE_RIGHT: "doors_right_lamp",
}
const PERMITS_LAMP:String = "door_permits_lamp"

var _present:Dictionary[StringName, bool]
var _all_close_type:CabinButton.ButtonType
var _vehicle_rid:RID
var _cabin:RID
var _handlers:Dictionary[StringName, Callable] = {}


## `present` - whether the cab has the gauge of each of CONTROLS; `all_close_type` - the kind of the
## close-all button, which closes on its release when delayed (Train.cpp:7659)
func _init(present:Dictionary[StringName, bool], all_close_type:CabinButton.ButtonType) -> void:
    _present = present
    _all_close_type = all_close_type
    _handlers = {
        LEFT_TOGGLE: _toggle.bind(RailVehicleDoors.SIDE_LEFT),
        RIGHT_TOGGLE: _toggle.bind(RailVehicleDoors.SIDE_RIGHT),
        LEFT_OPEN: _open.bind(RailVehicleDoors.SIDE_LEFT),
        RIGHT_OPEN: _open.bind(RailVehicleDoors.SIDE_RIGHT),
        LEFT_CLOSE: _close.bind(RailVehicleDoors.SIDE_LEFT),
        RIGHT_CLOSE: _close.bind(RailVehicleDoors.SIDE_RIGHT),
        ALL_OPEN: _open_all,
        ALL_CLOSE: _close_all,
        REMOTE_MODE: _remote_mode,
        LOCK: _lock,
        DEPARTURE_SIGNAL: _departure_signal,
    }


func control_ids() -> Array[StringName]:
    return CONTROLS


func register(vehicle_rid:RID, cabin:RID) -> void:
    _vehicle_rid = vehicle_rid
    _cabin = cabin
    for control_id:StringName in _handlers:
        CabinSystem.register_control(cabin, control_id, _handlers[control_id])
    CabinSystem.state_computed_value_register(vehicle_rid, DOORS_OPEN_LAMP, _doors_open)
    for side:RailVehicleDoors.Side in SIDE_OPEN_LAMPS:
        CabinSystem.state_computed_value_register(vehicle_rid, SIDE_OPEN_LAMPS[side], _side_open.bind(side))
    CabinSystem.state_computed_value_register(vehicle_rid, PERMITS_LAMP, _permits)


func unregister() -> void:
    CabinSystem.state_computed_value_unregister(_vehicle_rid, PERMITS_LAMP)
    for side:RailVehicleDoors.Side in SIDE_OPEN_LAMPS:
        CabinSystem.state_computed_value_unregister(_vehicle_rid, SIDE_OPEN_LAMPS[side])
    CabinSystem.state_computed_value_unregister(_vehicle_rid, DOORS_OPEN_LAMP)
    for control_id:StringName in _handlers:
        CabinSystem.unregister_control(_cabin, control_id, _handlers[control_id])


# Train.cpp:7121-7194, 7420-7493
func _toggle(state:CabinState, action:StringName, value:Variant, cab_side:RailVehicleDoors.Side) -> Variant:
    if action == &"toggle":
        _toggle(state, &"hold", value, cab_side)
        return _toggle(state, &"release", value, cab_side)
    if not action in [&"hold", &"release"]:
        return null
    var open:bool = float(state.get_value(TOGGLES[cab_side], 0.0)) > OPEN_THRESHOLD \
            or float(state.get_value(OPEN_BUTTONS[cab_side], 0.0)) > OPEN_THRESHOLD
    # a two-button cab without its own close button is closed with the close-all one (Train.cpp:7139)
    var closed_elsewhere:bool = _present[ALL_CLOSE] and not _present[CLOSE_BUTTONS[cab_side]]
    if action == &"hold":
        if not open:
            return _open(state, action, value, cab_side)
        return null if closed_elsewhere else _close(state, action, value, cab_side)
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    var closing:bool = not open or (doors and doors.close_auto_close_warning and doors.get_departure_signal())
    if closing and closed_elsewhere:
        return null
    var result:Variant = _close(state, action, value, cab_side) if closing else _open(state, action, value, cab_side)
    # the dedicated buttons spring back to neutral (Train.cpp:7187-7192)
    if _present[CLOSE_BUTTONS[cab_side]]:
        state.set_value(CLOSE_BUTTONS[cab_side], 0.0)
    if _present[OPEN_BUTTONS[cab_side]]:
        state.set_value(OPEN_BUTTONS[cab_side], 0.0)
    return result


# Train.cpp:7320-7360, 7495-7536
func _open(state:CabinState, action:StringName, _value:Variant, cab_side:RailVehicleDoors.Side) -> Variant:
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors or not _driver_controlled(doors.open_method):
        return null
    var button:StringName = OPEN_BUTTONS[cab_side]
    if not _present[button] and not _present[TOGGLES[cab_side]]:
        return null
    if action == &"release":
        if _present[button]:
            state.set_value(button, 0.0)
        return null
    if not action in [&"hold", &"toggle"]:
        return null
    state.set_value(button if _present[button] else TOGGLES[cab_side], 1.0)
    return state.send_vehicle_command(COMMANDS[_vehicle_side(cab_side)], true)


# Train.cpp:7362-7418, 7538-7593 - with the automatic departure signal the doors close only when
# the button is let go, the signal sounding while it is held
func _close(state:CabinState, action:StringName, _value:Variant, cab_side:RailVehicleDoors.Side) -> Variant:
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors or not _driver_controlled(doors.close_method):
        return null
    var button:StringName = CLOSE_BUTTONS[cab_side]
    if not _present[button] and not _present[TOGGLES[cab_side]]:
        return null
    var command:String = COMMANDS[_vehicle_side(cab_side)]
    if action == &"release":
        var result:Variant = null
        if doors.close_auto_close_warning:
            state.send_vehicle_command("doors_departure_signal", false)
            result = state.send_vehicle_command(command, false)
        if _present[button]:
            state.set_value(button, 0.0)
        return result
    if not action in [&"hold", &"toggle"]:
        return null
    if _present[button]:
        state.set_value(button, 1.0)
    else:
        state.set_value(TOGGLES[cab_side], 0.0)
    if doors.close_auto_close_warning:
        return state.send_vehicle_command("doors_departure_signal", true)
    return state.send_vehicle_command(command, false)


# Train.cpp:7595-7628 - both sides of the vehicle, whatever the cab
func _open_all(state:CabinState, action:StringName, _value:Variant) -> Variant:
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors or not _driver_controlled(doors.open_method) or not _present[ALL_OPEN]:
        return null
    if action == &"release":
        state.set_value(ALL_OPEN, 0.0)
        return null
    if not action in [&"hold", &"toggle"]:
        return null
    state.set_value(ALL_OPEN, 1.0)
    state.send_vehicle_command(COMMANDS[RailVehicleDoors.SIDE_RIGHT], true)
    return state.send_vehicle_command(COMMANDS[RailVehicleDoors.SIDE_LEFT], true)


# Train.cpp:7630-7685 - a delayed button closes on its release
func _close_all(state:CabinState, action:StringName, _value:Variant) -> Variant:
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors or not _driver_controlled(doors.close_method) or not _present[ALL_CLOSE]:
        return null
    var delayed:bool = _all_close_type == CabinButton.ButtonType.PUSH_DELAYED
    var released:bool = action == &"release"
    if not released and not action in [&"hold", &"toggle"]:
        return null
    if doors.close_auto_close_warning:
        state.send_vehicle_command("doors_departure_signal", not released)
    if delayed == released:
        state.send_vehicle_command(COMMANDS[RailVehicleDoors.SIDE_RIGHT], false)
        state.send_vehicle_command(COMMANDS[RailVehicleDoors.SIDE_LEFT], false)
    if not released:
        state.set_value(LEFT_TOGGLE, 0.0)
        state.set_value(RIGHT_TOGGLE, 0.0)
    state.set_value(ALL_CLOSE, 0.0 if released else 1.0)
    return null


# Train.cpp:7717-7724 - no gauge needed; the switch shows Doors.remote_only (Train.cpp:12054)
func _remote_mode(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if not action in [&"hold", &"toggle"]:
        return null
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors:
        return null
    return state.send_vehicle_command("doors_remote_control", not doors.get_remote_only())


# Train.cpp:7087-7119 - a press only, so the lock's sound loops uninterrupted
func _lock(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if not _present[LOCK] or not action in [&"hold", &"toggle"]:
        return null
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors:
        return null
    var locked:bool = not doors.get_lock_enabled()
    state.set_value(LOCK, 1.0 if locked else 0.0)
    return state.send_vehicle_command("doors_lock", locked)


# Train.cpp:7899-7929
func _departure_signal(state:CabinState, action:StringName, value:Variant) -> Variant:
    if not _present[DEPARTURE_SIGNAL]:
        return null
    if action == &"toggle":
        _departure_signal(state, &"hold", value)
        return _departure_signal(state, &"release", value)
    if action == &"release":
        state.set_value(DEPARTURE_SIGNAL, 0.0)
        return state.send_vehicle_command("doors_departure_signal", false)
    if not action == &"hold":
        return null
    var doors:RailVehicleDoors = state.vehicle_component(VehicleComponentType.COMPONENT_DOORS) as RailVehicleDoors
    if not doors or doors.get_departure_signal():
        return null
    state.set_value(DEPARTURE_SIGNAL, 1.0)
    return state.send_vehicle_command("doors_departure_signal", true)


## The side of the vehicle a side of the cab works: the other one from the rear cab (cab_to_end())
func _vehicle_side(cab_side:RailVehicleDoors.Side) -> RailVehicleDoors.Side:
    if RailVehicleServer.cabin_get_kind(_cabin) == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR:
        return LegacyCabinDoorPermits.other_side(cab_side)
    return cab_side


## m_doors (Train.cpp:8509): a doorway of the trainset not closed, either side
func _doors_open() -> bool:
    return RailVehicleServer.trainset_get_doorway_open(_vehicle_rid, RailVehicleDoors.SIDE_LEFT) \
            or RailVehicleServer.trainset_get_doorway_open(_vehicle_rid, RailVehicleDoors.SIDE_RIGHT)


## btLampkaDoorLeft/Right (Train.cpp:9167-9168): a doorway of the trainset not closed on the cab's side
func _side_open(cab_side:RailVehicleDoors.Side) -> bool:
    return RailVehicleServer.trainset_get_doorway_open(_vehicle_rid, _vehicle_side(cab_side))


## m_doorpermits (Train.cpp:8510): a door permit of the trainset given, either side
func _permits() -> bool:
    return RailVehicleServer.trainset_get_door_permit(_vehicle_rid, RailVehicleDoors.SIDE_LEFT) \
            or RailVehicleServer.trainset_get_door_permit(_vehicle_rid, RailVehicleDoors.SIDE_RIGHT)


## Doors worked from the cab (control_t::driver or mixed, Train.cpp:7323)
static func _driver_controlled(controls:RailVehicleDoors.Controls) -> bool:
    return controls == RailVehicleDoors.CONTROLS_DRIVER or controls == RailVehicleDoors.CONTROLS_MIXED
