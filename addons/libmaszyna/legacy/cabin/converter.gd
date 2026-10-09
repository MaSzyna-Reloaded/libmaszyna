extends RefCounted
class_name LegacyCabinConverter

## The converter switch and its off switch, ported from the original cab layer:
## TTrain::OnCommand_convertertoggle/converterenable/converterdisable (Train.cpp:4382-4458). A
## two-state switch turns the converter on when it shows off; an impulse one (Switches:
## Converter=impulse) turns it on while there is no 110 V and no converter overload relay open in
## the train, only with the controlled vehicle's main circuit closed, and springs back on release.

const SWITCH:StringName = &"converter_sw"
const OFF_SWITCH:StringName = &"converteroff_sw"

var _impulse:bool
var _has_off_switch:bool
var _cabin:RID
var _toggle_handler:Callable = _toggle
var _off_handler:Callable = _off


func _init(impulse:bool, has_off_switch:bool) -> void:
    _impulse = impulse
    _has_off_switch = has_off_switch


func control_ids() -> Array[StringName]:
    return [SWITCH, OFF_SWITCH]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, SWITCH, _toggle_handler)
    CabinSystem.register_control(cabin, OFF_SWITCH, _off_handler)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, SWITCH, _toggle_handler)
    CabinSystem.unregister_control(_cabin, OFF_SWITCH, _off_handler)


## OnCommand_convertertoggle (Train.cpp:4382-4409)
func _toggle(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if action == &"release":
        _spring_back(state)
        return null
    var on:bool = (not _overload_relay_open(state) and not bool(state.vehicle_state_value("power110_available", false))
            if _impulse else not bool(state.get_value(SWITCH, false)))
    if not on:
        return _switch_off(state)
    # OnCommand_converterenable (Train.cpp:4411-4427): an impulse switch needs the main circuit
    state.set_value(SWITCH, true)
    if _impulse and not bool(state.vehicle_state_value("main_switch_enabled", false, CabinState.Target.CONTROLLED)):
        return null
    return state.send_vehicle_command("converter", true)


## OnCommand_converterdisable (Train.cpp:4429-4446)
func _off(state:CabinState, action:StringName, _value:Variant) -> Variant:
    if action == &"release":
        _spring_back(state)
        return null
    return _switch_off(state)


func _switch_off(state:CabinState) -> Variant:
    state.set_value(SWITCH, false)
    if _has_off_switch:
        state.set_value(OFF_SWITCH, true)
    return state.send_vehicle_command("converter", false)


## An impulse switch's positions back at the start on release (Train.cpp:4401-4407)
func _spring_back(state:CabinState) -> void:
    if _impulse:
        state.set_value(SWITCH, false)
        state.set_value(OFF_SWITCH, false)


## Whether a converter overload relay of the train is open (Driver IsAnyConverterOverloadRelayOpen,
## Train.cpp:4386) - over the cars under control
func _overload_relay_open(state:CabinState) -> bool:
    for car:RID in RailVehicleServer.vehicle_get_coupled(
            state.vehicle_rid, RailVehicleController.COUPLER_END_FRONT, RailVehicleController.COUPLING_FLAG_CONTROL):
        if bool(CabinSystem.vehicle_state_value(car, "converter_overload", false)):
            return true
    return false
