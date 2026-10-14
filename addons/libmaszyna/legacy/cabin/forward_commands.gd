extends RefCounted
class_name LegacyCabinForwardCommands

## Direct control -> vehicle command wiring of an MMD-built cabin: every control that has no
## dedicated cabin behaviour is registered in CabinSystem with a handler translating its
## manipulations into the vehicle command it is wired to. The wiring (command, command_param,
## controller mode, ...) is the control's MmdSemanticCatalog entry, as the cab's MMD lists it
## (LegacyCabinControls) - not read off the widgets, which a cab only the AI drives does not have.

var _controls:LegacyCabinControls
var _skip:Array[StringName] = []
var _cabin:RID
var _handlers:Dictionary = {}


func _init(controls:LegacyCabinControls, skip:Array[StringName]) -> void:
    _controls = controls
    _skip = skip


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    for control_id:StringName in _controls.get_control_ids():
        if control_id in _skip:
            continue
        var wiring:Dictionary = _controls.wiring(control_id)
        if not wiring:
            continue
        wiring["control_id"] = control_id
        var handler:Callable = _handle.bind(wiring)
        _handlers[control_id] = handler
        CabinSystem.register_control(cabin, control_id, handler)


func unregister() -> void:
    for control_id:StringName in _handlers:
        CabinSystem.unregister_control(_cabin, control_id, _handlers[control_id])
    _handlers.clear()


## The wiring of a control of the catalog class, by the fields of its catalog entry - the fields a
## widget of that class would have been given (MmdCabinInstancer._build_widget) - and the vehicle of
## the cab its commands go to; a gauge has none
static func wiring(
    widget_class:Variant, fields:Dictionary, target:CabinState.Target = CabinState.Target.OCCUPIED
) -> Dictionary:
    if widget_class == CabinButton:
        return {"kind": &"button", "command": fields.get("command", ""),
                "command_param": fields.get("command_param"),
                "controller_mode": fields.get("controller_mode", CabinButton.ControllerMode.OnOff), "target": target}
    if widget_class == CabinSwitch:
        return {"kind": &"switch", "command_increase": fields.get("command_increase", ""),
                "command_decrease": fields.get("command_decrease", ""),
                "command_set": fields.get("command_set", ""),
                "position_commands": fields.get("position_commands", {}), "target": target}
    if widget_class == CabinKnob:
        return {"kind": &"knob", "command": fields.get("command", ""),
                "command_increase": fields.get("command_increase", ""),
                "command_decrease": fields.get("command_decrease", ""), "target": target}
    return {}


static func _handle(state:CabinState, action:StringName, value:Variant, wiring:Dictionary) -> Variant:
    match wiring["kind"]:
        &"button":
            return _handle_button(state, action, value, wiring)
        &"switch":
            return _handle_switch(state, action, value, wiring)
        &"knob":
            # a step of a knob that steps (a key press) is the vehicle's own command
            var step_command:String = (
                    wiring["command_increase"] if action == &"increase"
                    else wiring["command_decrease"] if action == &"decrease" else "")
            if step_command:
                return state.send_vehicle_command(step_command, null, null, wiring["target"])
            state.set_value(wiring["control_id"], value)
            if wiring["command"]:
                return state.send_vehicle_command(wiring["command"], value, null, wiring["target"])
    return null


static func _handle_button(state:CabinState, action:StringName, value:Variant, wiring:Dictionary) -> Variant:
    # toggle without a value (e.g. from the console) flips the current position
    if action == &"toggle" and value == null:
        value = not state.get_value(wiring["control_id"], false)
    var pressed:bool = action == &"hold" or (action == &"toggle" and bool(value))
    state.set_value(wiring["control_id"], pressed)
    var command:String = wiring["command"]
    if not command:
        return null
    match int(wiring["controller_mode"]):
        CabinButton.ControllerMode.OnOff:
            if not wiring["command_param"] == null:
                return state.send_vehicle_command(command, wiring["command_param"], pressed, wiring["target"])
            return state.send_vehicle_command(command, pressed, null, wiring["target"])
        CabinButton.ControllerMode.On:
            # a button that picks one of several (speedbutton0..9) sends which one it is
            if pressed:
                var value_on:Variant = true if wiring["command_param"] == null else wiring["command_param"]
                return state.send_vehicle_command(command, value_on, null, wiring["target"])
        CabinButton.ControllerMode.Off:
            if pressed:
                return state.send_vehicle_command(command, false, null, wiring["target"])
    return null


static func _handle_switch(state:CabinState, action:StringName, value:Variant, wiring:Dictionary) -> Variant:
    var result:Variant = null
    var previous:int = int(state.get_value(wiring["control_id"], 0))
    if not value == null:
        state.set_value(wiring["control_id"], int(value))
    var step_command:String = (
            wiring["command_increase"] if action == &"increase"
            else wiring["command_decrease"] if action == &"decrease" else "")
    if step_command:
        result = state.send_vehicle_command(step_command, null, null, wiring["target"])
    if wiring["command_set"] and not value == null:
        result = state.send_vehicle_command(wiring["command_set"], int(value), null, wiring["target"])
    # a position that holds a command of its own (horn_bt: +1 horn_low, -1 horn_high): leaving it
    # releases that command, reaching it presses its own
    var position_commands:Dictionary = wiring["position_commands"]
    if position_commands and not value == null and not int(value) == previous:
        if position_commands.has(previous):
            result = state.send_vehicle_command(position_commands[previous], false, null, wiring["target"])
        if position_commands.has(int(value)):
            result = state.send_vehicle_command(position_commands[int(value)], true, null, wiring["target"])
    return result
