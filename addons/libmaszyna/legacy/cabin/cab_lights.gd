extends RefCounted
class_name LegacyCabinCabLights

## The lights of the cab itself, ported from the original cab layer: the cab light
## (TTrain::OnCommand_interiorlightenable/disable, Train.cpp:5244-5280), its dimmer
## (TTrain::OnCommand_interiorlightdimenable/disable, Train.cpp:6291-6340), the instrument light
## (TTrain::InstrumentLightActive) of the kind the cab's lamp label says, and the dashboard and
## timetable lights (Train.cpp:6537-6660, 9562-9574). Every cab keeps its own - a light left on stays in the cab it
## was switched on in - and the level it shines at follows the power of the vehicle
## (TTrain::Update(), Train.cpp:8436-8453).

const CAB_LIGHT:StringName = &"cablight_sw"
const CAB_LIGHT_DIM:StringName = &"cablightdim_sw"
const INSTRUMENT_LIGHT:StringName = &"instrumentlight_sw"
const DASHBOARD_LIGHT:StringName = &"dashboardlight_sw"
const TIMETABLE_LIGHT:StringName = &"timetablelight_sw"

## What powers the instrument light and what switches it - the cab's i-instrumentlight label
## (InstrumentLightType, Train.cpp:11755-11779, 9562-9572)
enum InstrumentLightType {
    ## i-instrumentlight: - the low voltage, its switch
    STANDARD,
    ## i-instrumentlight_m: - the controlled vehicle's main circuit, its switch
    MAINS,
    ## i-instrumentlight_c: - the 110 V converter, its switch
    CONVERTER,
    ## i-instrumentlight_a: - the low voltage, always on
    ALWAYS,
    ## i-instrumentlight_l: - the low voltage, on with any head light
    HEAD_LIGHTS,
}
## A dimmed cab light's level (cablightlevel, Train.cpp:9745)
const DIMMED_LEVEL:float = 0.4
## The cab light's level fed without the 110 V converter (cablightlevel, Train.cpp:9745)
const LOW_VOLTAGE_LEVEL:float = 0.5

var _cab_light:Callable = _switch.bind(CAB_LIGHT)
var _cab_light_dim:Callable = _switch.bind(CAB_LIGHT_DIM)
var _instrument_light:Callable = _switch.bind(INSTRUMENT_LIGHT)
var _dashboard_light:Callable = _switch.bind(DASHBOARD_LIGHT)
var _timetable_light:Callable = _switch.bind(TIMETABLE_LIGHT)
var _instrument_light_type:InstrumentLightType
var _cabin:RID


func _init(instrument_light_type:InstrumentLightType) -> void:
    _instrument_light_type = instrument_light_type


func control_ids() -> Array[StringName]:
    return [CAB_LIGHT, CAB_LIGHT_DIM, INSTRUMENT_LIGHT, DASHBOARD_LIGHT, TIMETABLE_LIGHT]


func register(_vehicle_rid:RID, cabin:RID) -> void:
    _cabin = cabin
    CabinSystem.register_control(cabin, CAB_LIGHT, _cab_light)
    CabinSystem.register_control(cabin, CAB_LIGHT_DIM, _cab_light_dim)
    CabinSystem.register_control(cabin, INSTRUMENT_LIGHT, _instrument_light)
    CabinSystem.register_control(cabin, DASHBOARD_LIGHT, _dashboard_light)
    CabinSystem.register_control(cabin, TIMETABLE_LIGHT, _timetable_light)
    CabinSystem.register_process(cabin, _process)


func unregister() -> void:
    CabinSystem.unregister_control(_cabin, CAB_LIGHT, _cab_light)
    CabinSystem.unregister_control(_cabin, CAB_LIGHT_DIM, _cab_light_dim)
    CabinSystem.unregister_control(_cabin, INSTRUMENT_LIGHT, _instrument_light)
    CabinSystem.unregister_control(_cabin, DASHBOARD_LIGHT, _dashboard_light)
    CabinSystem.unregister_control(_cabin, TIMETABLE_LIGHT, _timetable_light)
    CabinSystem.unregister_process(_cabin, _process)


func _switch(state:CabinState, action:StringName, value:Variant, control:StringName) -> Variant:
    state.set_value(control, state.is_pressed(control, action, value))
    _light(state)
    return null


func _process(state:CabinState, _delta:float) -> void:
    _light(state)


## The cab's lights as its switches and the vehicle's power leave them - lit only while 24 V or
## 110 V is there, the cab light at half without the 110 V converter (Train.cpp:8436-8453)
func _light(state:CabinState) -> void:
    var power_supply:RailVehiclePowerSupply = RailVehicleServer.vehicle_component_get(
            state.vehicle_rid, RailVehicleComponentType.COMPONENT_POWER_SUPPLY) as RailVehiclePowerSupply
    var power110:bool = power_supply and power_supply.get_power110_available()
    var powered:bool = power110 or (power_supply and power_supply.get_power24_available())
    var level:float = 0.0
    if powered and bool(state.get_value(CAB_LIGHT, false)):
        level = ((DIMMED_LEVEL if state.get_value(CAB_LIGHT_DIM, false) else 1.0)
                * (1.0 if power110 else LOW_VOLTAGE_LEVEL))
    CabinSystem.cabin_set_light_level(_cabin, level)
    # the instrument, dashboard and timetable lights share the instrument light's power (Train.cpp:9562-9574)
    var light_power:bool = powered
    var instrument_light:bool = bool(state.get_value(INSTRUMENT_LIGHT, false))
    match _instrument_light_type:
        InstrumentLightType.MAINS:
            var engine:RailVehicleEngine = state.vehicle_component(
                    VehicleComponentType.COMPONENT_ENGINE, CabinState.Target.CONTROLLED) as RailVehicleEngine
            light_power = engine != null and engine.get_main_switch_enabled()
        InstrumentLightType.CONVERTER:
            light_power = power110
        InstrumentLightType.ALWAYS:
            instrument_light = true
        InstrumentLightType.HEAD_LIGHTS:
            var lighting:RailVehicleLighting = state.vehicle_component(VehicleComponentType.COMPONENT_LIGHTING) as RailVehicleLighting
            instrument_light = lighting != null and lighting.get_any_light_enabled()
    CabinSystem.cabin_set_instrument_light_enabled(_cabin, light_power and instrument_light)
    CabinSystem.cabin_set_dashboard_light_enabled(
            _cabin, light_power and bool(state.get_value(DASHBOARD_LIGHT, false)))
    CabinSystem.cabin_set_timetable_light_enabled(
            _cabin, light_power and bool(state.get_value(TIMETABLE_LIGHT, false)))
