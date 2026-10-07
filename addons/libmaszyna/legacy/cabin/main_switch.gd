extends RefCounted
class_name LegacyCabinMainSwitch

## Line breaker (main switch), ported from the original cab layer: TTrain::
## OnCommand_linebreakeropen/close/toggle (Train.cpp:3712-3868) and the line breaker block of
## TTrain::Update() (Train.cpp:8436-8474).
##
## The breaker becomes ready to close once the closing control was held down for
## InitialCtrlDelay while MainSwitchCheck() allows it. An electric series motor vehicle with a
## main_on_bt then closes it on the button's release, every other one straight away. The closing
## control is main_on_bt, or main_sw in its "on" position - a two-state main_sw (no `type:`) stays
## there, an impulse one (dynamic/pkp/e186_v2) springs back on release. The vehicle-level
## "main_switch" command itself still acts immediately (AI, console, tests).

## The line breaker is the driven vehicle's (OnCommand_linebreaker*: mvControlled->MainSwitch())
const TARGET:CabinState.Target = CabinState.Target.CONTROLLED
const ON_BUTTON:StringName = &"main_on_bt"
const OFF_BUTTON:StringName = &"main_off_bt"
const TOGGLE_SWITCH:StringName = &"main_sw"
## The line breaker's key (M, linebreakertoggle, driverkeyboardinput.cpp:108) - a keyboard-only
## control, pressed and let go whatever switches the cab has
const KEY:StringName = &"main_switch_key"
const KEY_ACTION:StringName = &"main_switch_toggle"

## m_linebreakerstate: 0 = open, 1 = closed, 2 = ready to close
const OPEN:int = 0
const CLOSED:int = 1
const READY:int = 2
## main_sw's pose: pushed down to open, up to close, and an impulse one rests midway
## (Train.cpp:3776, 3818, 11348)
const LEVER_OPEN:float = 0.0
const LEVER_CLOSE:float = 1.0
const LEVER_REST:float = 0.5
## main_sw counts as held towards closing past this (ggMainButton.GetDesiredValue(), Train.cpp:8436)
const LEVER_HELD_THRESHOLD:float = 0.95

var _toggle_button_type:CabinButton.ButtonType
## Train.cpp:3733, 3773, 8467 - which of the three controls the cab models
var _has_on_button:bool
var _has_off_button:bool
var _has_toggle_switch:bool
var _cabin:RID


func _init(toggle_button_type:CabinButton.ButtonType, has_on_button:bool, has_off_button:bool,
        has_toggle_switch:bool) -> void:
    _toggle_button_type = toggle_button_type
    _has_on_button = has_on_button
    _has_off_button = has_off_button
    _has_toggle_switch = has_toggle_switch


func control_ids() -> Array[StringName]:
    return [ON_BUTTON, OFF_BUTTON, TOGGLE_SWITCH, KEY]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, ON_BUTTON, _on_button)
    CabinSystem.register_control(cabin, OFF_BUTTON, _off_button)
    CabinSystem.register_control(cabin, TOGGLE_SWITCH, _toggle_switch)
    CabinSystem.register_control(cabin, KEY, _key)
    CabinSystem.register_process(cabin, _process)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, ON_BUTTON, _on_button)
    CabinSystem.unregister_control(_cabin, OFF_BUTTON, _off_button)
    CabinSystem.unregister_control(_cabin, TOGGLE_SWITCH, _toggle_switch)
    CabinSystem.unregister_control(_cabin, KEY, _key)
    CabinSystem.unregister_process(_cabin, _process)


# Train.cpp:3809 OnCommand_linebreakerclose - pressing only holds the control down, closing happens
# in _process() or, for a series motor vehicle with main_on_bt, on the release.
func _on_button(state:CabinState, action:StringName, value:Variant) -> Variant:
    var pressed:bool = state.is_pressed(ON_BUTTON, action, value)
    state.set_value(ON_BUTTON, pressed)
    # Train.cpp:3815-3824 - without main_on_bt the key moves main_sw instead
    if not _has_on_button and _has_toggle_switch:
        if pressed:
            state.set_value(TOGGLE_SWITCH, LEVER_CLOSE)
        elif not _two_state():
            state.set_value(TOGGLE_SWITCH, LEVER_REST)
    if pressed:
        return null
    return _close_released(state)


# Train.cpp:3754 OnCommand_linebreakeropen - opening happens on press.
func _off_button(state:CabinState, action:StringName, value:Variant) -> Variant:
    var pressed:bool = state.is_pressed(OFF_BUTTON, action, value)
    state.set_value(OFF_BUTTON, pressed)
    # Train.cpp:3773-3786, 3796 - without main_off_bt the key moves main_sw instead
    if not _has_off_button and _has_toggle_switch:
        state.set_value(TOGGLE_SWITCH, LEVER_OPEN if pressed or _two_state() else LEVER_REST)
    if not pressed:
        return null
    return _open(state)


# Train.cpp:3712 OnCommand_linebreakertoggle - a press closes an open breaker or opens a closed one;
# only an impulse switch (or a cab that also has main_on_bt) reacts to the release.
func _toggle_switch(state:CabinState, action:StringName, value:Variant) -> Variant:
    var pressed:bool = state.is_pressed(TOGGLE_SWITCH, action, value)
    # a two-state switch reports every flip as a toggle - each of them is a press
    if pressed or (_two_state() and action == &"toggle"):
        var linebreaker_state:int = state.data.get("linebreaker_state", OPEN)
        if linebreaker_state == OPEN:
            state.set_value(TOGGLE_SWITCH, LEVER_CLOSE)
            return null
        if linebreaker_state == CLOSED:
            state.set_value(TOGGLE_SWITCH, LEVER_OPEN)
            return _open(state)
        return null
    if _two_state() and not _has_on_button:
        return null
    state.set_value(TOGGLE_SWITCH, LEVER_REST)
    if state.data.get("linebreaker_state", OPEN) == OPEN:
        return null
    return _close_released(state)


# Train.cpp:3712-3750 OnCommand_linebreakertoggle by its key: pressed and let go like a button. A
# cab whose closing control is main_sw works its lever; any other one presses main_on_bt to close
# an open breaker (OnCommand_linebreakerclose) and main_off_bt to open a closed one
# (OnCommand_linebreakeropen), and lets go of both on the release (Train.cpp:3725-3745)
func _key(state:CabinState, action:StringName, value:Variant) -> Variant:
    var pressed:bool = state.is_pressed(KEY, action, value)
    state.set_value(KEY, pressed)
    if _has_toggle_switch and not _has_on_button:
        return _toggle_switch(state, &"hold" if pressed else &"release", null)
    var linebreaker_state:int = state.data.get("linebreaker_state", OPEN)
    if pressed:
        if linebreaker_state == OPEN:
            return _on_button(state, &"hold", null)
        if linebreaker_state == CLOSED:
            return _off_button(state, &"hold", null)
        return null
    _off_button(state, &"release", null)
    return _on_button(state, &"release", null)


func _two_state() -> bool:
    return _toggle_button_type == CabinButton.ButtonType.TOGGLE


func _open(state:CabinState) -> Variant:
    state.data["relay_timer"] = 0.0
    if state.data.get("linebreaker_state", OPEN) == OPEN:
        return null
    var result:Variant = state.send_vehicle_command("main_switch", false, null, TARGET)
    if result:
        state.data["linebreaker_state"] = OPEN
    return result


# Train.cpp:3850-3866 - a series motor vehicle finishes closing on the release
func _close_released(state:CabinState) -> Variant:
    var result:Variant = null
    if state.data.get("linebreaker_state", OPEN) == READY \
            and int(state.vehicle_state_value("engine_type", 0, TARGET)) == RailVehicleEngine.ELECTRIC_SERIES_MOTOR:
        result = state.send_vehicle_command("main_switch", true, null, TARGET)
        state.data["linebreaker_state"] = CLOSED if result else OPEN
    state.data["relay_timer"] = 0.0
    return result


# Train.cpp:8436-8474
func _process(state:CabinState, delta:float) -> void:
    var linebreaker_state:int = state.data.get("linebreaker_state", OPEN)
    var engine:RailVehicleEngine = state.vehicle_component(VehicleComponentType.COMPONENT_ENGINE, TARGET) as RailVehicleEngine
    var mains:bool = engine and engine.get_main_switch_enabled()
    # sync with the vehicle - closed by someone else, or knocked out
    if linebreaker_state == OPEN and mains:
        linebreaker_state = CLOSED
    elif linebreaker_state == CLOSED and not mains:
        linebreaker_state = OPEN

    var relay_timer:float = state.data.get("relay_timer", 0.0)
    if state.get_value(ON_BUTTON, false) \
            or float(state.get_value(TOGGLE_SWITCH, LEVER_OPEN)) > LEVER_HELD_THRESHOLD:
        if engine and engine.get_main_switch_closable():
            relay_timer += delta
    else:
        relay_timer = 0.0
    if state.get_value(OFF_BUTTON, false):
        relay_timer = 0.0

    var electric:RailVehicleElectricEngine = engine as RailVehicleElectricEngine
    if linebreaker_state == OPEN and relay_timer > (electric.get_line_breaker_initial_delay() if electric else 0.0):
        linebreaker_state = READY
    # Train.cpp:8467 - without main_on_bt, or for anything but a series motor, closing completes here
    if linebreaker_state == READY and (not _has_on_button
            or not (engine and engine.get_type() == RailVehicleEngine.ELECTRIC_SERIES_MOTOR)):
        linebreaker_state = CLOSED if state.send_vehicle_command("main_switch", true, null, TARGET) else OPEN

    state.data["linebreaker_state"] = linebreaker_state
    state.data["relay_timer"] = relay_timer
