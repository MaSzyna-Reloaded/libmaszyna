@tool
extends RefCounted
class_name FizTrainEngineCommon

## Field-application helpers shared by every concrete engine parser (Diesel/DieselElectric/
## ElectricSeries/ElectricInduction) - NOT a section parser itself (no parse()/prefix), just
## the common part of Engine:'s and Cntrl.'s field sets, factored out because every concrete
## RailVehicleEngine subclass inherits these Godot properties. LoadFIZ_Engine common subset:
## Mover.cpp:11119; Cntrl. engine subset: Mover.cpp:10707.


## EngineType= decode (RailVehicleEngine.EngineType - the enum this class family owns).
## LoadFIZ_EngineDecode: Mover.cpp:11689. Used by Engine: and by other sections that reference
## an engine type (Light:/Clima: generator engine).
static func parse_engine_type(value: String, default_value: int = RailVehicleEngine.NONE) -> int:
    if not value:
        return default_value
    match value.to_lower():
        "electricseriesmotor": return RailVehicleEngine.ELECTRIC_SERIES_MOTOR
        "dieselengine": return RailVehicleEngine.DIESEL
        "steamengine": return RailVehicleEngine.STEAM
        "wheelsdriven": return RailVehicleEngine.WHEELS_DRIVEN
        "dumb": return RailVehicleEngine.DUMB
        "dieselelectric", "dumbde": return RailVehicleEngine.DIESEL_ELECTRIC
        "electricinductionmotor": return RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR
        "main": return RailVehicleEngine.MAIN
        _: return RailVehicleEngine.NONE


## Engine: fields common to every EngineType (Trans=, TransEff, motor blowers, ...).
static func apply_engine_common(node: RailVehicleEngine, kv: Dictionary, context: FizImportContext) -> void:
    if kv.has("Trans"):
        var parts: PackedStringArray = FizLineUtil.get_string(kv, "Trans").split(":")
        if parts.size() == 2:
            node.transmission_gear_teeth_motor = parts[0].to_int()
            node.transmission_gear_teeth_wheel = parts[1].to_int()
    if kv.has("TransEff"):
        node.transmission_efficiency = FizLineUtil.get_float(kv, "TransEff")
    if kv.has("Ftmax"):
        node.maximum_traction_force = FizLineUtil.get_float(kv, "Ftmax")
    if kv.has("MotorBlowersSpeed"):
        node.motor_blowers_speed = FizLineUtil.get_float(kv, "MotorBlowersSpeed")
    if kv.has("MotorBlowersSustainTime"):
        node.motor_blowers_sustain_time = FizLineUtil.get_float(kv, "MotorBlowersSustainTime")
    if kv.has("MotorBlowersStartVelocity"):
        node.motor_blowers_start_velocity = FizLineUtil.get_float(kv, "MotorBlowersStartVelocity")
    if kv.has("InvNo"):
        node.inverters_count = FizLineUtil.get_int(kv, "InvNo")

    # PressureSwitch's absent-key default (true, unless the vehicle is EZT) differs from the
    # compiled default (false).
    var pressure_switch_default: bool = context.train_type != RailVehicleController.TRAIN_TYPE_EZT
    node.pressure_switch_present = FizLineUtil.get_bool(kv, "PressureSwitch", pressure_switch_default)


## The controller-position-count subset of Cntrl. (stashed on context.cntrl_kv by
## FizTrainCntrlParser, since Cntrl. conventionally precedes Engine: in real files).
static func apply_cntrl_engine_subset(node: RailVehicleEngine, cntrl_kv: Dictionary) -> void:
    if not cntrl_kv:
        return
    if cntrl_kv.has("Camshaft"):
        node.cntrl_has_camshaft = FizLineUtil.get_bool(cntrl_kv, "Camshaft")
    if cntrl_kv.has("ScndS"):
        node.cntrl_series_shunt_on_series_position = FizLineUtil.get_bool(cntrl_kv, "ScndS")
    if cntrl_kv.has("FSCircuit"):
        node.cntrl_fast_series_circuit = FizLineUtil.get_bool(cntrl_kv, "FSCircuit")
    if cntrl_kv.has("EIMCtrlAddZeros"):
        node.cntrl_eim_control_additional_zeros = FizLineUtil.get_bool(cntrl_kv, "EIMCtrlAddZeros")
    if cntrl_kv.has("EIMCtrlEmergency"):
        node.cntrl_eim_control_emergency = FizLineUtil.get_bool(cntrl_kv, "EIMCtrlEmergency")
    # Mover.cpp:10910
    if cntrl_kv.has("MainInitTime"):
        node.main_init_time = FizLineUtil.get_float(cntrl_kv, "MainInitTime")
    if cntrl_kv.has("EIMCtrlType"):
        node.cntrl_eim_control_type = clampi(FizLineUtil.get_int(cntrl_kv, "EIMCtrlType"), 0, 3)
    if cntrl_kv.has("MotorBlowersStart"):
        node.motor_blowers_start_mode = FizTrainControllerParser.parse_start_mode(
                FizLineUtil.get_string(cntrl_kv, "MotorBlowersStart"), RailVehicleController.START_MODE_MANUAL)
    # Mover.cpp:10948-10962 - each pump's start, manual when the key is missing
    if node is RailVehicleDieselEngine and cntrl_kv.has("FuelStart"):
        (node as RailVehicleDieselEngine).fuel_pump_start_mode = FizTrainControllerParser.parse_start_mode(
                        FizLineUtil.get_string(cntrl_kv, "FuelStart"), RailVehicleController.START_MODE_MANUAL)
    if node is RailVehicleDieselEngine and cntrl_kv.has("OilStart"):
        (node as RailVehicleDieselEngine).oil_pump_start_mode = FizTrainControllerParser.parse_start_mode(
                        FizLineUtil.get_string(cntrl_kv, "OilStart"), RailVehicleController.START_MODE_MANUAL)
    if node is RailVehicleDieselEngine and cntrl_kv.has("WaterStart"):
        (node as RailVehicleDieselEngine).water_pump_start_mode = FizTrainControllerParser.parse_start_mode(
                        FizLineUtil.get_string(cntrl_kv, "WaterStart"), RailVehicleController.START_MODE_MANUAL)

    match FizLineUtil.get_string(cntrl_kv, "AutoRelay").to_lower():
        "optional": node.cntrl_auto_relay_mode = RailVehicleEngine.AUTO_RELAY_OPTIONAL
        "yes": node.cntrl_auto_relay_mode = RailVehicleEngine.AUTO_RELAY_YES


## Shared MotorParamTable0:/MotorParamTable: row parser. These are TWO DIFFERENT sections in the
## original, dispatched to two DIFFERENT reader functions purely by whether the header has a
## trailing "0" (Mover.cpp:9737 issection("MotorParamTable0:") -> startMPT0 -> readMPT0(),
## Mover.cpp:9729 issection("MotorParamTable:") -> startMPT -> readMPT() -> EngineType switch) -
## NOT the same row shape, despite the near-identical header text. Verified directly against both
## real readers (not the wiki, which marks every column "?"):
## - "MotorParamTable0:" (what ElectricSeriesMotor vehicles - e.g. 303e-ep.fiz - actually use) ->
##   readMPT0 (Mover.cpp:8948), default (non-DieselEngine) case: idx, mfi, mIsat, mfi0, fi, Isat,
##   fi0 - SIX mandatory fields, then an optional 7th auto-shunt flag (int==1). Previously
##   misread as readMPTElectricSeries's shape (below), which silently dropped mfi0/fi0 entirely
##   and read "fi" from the wrong column (0.11 instead of the real ~140) - fi is the back-EMF
##   constant Current() (Mover.cpp:271) uses to taper current/torque as motor RPM rises, so
##   reading it 1000x too small meant the motor never lost torque with speed: unbounded
##   acceleration at any fixed controller notch, confirmed against a live run of the actual
##   original executable (ammeter drops quickly and speed plateaus around 50 km/h on notch 6,
##   which this wrapper's sim could not reproduce until this fix).
## - "MotorParamTable:" (no "0" - what this wrapper's DieselElectric parser uses) -> readMPT() ->
##   readMPTElectricSeries (Mover.cpp:9004) for ElectricSeriesMotor: idx, mfi, mIsat, fi, Isat,
##   optional 5th auto-shunt flag - OR readMPTDieselElectric (Mover.cpp:9028) for DieselElectric:
##   idx, mfi, mIsat, fi, Isat, then two REQUIRED trailing columns as MPTRelay[]'s
##   shunting_up/shunting_down thresholds (p_is_diesel_electric selects this variant - this
##   wrapper has no plain-ElectricSeriesMotor caller for "MotorParamTable:" today, only
##   "MotorParamTable0:", so that reader's shape is documented here for completeness but unused).
static func parse_motor_param_row(p: MaszynaParser, p_is_diesel_electric: bool = false) -> RailVehicleMotorParameter:
    var tokens: Array = p.get_tokens(8)
    var min_tokens: int = (7 if p_is_diesel_electric else 7)
    if tokens.size() < min_tokens:
        return null
    var item := RailVehicleMotorParameter.new()
    if p_is_diesel_electric:
        item.voltage_constant_multiplier = float(tokens[1])   # mfi
        item.saturation_current_multiplier = float(tokens[2]) # mIsat
        item.voltage_constant = float(tokens[3])               # fi
        item.saturation_current = float(tokens[4])             # Isat
        item.shunting_up = float(tokens[5])
        item.shunting_down = float(tokens[6])
    else:
        item.voltage_constant_multiplier = float(tokens[1])          # mfi
        item.saturation_current_multiplier = float(tokens[2])        # mIsat
        item.initial_voltage_constant_multiplier = float(tokens[3])  # mfi0
        item.voltage_constant = float(tokens[4])                     # fi
        item.saturation_current = float(tokens[5])                   # Isat
        item.initial_voltage_constant = float(tokens[6])             # fi0
        if tokens.size() >= 8:
            item.auto_switch = (int(tokens[7]) == 1)
    return item


## A gear of a plain diesel engine (readMPTDieselEngine, Mover.cpp:9175): idx, mIsat, fi, mfi -
## the gear's ratio, and the lowest and highest speed it is driven in [km/h] - then an optional
## flag of a gear the controller passes by itself
static func parse_diesel_gear_row(p: MaszynaParser) -> RailVehicleMotorParameter:
    var tokens: Array = p.get_tokens(5)
    if tokens.size() < 4:
        return null
    var item := RailVehicleMotorParameter.new()
    item.saturation_current_multiplier = float(tokens[1]) # mIsat
    item.voltage_constant = float(tokens[2])              # fi
    item.voltage_constant_multiplier = float(tokens[3])   # mfi
    item.auto_switch = tokens.size() >= 5 and int(tokens[4]) == 1
    return item


