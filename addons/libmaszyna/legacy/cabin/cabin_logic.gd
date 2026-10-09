extends CabinLogic
class_name LegacyCabinLogic

## Cabin logic of the original engine (TTrain, Train.cpp) for one vehicle's cab. It composes the
## legacy behaviours for the cab and registers their callbacks in CabinSystem for its cabin; the
## cabin state itself stays in CabinSystem.
##
## It is no part of the 3D cab. CabinSystem holds it for the vehicle (attached by
## MaszynaLegacyVehicleSystem) and registers it for the cabin its driver sits in - the player or the
## AI (RailVehicleServer.vehicle_get_driver_cabin()) - so the AI's CabinSystem.act() does exactly what the player's controls do,
## without a single widget. What the cab has comes from its MMD (LegacyCabinControls), never from
## the widgets built of it.
##
## Behaviours with dedicated cabin logic claim their controls first; every remaining control is
## wired straight to its vehicle command by LegacyCabinForwardCommands; the catalog controls the cab
## does not have come last (LegacyCabinUnmodelledControls).
##
## The keys are the cab's, as TTrain::OnCommand_* take them whether a gauge is there or not
## (Train.cpp:3156): every control of the cab and of the catalog takes its key here, through the
## same press()/release()/increase()/decrease() a click on its widget calls - the widget only shows.

## A knob held by its key moves this much of its range a second, unless its entry says (CabinKnob's
## step)
const KNOB_KEY_SPEED:float = 1.0
## Keyboard-only controls of the behaviours, each a push button of its key
const KEYBOARD_ONLY:Dictionary[StringName, StringName] = {
    LegacyCabinOccupiedCouplerDisconnect.CONTROL: LegacyCabinOccupiedCouplerDisconnect.ACTION,
    LegacyCabinSpringBrakeShutOff.CONTROL: LegacyCabinSpringBrakeShutOff.ACTION,
    LegacyCabinBrakeCharging.CONTROL: LegacyCabinBrakeCharging.ACTION,
    LegacyCabinMainSwitch.KEY: LegacyCabinMainSwitch.KEY_ACTION,
}
## The field of a control's entry naming the key of each gesture
const GESTURE_FIELDS:Dictionary[Gesture, String] = {
    Gesture.PRESS: "action",
    Gesture.INCREASE: "action_increase",
    Gesture.DECREASE: "action_decrease",
}

## (cabin:RID) -> LegacyCabinControls - the controls of the cabin it is registered for
var _controls_for_cabin:Callable
var _behaviours:Array = []
var _unmodelled_controls:LegacyCabinUnmodelledControls
var _vehicle_rid:RID
var _cabin:RID
## action -> the control taking the key; the first control naming it keeps it
var _keys:Dictionary[String, StringName] = {}
## control_id -> {kind, target, fields} of the control as the cab has it
var _bindings:Dictionary[StringName, Dictionary] = {}
## control_id -> {rate, value} of a knob its key holds
var _held_knobs:Dictionary[StringName, Dictionary] = {}


func _init(controls_for_cabin:Callable) -> void:
    _controls_for_cabin = controls_for_cabin


## The logic of the cabs a vehicle's MMD defines
static func from_mmd(data_path:String, mmd_filename:String, skin:String, vehicle_name:String) -> LegacyCabinLogic:
    var game_dir:String = UserSettings.get_maszyna_game_dir()
    var relative_path:String = data_path.trim_prefix("/").path_join(mmd_filename + ".mmd")
    var abs_mmd_path:String = game_dir.path_join(MaszynaDataPath.resolve(game_dir, relative_path))
    var parameters:Dictionary = MmdCabinInstancer.vehicle_parameters(vehicle_name, mmd_filename, skin)
    return LegacyCabinLogic.new(LegacyCabinControls.from_mmd.bind(abs_mmd_path, parameters))


func register(vehicle_rid:RID, cabin:RID) -> void:
    _vehicle_rid = vehicle_rid
    _cabin = cabin
    var controls:LegacyCabinControls = _controls_for_cabin.call(cabin)
    var main_switch:LegacyCabinMainSwitch = LegacyCabinMainSwitch.new(
            controls.button_type(LegacyCabinMainSwitch.TOGGLE_SWITCH),
            controls.has_control(LegacyCabinMainSwitch.ON_BUTTON),
            controls.has_control(LegacyCabinMainSwitch.OFF_BUTTON),
            controls.has_control(LegacyCabinMainSwitch.TOGGLE_SWITCH))
    var claimed:Array[StringName] = main_switch.control_ids()
    _behaviours = [main_switch]
    # controls whose original handler branches on the kind of switch (TGaugeType, CabinButton.ButtonType) - each gets the
    # type of its own control, as TTrain reads ggX.type()
    var pantograph_presets:LegacyCabinPantographPresets = LegacyCabinPantographPresets.new(
            controls.has_control(LegacyCabinPantographPresets.SELECTOR),
            controls.has_control(LegacyCabinPantographPresets.VALVES_LEVER))
    claimed.append_array(pantograph_presets.control_ids())
    _behaviours.append(pantograph_presets)
    var pantograph_selected:LegacyCabinPantographSelected = LegacyCabinPantographSelected.new(
            controls.button_type(LegacyCabinPantographSelected.RAISE),
            controls.button_type(LegacyCabinPantographSelected.LOWER),
            controls.has_control(LegacyCabinPantographSelected.LOWER), pantograph_presets)
    claimed.append_array(pantograph_selected.control_ids())
    _behaviours.append(pantograph_selected)
    var present:Dictionary[StringName, bool] = {}
    for control_id:StringName in LegacyCabinPantographs.SWITCHES:
        present[control_id] = controls.has_control(control_id)
    var pantographs:LegacyCabinPantographs = LegacyCabinPantographs.new(present, controls.has_control(LegacyCabinPantographPresets.SELECTOR))
    claimed.append_array(pantographs.control_ids())
    _behaviours.append(pantographs)
    var config:Dictionary = CabinSystem.vehicle_config(vehicle_rid)
    var doors_present:Dictionary[StringName, bool] = {}
    for control_id:StringName in LegacyCabinDoors.CONTROLS:
        doors_present[control_id] = controls.has_control(control_id)
    var switch_behaviours:Array[RefCounted] = [
        LegacyCabinBattery.new(),
        LegacyCabinCabActivation.new(),
        LegacyCabinCabLights.new(controls.instrument_light_type),
        LegacyCabinPump.new(&"fuelpump_sw", "fuel_pump", "fuel_pump_switch_off", "fuel_pump_enabled",
                controls.button_type(&"fuelpump_sw")),
        LegacyCabinPump.new(&"oilpump_sw", "oil_pump", "oil_pump_switch_off", "oil_pump_enabled",
                controls.button_type(&"oilpump_sw")),
        LegacyCabinPump.new(&"waterpump_sw", "water_pump", "water_pump_switch_off", "water_pump_enabled",
                controls.button_type(&"waterpump_sw")),
        LegacyCabinPump.new(&"motorblowersfront_sw", "motor_blowers_front", "motor_blowers_front_switch_off",
                "motor_blowers_front_enabled", controls.button_type(&"motorblowersfront_sw")),
        LegacyCabinPump.new(&"motorblowersrear_sw", "motor_blowers_rear", "motor_blowers_rear_switch_off",
                "motor_blowers_rear_enabled", controls.button_type(&"motorblowersrear_sw")),
        LegacyCabinMotorBlowersAllOff.new(controls.button_type(LegacyCabinMotorBlowersAllOff.CONTROL)),
        LegacyCabinCompartmentLights.new(controls.button_type(LegacyCabinCompartmentLights.SWITCH),
                controls.has_control(LegacyCabinCompartmentLights.SWITCH)),
        LegacyCabinTrainHeating.new(),
        LegacyCabinPantographsDropAll.new(controls.button_type(LegacyCabinPantographsDropAll.CONTROL)),
        LegacyCabinConverter.new(bool(config.get("converter_switch_impulse", false)),
                controls.has_control(LegacyCabinConverter.OFF_SWITCH)),
        LegacyCabinTempomat.new(controls.button_type(LegacyCabinTempomat.SWITCH),
                controls.has_control(LegacyCabinTempomat.OFF_SWITCH)),
        LegacyCabinDoorStep.new(controls.button_type(LegacyCabinDoorStep.SWITCH)),
        LegacyCabinDoorPermits.new(
                controls.button_type(LegacyCabinDoorPermits.LEFT_SWITCH),
                controls.button_type(LegacyCabinDoorPermits.RIGHT_SWITCH)),
        LegacyCabinDoors.new(doors_present, controls.button_type(LegacyCabinDoors.ALL_CLOSE)),
    ]
    for behaviour:RefCounted in switch_behaviours:
        claimed.append_array(behaviour.control_ids())
    _behaviours.append_array(switch_behaviours)
    var reverser:LegacyCabinReverser = LegacyCabinReverser.new()
    claimed.append_array(reverser.control_ids())
    _behaviours.append(reverser)
    if controls.has_control(LegacyCabinJointController.CONTROL):
        var joint_controller:LegacyCabinJointController = LegacyCabinJointController.new()
        claimed.append_array(joint_controller.control_ids())
        _behaviours.append(joint_controller)
    _behaviours.append(LegacyCabinForwardCommands.new(controls, claimed))
    if config.get("direction_switches_circuit_imin_high", false):
        _behaviours.append(LegacyCabinDirectionKey.new())
    if not controls.has_control(LegacyCabinManualBrake.CONTROL):
        _behaviours.append(LegacyCabinManualBrake.new())
    if not controls.has_control(LegacyCabinWipers.CONTROL):
        _behaviours.append(LegacyCabinWipers.new())
    _behaviours.append(LegacyCabinBrakeCharging.new())
    _behaviours.append(LegacyCabinOccupiedCouplerDisconnect.new())
    _behaviours.append(LegacyCabinSpringBrakeShutOff.new())
    for behaviour:RefCounted in _behaviours:
        behaviour.register(vehicle_rid, cabin)
    # last: whatever is registered by now is taken care of
    _unmodelled_controls = LegacyCabinUnmodelledControls.new(controls)
    _unmodelled_controls.register(vehicle_rid, cabin)
    _behaviours.append(_unmodelled_controls)
    for control_id:StringName in controls.get_control_ids():
        _bind(control_id, controls.wiring(control_id).get("kind", &""), controls.target(control_id),
                controls.resolved_fields(control_id, config))
    for control_id:StringName in _unmodelled_controls.get_key_control_ids():
        var entry:Dictionary = MmdSemanticCatalog.get_entry(control_id)
        var target:CabinState.Target = entry.get("target", CabinState.Target.OCCUPIED)
        var fields:Dictionary = MmdSemanticCatalog.resolve_fields(control_id, CabinButton.ButtonType.TOGGLE, config)
        _bind(control_id, LegacyCabinForwardCommands.wiring(entry["widget_class"], fields, target).get("kind", &""),
                target, fields)
    for control_id:StringName in KEYBOARD_ONLY:
        _bind(control_id, &"button", CabinState.Target.OCCUPIED, {"action": KEYBOARD_ONLY[control_id], "monostable": true})


func unregister() -> void:
    for control_id:StringName in _held_knobs.keys():
        _let_go_knob(control_id)
    for behaviour:RefCounted in _behaviours:
        behaviour.unregister()
    _behaviours.clear()
    _unmodelled_controls = null
    _keys.clear()
    _bindings.clear()


## A control takes the keys of its fields no control bound before it took
func _bind(control_id:StringName, kind:StringName, target:CabinState.Target, fields:Dictionary) -> void:
    if not kind or _bindings.has(control_id):
        return
    _bindings[control_id] = {"kind": kind, "target": target, "fields": fields}
    for field:String in LegacyCabinControls.ACTION_FIELDS:
        var action:String = fields.get(field, "")
        if action and not _keys.has(action):
            _keys[action] = control_id


## The action of a gesture's field of the control - as the cab binds it, else as the catalog has
## it for a control the cab leaves to another (mainctrl's key works jointctrl) - if a control of
## the cab takes its key
func get_action(control_id:StringName, gesture:Gesture) -> StringName:
    var fields:Dictionary = _bindings[control_id]["fields"] if _bindings.has(control_id) \
            else MmdSemanticCatalog.get_entry(control_id).get("fixed_fields", {})
    var action:String = fields.get(GESTURE_FIELDS[gesture], "")
    return StringName(action) if _keys.has(action) else &""


## The player's keys of the cab's controls - a key down is the hand on its control, a key up lets
## it go; a control stepping on key repeat (repeat_on_hold) takes the echoes too
func input(event:InputEvent) -> void:
    for action:String in _keys:
        var control_id:StringName = _keys[action]
        var fields:Dictionary = _bindings[control_id]["fields"]
        if event.is_action_pressed(action, fields.get("repeat_on_hold", false), true):
            if action == fields.get("action_increase", ""):
                increase(control_id)
            elif action == fields.get("action_decrease", ""):
                decrease(control_id)
            else:
                press(control_id)
        elif event.is_action_released(action, true):
            release(control_id)


## A monostable button is held down, any other one flipped from what it shows; a two-position
## switch goes to its other end (CabinButton, CabinSwitch)
func press(control_id:StringName) -> void:
    var binding:Dictionary = _bindings.get(control_id, {})
    var fields:Dictionary = binding.get("fields", {})
    match binding.get("kind", &""):
        &"button":
            if fields.get("monostable", false):
                CabinSystem.act(_cabin, control_id, &"hold")
                return
            var state_property:String = fields.get("state_property", "")
            CabinSystem.act(_cabin, control_id, &"toggle",
                    not bool(CabinSystem.vehicle_state_value(
                            CabinState.vehicle_of(_vehicle_rid, binding["target"]), state_property, false))
                    if state_property else null)
        &"switch":
            var max_position:int = fields.get("switch_max_position", 1)
            var min_position:int = fields.get("switch_min_position", 0)
            if max_position - min_position == 1:
                _move_switch(control_id, binding,
                        min_position if _switch_position(control_id, binding) == max_position else max_position)


## A monostable button springs back, a self-returning switch goes back to rest, a knob stops
func release(control_id:StringName) -> void:
    var binding:Dictionary = _bindings.get(control_id, {})
    var fields:Dictionary = binding.get("fields", {})
    match binding.get("kind", &""):
        &"button":
            if fields.get("monostable", false):
                CabinSystem.act(_cabin, control_id, &"release")
        &"switch":
            if fields.get("automatic_reset", false):
                _move_switch(control_id, binding, fields.get("switch_reset_position", 0))
        &"knob":
            _let_go_knob(control_id)


func increase(control_id:StringName) -> void:
    var binding:Dictionary = _bindings.get(control_id, {})
    match binding.get("kind", &""):
        &"switch":
            _move_switch(control_id, binding, _switch_position(control_id, binding) + 1)
        &"knob":
            if binding["fields"].get("key_stepped", false):
                CabinSystem.act(_cabin, control_id, &"increase")
                return
            _hold_knob(control_id, binding, binding["fields"].get("step", KNOB_KEY_SPEED))


func decrease(control_id:StringName) -> void:
    var binding:Dictionary = _bindings.get(control_id, {})
    match binding.get("kind", &""):
        &"switch":
            _move_switch(control_id, binding, _switch_position(control_id, binding) - 1)
        &"knob":
            if binding["fields"].get("key_stepped", false):
                CabinSystem.act(_cabin, control_id, &"decrease")
                return
            _hold_knob(control_id, binding, -binding["fields"].get("step", KNOB_KEY_SPEED))


## Where the switch stands: the vehicle's state behind it, else the position the cab holds for it
func _switch_position(control_id:StringName, binding:Dictionary) -> int:
    var fields:Dictionary = binding["fields"]
    var held:Variant = CabinSystem.get_control(_cabin, control_id)
    var position:Variant = fields.get("switch_position", 0) if held == null else held
    var state_property:String = fields.get("state_property", "")
    if state_property:
        position = CabinSystem.vehicle_state_value(
                CabinState.vehicle_of(_vehicle_rid, binding["target"]), state_property, position)
    return int(position)


## One step at a time within its positions; standing still sends nothing
func _move_switch(control_id:StringName, binding:Dictionary, position:int) -> void:
    var fields:Dictionary = binding["fields"]
    var current:int = _switch_position(control_id, binding)
    var moved:int = clampi(position, fields.get("switch_min_position", 0), fields.get("switch_max_position", 1))
    if moved == current:
        return
    CabinSystem.act(_cabin, control_id, &"increase" if moved > current else &"decrease", moved)


## The key holds the knob: it moves `rate` of its range a simulated second while held, from where
## the vehicle shows it - by the simulation's clock, as the vehicle it moves (standing in a pause)
func _hold_knob(control_id:StringName, binding:Dictionary, rate:float) -> void:
    if _held_knobs.has(control_id):
        _held_knobs[control_id]["rate"] = rate
        return
    var fields:Dictionary = binding["fields"]
    var value:Variant = CabinSystem.get_control(_cabin, control_id)
    var state_property:String = fields.get("state_property", "")
    if state_property:
        value = CabinSystem.vehicle_state_value(CabinState.vehicle_of(_vehicle_rid, binding["target"]), state_property, value)
    _held_knobs[control_id] = {"rate": rate, "value": fields.get("value_min", 0.0) if value == null else float(value)}
    if _held_knobs.size() == 1:
        SimulationServer.simulation_advanced.connect(_on_knobs_held)


func _let_go_knob(control_id:StringName) -> void:
    if not _held_knobs.erase(control_id) or _held_knobs:
        return
    SimulationServer.simulation_advanced.disconnect(_on_knobs_held)


func _on_knobs_held(seconds:float) -> void:
    for control_id:StringName in _held_knobs:
        var held:Dictionary = _held_knobs[control_id]
        var fields:Dictionary = _bindings[control_id]["fields"]
        var value:float = clampf(held["value"] + held["rate"] * seconds,
                fields.get("value_min", 0.0), fields.get("value_max", 1.0))
        if value == held["value"]:
            continue
        held["value"] = value
        CabinSystem.act(_cabin, control_id, &"set", value)
