@tool
extends RefCounted
class_name FizTrainDieselElectricEngineParser

## RailVehicleDieselElectricEngine's own subset of Engine: (EngineType=DieselElectric/DumbDE, called
## directly by FizTrainEngineParser once it creates the node), plus WWList:+rows and
## MotorParamTable:+rows (both registered directly in FizVehicleBuilder's section
## table, using the standard parse()/parse_row()/end_table() interface).
##
## WWList: rows map 1:1 onto RailVehicleDieselElectricEngine.wwlist (RailVehicleWWListItem), which
## _do_update_internal_mover already pushes into the mover's DElist/SST tables - that C++ side
## was already fully wired, only the FIZ-side parser was missing (this is what blocked
## main_switch/direction/brake on any DumbDE vehicle: DElist stayed all-zero, and MainCtrlPosNo
## is derived from wwlist.size(), so it stayed at -1 with no rows at all).
##
## WWList row columns (readWWList, this repo's vendored Mover.cpp doesn't keep the original
## LoadFIZ_* loader, so this is sourced from the still-present reader function plus the
## already-written C++ side's field mapping): RPM GenPower Umax Imax, optionally followed by
## 3 more columns (shunt-mode wakeup Umin/Umax/Pmax) enabling has_shunting for that row.
##
## MotorParamTable: (the diesel/diesel-electric variant, no "0" suffix - distinct from
## MotorParamTable0:, which only ElectricSeriesMotor uses) rows share FizTrainEngineCommon's
## parse_motor_param_row() and populate the same RailVehicleEngine.motor_param_table -> mover
## MotorParam[] used by TractionForce()'s DieselElectric branch for Im (motor current). Without
## it MotorParam[] stays all-zero, which divides by zero computing Im (-> inf), which then
## zeroes Ft via the "clamp Im to tempImax" step (Mover.cpp ~5403-5423) - this is what left a
## DumbDE vehicle producing engine current but exactly zero traction force even with the WWList
## fix applied and every startup step (fuel/oil pump, main switch, direction, controller
## position) done correctly.
##
## Engine:'s diesel-electric-specific subset (LoadFIZ_Engine, Mover.cpp:11261-11285 of the
## original): Flat/ShuntMode compare to the literal "1", Vhyp/Vadd are read in km/h and kept
## in m/s, and AIM/RPMDecRate go to the diesel's own mechanical_inertia/
## mechanical_rpm_decrease_rate - AIM with a default of its own.

## Original engine: Mover.cpp:11282 extract_value(dizel_AIM, "AIM", Input, "1.25")
const DEFAULT_INERTIA: float = 1.25

var _wwlist_rows: Array[RailVehicleWWListItem] = []
var _motor_param_rows: Array[RailVehicleMotorParameter] = []
var _active_table: String = ""
## The MotorParamTable: being read is a plain diesel engine's gearbox
var _diesel: bool = false


func create_node() -> RailVehicleDieselElectricEngine:
    return MoverRailVehicleDieselElectricEngine.new()


## The diesel-electric-specific subset of Engine:'s key/value set (common fields already
## applied by FizTrainEngineCommon via FizTrainEngineParser).
func apply_engine_fields(kv: Dictionary, node: RailVehicleDieselElectricEngine) -> void:
    if kv.has("Flat"):
        # Original quirk: compares to the literal string "1", not the normal Yes/No convention.
        node.generator_voltage_flat = FizLineUtil.get_string(kv, "Flat") == "1"
    if kv.has("Vhyp"):
        node.hyperbolic_speed = FizLineUtil.get_float(kv, "Vhyp") / 3.6
    if kv.has("Vadd"):
        node.additional_speed = FizLineUtil.get_float(kv, "Vadd") / 3.6
    if kv.has("Cr"):
        node.power_correction_ratio = FizLineUtil.get_float(kv, "Cr")
    if kv.has("RelayType"):
        node.shunt_relay_type = FizLineUtil.get_int(kv, "RelayType")
    if kv.has("ShuntMode"):
        # Same literal-"1" quirk as Flat.
        node.shunt_mode_allowed = FizLineUtil.get_string(kv, "ShuntMode") == "1"
    if kv.has("HeatingRPM"):
        node.heating_rpm = FizLineUtil.get_float(kv, "HeatingRPM")
    node.mechanical_inertia = FizLineUtil.get_float(kv, "AIM", DEFAULT_INERTIA)
    if kv.has("RPMDecRate"):
        node.mechanical_rpm_decrease_rate = FizLineUtil.get_float(kv, "RPMDecRate")


## Standard section-parser interface, used for "WWList:" and "MotorParamTable:" (registered
## directly against this instance in FizVehicleBuilder's section table).
##
## "MotorParamTable:" is a plain diesel engine's gearbox as well (readMPT(), Mover.cpp:9107 - the
## reader by EngineType): its header carries the clutch (LoadFIZ_MotorParamTable, Mover.cpp:11394)
## and its rows the gears (FizTrainEngineCommon.parse_diesel_gear_row()).
func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    # the header's key=value pairs (e.g. WWList's "Size=") - rows are self-terminating either way
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    _diesel = context.engine_type == RailVehicleEngine.DIESEL
    if prefix == "MotorParamTable:" and _diesel:
        var node: RailVehicleDieselEngine = context.get_part("RailVehicleEngine") as RailVehicleDieselEngine
        if node:
            FizTrainDieselEngineParser.apply_clutch(kv, node)
    if prefix == "MotorParamTable:":
        _active_table = "MotorParamTable"
        _motor_param_rows = []
    else:
        _active_table = "WWList"
        _wwlist_rows = []


func _get_node(context: FizImportContext) -> RailVehicleDieselEngine:
    var node: VehicleComponent = context.get_part("RailVehicleEngine")
    return node as RailVehicleDieselEngine


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    match _active_table:
        "WWList": _parse_wwlist_row(p)
        "MotorParamTable": _parse_motor_param_row(p)


func _parse_wwlist_row(p: MaszynaParser) -> void:
    var tokens: Array = p.get_tokens(7)
    if tokens.size() < 4:
        return
    var item := RailVehicleWWListItem.new()
    item.rpm = float(tokens[0])
    item.max_power = float(tokens[1])
    item.max_voltage = float(tokens[2])
    item.max_current = float(tokens[3])
    if tokens.size() >= 7:
        item.has_shunting = true
        item.min_wakeup_voltage = float(tokens[4])
        item.max_wakeup_voltage = float(tokens[5])
        item.max_wakeup_power = float(tokens[6])
    _wwlist_rows.append(item)


func _parse_motor_param_row(p: MaszynaParser) -> void:
    var item: RailVehicleMotorParameter = FizTrainEngineCommon.parse_diesel_gear_row(p) if _diesel \
            else FizTrainEngineCommon.parse_motor_param_row(p, true)
    if item:
        _motor_param_rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node := _get_node(context)
    if node == null:
        _wwlist_rows = []
        _motor_param_rows = []
        return
    if _wwlist_rows:
        (node as RailVehicleDieselElectricEngine).wwlist = _wwlist_rows
    if _motor_param_rows:
        node.motor_param_table = _motor_param_rows
    _wwlist_rows = []
    _motor_param_rows = []
