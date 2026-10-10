extends RefCounted
class_name LegacyCabinCompartmentLights

## The passengers' compartment lights - TTrain::OnCommand_compartmentlightstoggle/enable/disable
## (Train.cpp:6357-6460): one switch (compartmentlights_sw), and an on and an off button
## (compartmentlightson_sw, compartmentlightsoff_sw). The switch turns them on while they are
## neither switched on nor lit, off otherwise; a push switch returns to its middle on release,
## leaving the lights to themselves, and a toggle one holds its "off" side too.

## The lights are the occupied vehicle's (mvOccupied->CompartmentLightsSwitch())
const TARGET:CabinState.Target = CabinState.Target.OCCUPIED
const SWITCH:StringName = &"compartmentlights_sw"
const ON_BUTTON:StringName = &"compartmentlightson_sw"
const OFF_BUTTON:StringName = &"compartmentlightsoff_sw"
## The switch's poses: off, its middle and on (Train.cpp:6400, 6408, 6447)
const SWITCH_OFF:float = 0.0
const SWITCH_MIDDLE:float = 0.5
const SWITCH_ON:float = 1.0

var _button_type:CabinButton.ButtonType
var _has_switch:bool
var _cabin:RID
## The buttons' handlers, each bound to its control
var _on_handler:Callable = _enable.bind(ON_BUTTON)
var _off_handler:Callable = _disable.bind(OFF_BUTTON)


func _init(button_type:CabinButton.ButtonType, has_switch:bool) -> void:
    _button_type = button_type
    _has_switch = has_switch


func control_ids() -> Array[StringName]:
    return [SWITCH, ON_BUTTON, OFF_BUTTON]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, SWITCH, _toggle)
    CabinSystem.register_control(cabin, ON_BUTTON, _on_handler)
    CabinSystem.register_control(cabin, OFF_BUTTON, _off_handler)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, SWITCH, _toggle)
    CabinSystem.unregister_control(_cabin, ON_BUTTON, _on_handler)
    CabinSystem.unregister_control(_cabin, OFF_BUTTON, _off_handler)


## compartmentlightstoggle (Train.cpp:6357-6374): on while neither switched on nor lit, else off -
## for the press and the release alike
func _toggle(state:CabinState, action:StringName, value:Variant) -> Variant:
    if not state.vehicle_state_value("compartment_lights_active", false, TARGET) \
            and not state.vehicle_state_value("compartment_lights_enabled", false, TARGET):
        return _enable(state, action, value, SWITCH)
    return _disable(state, action, value, SWITCH)


## compartmentlightsenable (Train.cpp:6376-6416), by the switch or the on button `control_id`
func _enable(state:CabinState, action:StringName, value:Variant, control_id:StringName) -> Variant:
    if state.is_pressed(control_id, action, value):
        var result:Variant = state.send_vehicle_command("compartment_lights", true, null, TARGET)
        if _has_switch and _button_type & CabinButton.ButtonType.TOGGLE:
            state.send_vehicle_command("compartment_lights_switch_off", false, null, TARGET)
        state.set_value(SWITCH, SWITCH_ON)
        state.set_value(ON_BUTTON, true)
        return result
    if _has_switch and _button_type == CabinButton.ButtonType.PUSH:
        state.send_vehicle_command("compartment_lights", false, null, TARGET)
        state.send_vehicle_command("compartment_lights_switch_off", false, null, TARGET)
        state.set_value(SWITCH, SWITCH_MIDDLE)
    if state.get_value(ON_BUTTON, false):
        state.send_vehicle_command("compartment_lights", false, null, TARGET)
        state.set_value(ON_BUTTON, false)
    return null


## compartmentlightsdisable (Train.cpp:6418-6460), by the switch or the off button `control_id`
func _disable(state:CabinState, action:StringName, value:Variant, control_id:StringName) -> Variant:
    if state.is_pressed(control_id, action, value):
        var result:Variant = state.send_vehicle_command("compartment_lights_switch_off", true, null, TARGET)
        if _has_switch and _button_type & CabinButton.ButtonType.TOGGLE:
            state.send_vehicle_command("compartment_lights", false, null, TARGET)
        state.set_value(SWITCH, SWITCH_OFF)
        state.set_value(OFF_BUTTON, true)
        return result
    if _has_switch and _button_type == CabinButton.ButtonType.PUSH:
        state.send_vehicle_command("compartment_lights", false, null, TARGET)
        state.send_vehicle_command("compartment_lights_switch_off", false, null, TARGET)
        state.set_value(SWITCH, SWITCH_MIDDLE)
    if state.get_value(OFF_BUTTON, false):
        state.send_vehicle_command("compartment_lights_switch_off", false, null, TARGET)
        state.set_value(OFF_BUTTON, false)
    return null
