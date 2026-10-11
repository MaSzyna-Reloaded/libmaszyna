@tool
extends RefCounted
class_name FizTrainElectricSeriesEngineParser

## RailVehicleElectricSeriesEngine's own subset of Engine: (EngineType=ElectricSeriesMotor, called
## directly by FizTrainEngineParser once it creates the node), plus Circuit:, RList:+rows, and
## MotorParamTable0:+rows (all registered directly in FizVehicleBuilder's section
## table, using the standard parse()/parse_row()/end_table() interface) - all configure the
## same node created by FizTrainEngineParser. Series-motor branch of LoadFIZ_Engine:
## Mover.cpp:11119, LoadFIZ_Circuit: Mover.cpp:11416, LoadFIZ_RList: Mover.cpp:11444,
## readRList: Mover.cpp:9095, readMPT0: Mover.cpp:8948.
##
## MotorParamTable0: column mapping - see FizTrainEngineCommon.parse_motor_param_row's own doc
## comment for the authoritative order (idx, mfi, mIsat, mfi0, fi, Isat, fi0, [auto-shunt flag]),
## verified directly against readMPT0 (Mover.cpp:8948) - the reader this header text actually
## dispatches to, NOT readMPTElectricSeries (a different, 4-column reader for the separate,
## unsuffixed "MotorParamTable:" header) - rather than the wiki's own "?"-marked column names.

var _relay_rows: Array[RailVehicleRelayListItem] = []
## A MotorParamTable: row's tokens before the optional AutoSwitch (readMPTElectricSeries, Mover.cpp:9151)
const MOTOR_PARAM_ROW_TOKENS: int = 5
var _motor_param_rows: Array[RailVehicleMotorParameter] = []
var _active_table: String = ""


func create_node() -> RailVehicleElectricSeriesEngine:
    return MoverRailVehicleElectricSeriesEngine.new()


## The series-motor-specific subset of Engine:'s key/value set (common fields already applied
## by FizTrainEngineCommon via FizTrainEngineParser).
func apply_engine_fields(kv: Dictionary, node: RailVehicleElectricSeriesEngine) -> void:
    if kv.has("Volt"):
        node.nominal_voltage = FizLineUtil.get_float(kv, "Volt")
    if kv.has("WindingRes"):
        # floors to 0.01 when the key resolves to 0, matching the original's division-by-zero guard.
        node.winding_resistance = maxf(FizLineUtil.get_float(kv, "WindingRes"), 0.01)
    if kv.has("nmax"):
        # max_rpm holds raw RPM, matching its name and the FIZ's own "nmax" units -
        # RailVehicleElectricSeriesEngine::_do_update_internal_mover does the RPM->rev/s conversion
        # (p_mover->nmax = max_rpm/60.0, mirroring the original's own LoadFIZ_Engine "nmax /= 60.0",
        # Mover.cpp:10884). Dividing here too silently double-converted it (3600x too small),
        # making Mover's own motor-overspeed damage check (Mover.cpp:446, FuzzyLogic(abs(enrot),
        # nmax*1.11, ...)) fire at a tiny fraction of a km/h instead of near the real max speed -
        # this is what caused the "wylacznik szybki wybija po paru sekundach" bug.
        node.max_rpm = FizLineUtil.get_float(kv, "nmax")


## Standard section-parser interface, used for "Circuit:", "RList:", "MotorParamTable0:" and a series
## motor's "MotorParamTable:" (FizTrainEngineParser picks it by the engine type)
## (registered directly against this instance in FizVehicleBuilder's section table).
func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    if prefix == "Circuit:":
        _parse_circuit(FizLineUtil.read_key_values(p), context)
    elif prefix == "RList:":
        _active_table = "RList"
        _relay_rows = []
        _parse_rlist_header(FizLineUtil.read_key_values(p), context)
    elif prefix == "MotorParamTable0:":
        _active_table = "MotorParamTable0"
        _motor_param_rows = []
    elif prefix == "MotorParamTable:":
        _active_table = "MotorParamTable"
        _motor_param_rows = []


func _get_node(context: FizImportContext) -> RailVehicleElectricSeriesEngine:
    var node: VehicleComponent = context.get_part("RailVehicleEngine")
    return node as RailVehicleElectricSeriesEngine


func _parse_circuit(kv: Dictionary, context: FizImportContext) -> void:
    var node := _get_node(context)
    if node == null:
        return
    if kv.has("CircuitRes"):
        node.circuit_resistance = FizLineUtil.get_float(kv, "CircuitRes")
    if kv.has("ImaxLo"):
        node.circuit_imax_low = FizLineUtil.get_int(kv, "ImaxLo")
    if kv.has("ImaxHi"):
        node.circuit_imax_high = FizLineUtil.get_int(kv, "ImaxHi")
    if kv.has("IminLo"):
        node.circuit_imin_low = FizLineUtil.get_int(kv, "IminLo")
    if kv.has("IminHi"):
        node.circuit_imin_high = FizLineUtil.get_int(kv, "IminHi")
    if kv.has("TUHEX_Sum"):
        node.circuit_tuhex_sum = FizLineUtil.get_float(kv, "TUHEX_Sum")
    if kv.has("TUHEX_Diff"):
        node.circuit_tuhex_diff = FizLineUtil.get_float(kv, "TUHEX_Diff")
    if kv.has("TUHEX_MaxIw"):
        node.circuit_tuhex_max_current = FizLineUtil.get_float(kv, "TUHEX_MaxIw")
    if kv.has("TUHEX_MinIw"):
        node.circuit_tuhex_min_current = FizLineUtil.get_float(kv, "TUHEX_MinIw")
    if kv.has("TUHEX_Stages"):
        node.circuit_tuhex_stages = FizLineUtil.get_int(kv, "TUHEX_Stages")
    if kv.has("TUHEX_Sum1"):
        node.circuit_tuhex_sum_1 = FizLineUtil.get_float(kv, "TUHEX_Sum1")
    if kv.has("TUHEX_Sum2"):
        node.circuit_tuhex_sum_2 = FizLineUtil.get_float(kv, "TUHEX_Sum2")
    if kv.has("TUHEX_Sum3"):
        node.circuit_tuhex_sum_3 = FizLineUtil.get_float(kv, "TUHEX_Sum3")


func _parse_rlist_header(kv: Dictionary, context: FizImportContext) -> void:
    var node := _get_node(context)
    if node == null:
        return
    var vent_str: String = FizLineUtil.get_string(kv, "RVent").to_lower()
    match vent_str:
        "automatic": node.resistor_fan_type = RailVehicleElectricSeriesEngine.FAN_TYPE_AUTOMATIC
        "yes": node.resistor_fan_type = RailVehicleElectricSeriesEngine.FAN_TYPE_YES
    if vent_str == "automatic" or vent_str == "yes":
        if kv.has("RVentnmax"):
            node.resistor_fan_max_rpm = FizLineUtil.get_float(kv, "RVentnmax") / LibMaszynaUnits.SECONDS_PER_MINUTE
        if kv.has("RVentCutOff"):
            node.resistor_fan_cutoff_resistance = FizLineUtil.get_float(kv, "RVentCutOff")
    if kv.has("RVentMinI"):
        node.resistor_fan_min_current = FizLineUtil.get_float(kv, "RVentMinI")
    if kv.has("RVentSpeed"):
        node.resistor_fan_speed = FizLineUtil.get_float(kv, "RVentSpeed")
    if kv.has("DynBrakeRes"):
        node.dynamic_brake_resistance = FizLineUtil.get_float(kv, "DynBrakeRes")
    if kv.has("DynBrakeRes1"):
        node.dynamic_brake_resistance_1 = FizLineUtil.get_float(kv, "DynBrakeRes1")
    if kv.has("DynBrakeRes2"):
        node.dynamic_brake_resistance_2 = FizLineUtil.get_float(kv, "DynBrakeRes2")


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    match _active_table:
        "RList": _parse_rlist_row(p)
        "MotorParamTable0": _parse_motor_param_row(p)
        "MotorParamTable": _parse_series_motor_param_row(p)


func _parse_rlist_row(p: MaszynaParser) -> void:
    var tokens: Array = p.get_tokens(6)
    if tokens.size() < 5:
        return
    var item := RailVehicleRelayListItem.new()
    item.relay_position = int(tokens[0])
    item.resistance = float(tokens[1])
    item.branch_count = int(tokens[2])
    item.motors_per_branch = int(tokens[3])
    item.auto_switch = String(tokens[4]) == "1" or String(tokens[4]).to_lower() == "yes"
    if tokens.size() >= 6:
        item.shunt_index = int(tokens[5])
    _relay_rows.append(item)


func _parse_motor_param_row(p: MaszynaParser) -> void:
    var item := FizTrainEngineCommon.parse_motor_param_row(p)
    if item:
        _motor_param_rows.append(item)


## A row of a series motor's MotorParamTable: (readMPTElectricSeries, Mover.cpp:9147): idx, mfi,
## mIsat, fi, Isat, then an optional AutoSwitch - without the initial constants of MotorParamTable0:
func _parse_series_motor_param_row(p: MaszynaParser) -> void:
    var tokens: Array = p.get_tokens(MOTOR_PARAM_ROW_TOKENS + 1)
    if tokens.size() < MOTOR_PARAM_ROW_TOKENS:
        return
    var item := RailVehicleMotorParameter.new()
    item.voltage_constant_multiplier = float(tokens[1])    # mfi
    item.saturation_current_multiplier = float(tokens[2])  # mIsat
    item.voltage_constant = float(tokens[3])               # fi
    item.saturation_current = float(tokens[4])             # Isat
    if tokens.size() > MOTOR_PARAM_ROW_TOKENS:
        item.auto_switch = int(tokens[MOTOR_PARAM_ROW_TOKENS]) == 1
    _motor_param_rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node := _get_node(context)
    if node == null:
        _relay_rows = []
        _motor_param_rows = []
        return
    if _relay_rows:
        node.relay_list = _relay_rows
    if _motor_param_rows:
        node.motor_param_table = _motor_param_rows
    _relay_rows = []
    _motor_param_rows = []
