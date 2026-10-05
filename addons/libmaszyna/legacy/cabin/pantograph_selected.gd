extends RefCounted
class_name LegacyCabinPantographSelected

## Raise/lower of the selected pantographs through their master valve (pantselected_sw, and
## pantselectedoff_sw where a cab has a separate lower button) - TTrain::
## OnCommand_pantographtoggleselected/raiseselected/lowerselected (Train.cpp:3403-3510).
##
## pantselected_sw alone toggles: a press lowers the pantographs when the valve is open and raises
## them otherwise. How the valve is operated depends on the kind of switch - a two-state one sets
## it (ENABLE/DISABLE), an impulse one presses one side of it (ENABLE_ON/DISABLE_ON) and lets go
## on release (NONE). With pantselectedoff_sw the two buttons raise and lower each on their own.
## Which pantographs are selected is LegacyCabinPantographPresets'.

## The pantographs' master valve is their carrier's (OperatePantographsValve() sent along the couplers)
const TARGET:CabinState.Target = CabinState.Target.PANTOGRAPH_UNIT
const RAISE:StringName = &"pantselected_sw"
const LOWER:StringName = &"pantselectedoff_sw"
## The lever's pose: up raising, down lowering, and an impulse one doing both rests midway
## (Train.cpp:3455, 3474, 3496)
const LEVER_DOWN:float = 0.0
const LEVER_UP:float = 1.0
const LEVER_REST:float = 0.5

var _raise_button_type:CabinButton.ButtonType
var _lower_button_type:CabinButton.ButtonType
## Train.cpp:3429 - m_controlmapper.contains("pantselectedoff_sw:")
var _has_lower_button:bool
var _presets:LegacyCabinPantographPresets
var _cabin:RID


func _init(raise_button_type:CabinButton.ButtonType, lower_button_type:CabinButton.ButtonType,
        has_lower_button:bool, presets:LegacyCabinPantographPresets) -> void:
    _raise_button_type = raise_button_type
    _lower_button_type = lower_button_type
    _has_lower_button = has_lower_button
    _presets = presets


func control_ids() -> Array[StringName]:
    return [RAISE, LOWER]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, RAISE, _raise)
    CabinSystem.register_control(cabin, LOWER, _lower)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, RAISE, _raise)
    CabinSystem.unregister_control(_cabin, LOWER, _lower)


# Train.cpp:3403 OnCommand_pantographtoggleselected
func _raise(state:CabinState, action:StringName, value:Variant) -> Variant:
    if state.is_pressed(RAISE, action, value) or action == &"toggle":
        # Train.cpp:3415 - the selection is moved there and back, which sets the valves to it
        _presets.select_next(state)
        _presets.select_previous(state)
        state.set_value(RAISE, LEVER_UP)
        if not _has_lower_button and (state.vehicle_state_value("current_collector/valve_enabled", false, TARGET)
                or state.vehicle_state_value("current_collector/valve_active", false, TARGET)):
            return _lower_selected(state)
        # Train.cpp:3472 - raise selected
        return state.send_vehicle_command("pantographs_valve_operate",
                RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE
                if _raise_button_type == CabinButton.ButtonType.TOGGLE
                else RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE_ON, null, TARGET)
    return _release(state)


# Train.cpp:3483 OnCommand_pantographlowerselected
func _lower(state:CabinState, action:StringName, value:Variant) -> Variant:
    if state.is_pressed(LOWER, action, value) or action == &"toggle":
        state.set_value(LOWER, LEVER_UP)
        return _lower_selected(state)
    return _release(state)


func _lower_selected(state:CabinState) -> Variant:
    # a single lever goes down to lower (Train.cpp:3496)
    if not _has_lower_button:
        state.set_value(RAISE, LEVER_DOWN)
    return state.send_vehicle_command("pantographs_valve_operate",
            RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE
            if _lower_button_type == CabinButton.ButtonType.TOGGLE
            else RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE_ON, null, TARGET)


# Train.cpp:3427-3457 - only impulse buttons react to a release
func _release(state:CabinState) -> Variant:
    var result:Variant = null
    if _has_lower_button:
        if not _raise_button_type == CabinButton.ButtonType.TOGGLE:
            state.set_value(RAISE, LEVER_DOWN)
            result = state.send_vehicle_command(
                    "pantographs_valve_operate", RailVehicleEnginePowerSource.VALVE_OPERATION_ENABLE_OFF, null, TARGET)
        if not _lower_button_type == CabinButton.ButtonType.TOGGLE:
            state.set_value(LOWER, LEVER_DOWN)
            result = state.send_vehicle_command(
                    "pantographs_valve_operate", RailVehicleEnginePowerSource.VALVE_OPERATION_DISABLE_OFF, null, TARGET)
        return result
    if not _raise_button_type == CabinButton.ButtonType.TOGGLE:
        # one impulse switch doing both, with its neutral position midway
        state.set_value(RAISE, LEVER_REST)
        result = state.send_vehicle_command(
                "pantographs_valve_operate", RailVehicleEnginePowerSource.VALVE_OPERATION_NONE, null, TARGET)
    return result
