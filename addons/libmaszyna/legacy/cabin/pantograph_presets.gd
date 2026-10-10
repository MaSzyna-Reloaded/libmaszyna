extends RefCounted
class_name LegacyCabinPantographPresets

## The pantograph selector and the lever that sets the pantographs' valves to it (pantselect_sw,
## pantvalves_sw, or the two buttons pantvalvesupdate_bt and pantvalvesoff_bt) - TTrain::
## change_pantograph_selection, update_pantograph_valves and OnCommand_pantographselectnext/previous,
## pantographvalvesupdate/off (Train.cpp:3375-3620).
##
## The selection is the vehicle's, one per end (RailVehicleSwitches, PantsPreset.second); the cab
## moves the one of its own end and sets the valves to the preset it selects, by the cab's ends.
## Without a valves lever a new selection sets the valves at once. A vehicle without
## RailVehicleSwitches (no Switches: in its FIZ) has no presets, and its selector does nothing.

## The valves are their carrier's, as for the individual pantograph switches (LegacyCabinPantographs)
const TARGET:CabinState.Target = CabinState.Target.PANTOGRAPH_UNIT
const SELECTOR:StringName = &"pantselect_sw"
const VALVES_LEVER:StringName = &"pantvalves_sw"
const UPDATE_BUTTON:StringName = &"pantvalvesupdate_bt"
const OFF_BUTTON:StringName = &"pantvalvesoff_bt"
## pantvalves_sw: down closes the valves, up sets them, at rest midway (Train.cpp:3575, 3587, 3610)
const LEVER_OFF:int = 0
const LEVER_UPDATE:int = 2

## Train.cpp:3384 - m_controlmapper.contains("pantselect_sw:")
var _has_selector:bool
## Train.cpp:3545 - m_controlmapper.contains("pantvalves_sw:")
var _has_valves_lever:bool
var _cabin:RID


func _init(has_selector:bool, has_valves_lever:bool) -> void:
    _has_selector = has_selector
    _has_valves_lever = has_valves_lever


func control_ids() -> Array[StringName]:
    return [SELECTOR, VALVES_LEVER, UPDATE_BUTTON, OFF_BUTTON]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, SELECTOR, _selector)
    CabinSystem.register_control(cabin, VALVES_LEVER, _valves_lever)
    CabinSystem.register_control(cabin, UPDATE_BUTTON, _update_button)
    CabinSystem.register_control(cabin, OFF_BUTTON, _off_button)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, SELECTOR, _selector)
    CabinSystem.unregister_control(_cabin, VALVES_LEVER, _valves_lever)
    CabinSystem.unregister_control(_cabin, UPDATE_BUTTON, _update_button)
    CabinSystem.unregister_control(_cabin, OFF_BUTTON, _off_button)


## The selector of the cab one position on, and back (Train.cpp:3532 change_pantograph_selection)
func select_next(state:CabinState) -> Variant:
    return _select(state, "pantograph_next_preset")


func select_previous(state:CabinState) -> Variant:
    return _select(state, "pantograph_previous_preset")


func _select(state:CabinState, command:String) -> Variant:
    # the rear cab, whose own end is the vehicle's rear (cab_to_end(), Train.h:220)
    var rear:bool = RailVehicleServer.cabin_get_kind(state.cabin) == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR
    var position_key:String = "pantograph_preset_position_rear" if rear \
            else "pantograph_preset_position_front"
    var position:Variant = state.vehicle_state_value(position_key)
    # a vehicle without RailVehicleSwitches has no presets to select
    if position == null:
        return null
    state.send_vehicle_command(command,
            RailVehicleController.COUPLER_END_REAR if rear
            else RailVehicleController.COUPLER_END_FRONT)
    var selected:int = state.vehicle_state_value(position_key, 0)
    state.set_value(SELECTOR, selected)
    # Train.cpp:3545 - a new selection sets the valves unless a lever does
    if selected == position or _has_valves_lever:
        return null
    return _update_valves(state)


# Train.cpp:3517 update_pantograph_valves
func _update_valves(state:CabinState) -> Variant:
    var rear:bool = RailVehicleServer.cabin_get_kind(state.cabin) == RailVehicleCabinKind.RAIL_VEHICLE_CABIN_REAR
    var preset:RailVehicleSwitches.PantographPreset = state.vehicle_state_value(
            "pantograph_preset_rear" if rear else "pantograph_preset_front",
            RailVehicleSwitches.PANTOGRAPH_PRESET_NONE)
    # Train.cpp:3523-3526 - the rear cab's own end is the vehicle's rear
    var front_end:RailVehicleSwitches.PantographPreset = RailVehicleSwitches.PANTOGRAPH_PRESET_OTHER_END \
            if rear else RailVehicleSwitches.PANTOGRAPH_PRESET_OWN_END
    var rear_end:RailVehicleSwitches.PantographPreset = RailVehicleSwitches.PANTOGRAPH_PRESET_OWN_END \
            if rear else RailVehicleSwitches.PANTOGRAPH_PRESET_OTHER_END
    state.send_vehicle_command("pantograph_valve_operate", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST,
            RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE if preset & front_end
            else RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE, TARGET)
    return state.send_vehicle_command("pantograph_valve_operate", RailVehicleEnginePowerSource.PANTOGRAPH_SECOND,
            RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE if preset & rear_end
            else RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE, TARGET)


# Train.cpp:3605 OnCommand_pantographvalvesoff
func _close_valves(state:CabinState) -> Variant:
    state.send_vehicle_command("pantograph_valve_operate", RailVehicleEnginePowerSource.PANTOGRAPH_FIRST,
            RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE, TARGET)
    return state.send_vehicle_command("pantograph_valve_operate", RailVehicleEnginePowerSource.PANTOGRAPH_SECOND,
            RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE, TARGET)


# Train.cpp:3375-3402 OnCommand_pantographselectnext/previous - only a cab with the selector
func _selector(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if not _has_selector:
        return null
    if action == &"increase":
        return select_next(state)
    if action == &"decrease":
        return select_previous(state)
    return null


# Train.cpp:3551, 3592 - the lever's position says which; its return to rest does nothing
func _valves_lever(state:CabinState, action:StringName, value:Variant) -> Variant:
    if value == null:
        return null
    state.set_value(VALVES_LEVER, value)
    if action == &"increase" and value == LEVER_UPDATE:
        return _update_valves(state)
    if action == &"decrease" and value == LEVER_OFF:
        return _close_valves(state)
    return null


func _update_button(state:CabinState, action:StringName, value:Variant) -> Variant:
    var pressed:bool = state.is_pressed(UPDATE_BUTTON, action, value)
    state.set_value(UPDATE_BUTTON, pressed)
    return _update_valves(state) if pressed else null


func _off_button(state:CabinState, action:StringName, value:Variant) -> Variant:
    var pressed:bool = state.is_pressed(OFF_BUTTON, action, value)
    state.set_value(OFF_BUTTON, pressed)
    return _close_valves(state) if pressed else null
