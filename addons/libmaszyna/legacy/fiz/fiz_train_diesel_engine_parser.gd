@tool
extends RefCounted
class_name FizTrainDieselEngineParser

## RailVehicleDieselEngine's own subset of Engine: (apply_engine_fields(), called by
## FizTrainEngineParser once it created the node), the clutch of its gearbox's header
## (apply_clutch()), and the DList:/DMList:/HTCList:/V2NList: table sections, registered directly
## in FizVehicleBuilder's section table.
##
## DList: header keys confirmed against a real vehicle line: `DList: Size=10 Mmax=2750
## nMmax=18.3 nmax=33.3 Mnmax=2142 nominalfill=1.0 Mstand=250.0 NomFuelConsRate=220`, cross-
## checked directly against RailVehicleDieselEngine::_do_update_internal_mover's own field mapping
## (`dizel_nominalfill = nominal_fuel_dose` etc.) - all six keys map onto RailVehicleDieselEngine's
## existing throttle_table-related properties. Row format confirmed via readDList
## (Mover.cpp:8444-8456): 3 columns, `Relay R Mn` -> RailVehicleThrottlePositionItem's
## throttle_position/fuel_dose/clutch_behavior.
##
## DMList: rows -> torque_table (VehicleCurvePointItem). The C++ side already applies /60.0 to x
## (rpm -> rev/s) when pushing to the mover, matching readV2NMAXList's sibling readMPTDiesel-
## adjacent convention - so this parser pushes the raw rpm value unconverted.
##
## HTCList: rows -> torque_converter_table (VehicleCurvePointItem), no conversion either side.
##
## V2NList: rows -> vel2nmax_table (VehicleCurvePointItem) - see fiz_train_controller_instancer.gd's
## comment on this prefix for why it's implemented despite 0 occurrences in the operator's
## corpus (dizel_vel2nmax_Table has a real, confirmed consumer at Mover.cpp:7183-7185).

var _throttle_rows: Array[RailVehicleThrottlePositionItem] = []
var _torque_rows: Array[VehicleCurvePointItem] = []
var _tc_rows: Array[VehicleCurvePointItem] = []
var _v2n_rows: Array[VehicleCurvePointItem] = []
var _active_table: String = ""


## Revolutions per minute in the file, per second in the Mover (LoadFIZ_Engine, Mover.cpp:11172-11196)
const SECONDS_PER_MINUTE: float = 60.0


## Engine:'s plain DieselEngine subset (LoadFIZ_Engine, Mover.cpp:11169-11241), applied by
## FizTrainEngineParser once it created the node. Each key only when present, so the properties
## keep the Mover's own defaults.
func apply_engine_fields(kv: Dictionary, node: RailVehicleDieselEngine) -> void:
    var min_rpm: float = FizLineUtil.get_float(kv, "nmin") / SECONDS_PER_MINUTE
    node.mechanical_min_rpm = min_rpm
    # nmin_hdrive and nmin_retarder fall back to nmin when absent or zero (Mover.cpp:11177-11189)
    var hydro_drive_rpm: float = FizLineUtil.get_float(kv, "nmin_hdrive") / SECONDS_PER_MINUTE
    node.mechanical_min_rpm_hydro_drive = hydro_drive_rpm if hydro_drive_rpm > 0.0 else min_rpm
    node.mechanical_min_rpm_hydro_drive_factor = FizLineUtil.get_float(kv, "nmin_hdrive_factor") / SECONDS_PER_MINUTE
    var retarder_rpm: float = FizLineUtil.get_float(kv, "nmin_retarder") / SECONDS_PER_MINUTE
    node.mechanical_min_rpm_retarder = retarder_rpm if retarder_rpm > 0.0 else min_rpm
    node.mechanical_nominal_max_rpm = FizLineUtil.get_float(kv, "nmax") / SECONDS_PER_MINUTE
    node.mechanical_fuel_cutoff_rpm = FizLineUtil.get_float(kv, "nmax_cutoff") / SECONDS_PER_MINUTE
    if kv.has("nreg_acc"):
        node.mechanical_regulator_acceleration = FizLineUtil.get_float(kv, "nreg_acc") / SECONDS_PER_MINUTE
    if kv.has("AIM"):
        node.mechanical_inertia = FizLineUtil.get_float(kv, "AIM")
    if kv.has("RPMDecRate"):
        node.mechanical_rpm_decrease_rate = FizLineUtil.get_float(kv, "RPMDecRate")
    if kv.has("EUS"):
        node.mechanical_clutch_engage_speed = FizLineUtil.get_float(kv, "EUS")
    if kv.has("EDS"):
        node.mechanical_clutch_disengage_speed = FizLineUtil.get_float(kv, "EDS")
    if kv.has("ShuntMode"):
        # the higher gear gives more force: a ratio under 1 is its inverse (Mover.cpp:11203-11213)
        var ratio: float = FizLineUtil.get_float(kv, "ShuntMode")
        node.mechanical_shunt_mode_ratio = 1.0 / ratio if ratio > 0.0 and ratio < 1.0 else ratio
    node.torque_converter_present = FizLineUtil.get_bool(kv, "IsTC")
    if node.torque_converter_present:
        _apply_torque_converter(kv, node)
    apply_diesel_common(kv, node)


## The clutch, from the header of the gearbox's MotorParamTable: (LoadFIZ_MotorParamTable,
## Mover.cpp:11394-11406)
static func apply_clutch(kv: Dictionary, node: RailVehicleDieselEngine) -> void:
    if kv.has("minVelfullengage"):
        node.clutch_min_velocity_full_engage = FizLineUtil.get_float(kv, "minVelfullengage")
    if kv.has("engageDia"):
        node.clutch_diameter = FizLineUtil.get_float(kv, "engageDia")
    if kv.has("engageMaxForce"):
        node.clutch_max_force = FizLineUtil.get_float(kv, "engageMaxForce")
    if kv.has("engagefriction"):
        node.clutch_friction = FizLineUtil.get_float(kv, "engagefriction")


## Engine:'s keys both diesel engine kinds share (Mover.cpp:11340-11373 of the original): the
## oil pump and the cooling. EngineMaxTemperature has no field in the vendored Mover (TODO.md).
static func apply_diesel_common(kv: Dictionary, node: RailVehicleDieselEngine) -> void:
    if kv.has("OilMinPressure"):
        node.oil_pump_pressure_minimum = FizLineUtil.get_float(kv, "OilMinPressure")
    if kv.has("OilMaxPressure"):
        node.oil_pump_pressure_maximum = FizLineUtil.get_float(kv, "OilMaxPressure")
    if kv.has("HeatKW"):
        node.cooling_heat_kw = FizLineUtil.get_float(kv, "HeatKW")
    if kv.has("HeatKV"):
        node.cooling_heat_kv = FizLineUtil.get_float(kv, "HeatKV")
    if kv.has("HeatKFE"):
        node.cooling_heat_kfe = FizLineUtil.get_float(kv, "HeatKFE")
    if kv.has("HeatKFS"):
        node.cooling_heat_kfs = FizLineUtil.get_float(kv, "HeatKFS")
    if kv.has("HeatKFO"):
        node.cooling_heat_kfo = FizLineUtil.get_float(kv, "HeatKFO")
    if kv.has("HeatKFO2"):
        node.cooling_heat_kfo2 = FizLineUtil.get_float(kv, "HeatKFO2")
    if kv.has("WaterMinTemperature"):
        node.cooling_water_min_temperature = FizLineUtil.get_float(kv, "WaterMinTemperature")
    if kv.has("WaterMaxTemperature"):
        node.cooling_water_max_temperature = FizLineUtil.get_float(kv, "WaterMaxTemperature")
    if kv.has("WaterFlowTemperature"):
        node.cooling_water_flow_temperature = FizLineUtil.get_float(kv, "WaterFlowTemperature")
    if kv.has("WaterCoolingTemperature"):
        node.cooling_water_cooling_temperature = FizLineUtil.get_float(kv, "WaterCoolingTemperature")
    if kv.has("WaterShutters"):
        node.cooling_water_shutters = FizLineUtil.get_bool(kv, "WaterShutters")
    if kv.has("WaterAuxCircuit"):
        node.cooling_water_aux_circuit = FizLineUtil.get_bool(kv, "WaterAuxCircuit")
    if kv.has("WaterAuxMinTemperature"):
        node.cooling_water_aux_min_temperature = FizLineUtil.get_float(kv, "WaterAuxMinTemperature")
    if kv.has("WaterAuxMaxTemperature"):
        node.cooling_water_aux_max_temperature = FizLineUtil.get_float(kv, "WaterAuxMaxTemperature")
    if kv.has("WaterAuxCoolingTemperature"):
        node.cooling_water_aux_cooling_temperature = FizLineUtil.get_float(kv, "WaterAuxCoolingTemperature")
    if kv.has("WaterAuxShutters"):
        node.cooling_water_aux_shutters = FizLineUtil.get_bool(kv, "WaterAuxShutters")
    if kv.has("OilMinTemperature"):
        node.cooling_oil_min_temperature = FizLineUtil.get_float(kv, "OilMinTemperature")
    if kv.has("OilMaxTemperature"):
        node.cooling_oil_max_temperature = FizLineUtil.get_float(kv, "OilMaxTemperature")
    if kv.has("WaterCoolingFanSpeed"):
        node.cooling_fan_speed = FizLineUtil.get_float(kv, "WaterCoolingFanSpeed")
    if kv.has("HeaterMinTemperature"):
        node.cooling_heater_min_temperature = FizLineUtil.get_float(kv, "HeaterMinTemperature")
    if kv.has("HeaterMaxTemperature"):
        node.cooling_heater_max_temperature = FizLineUtil.get_float(kv, "HeaterMaxTemperature")
    if kv.has("NominalCoolingPower"):
        node.cooling_nominal_power = FizLineUtil.get_float(kv, "NominalCoolingPower")


## The torque converter and the retarder behind it (Mover.cpp:11214-11241)
func _apply_torque_converter(kv: Dictionary, node: RailVehicleDieselEngine) -> void:
    if kv.has("TC_TMMax"):
        node.torque_converter_max_torque_ratio = FizLineUtil.get_float(kv, "TC_TMMax")
    if kv.has("TC_CP"):
        node.torque_converter_coupling_point = FizLineUtil.get_float(kv, "TC_CP")
    if kv.has("TC_LT"):
        node.torque_converter_lockup_torque = FizLineUtil.get_float(kv, "TC_LT")
    if kv.has("TC_LR"):
        node.torque_converter_lockup_rate = FizLineUtil.get_float(kv, "TC_LR")
    if kv.has("TC_ULR"):
        node.torque_converter_unlock_rate = FizLineUtil.get_float(kv, "TC_ULR")
    if kv.has("TC_FRI"):
        node.torque_converter_fill_rate_increase = FizLineUtil.get_float(kv, "TC_FRI")
    if kv.has("TC_FRD"):
        node.torque_converter_fill_rate_decrease = FizLineUtil.get_float(kv, "TC_FRD")
    if kv.has("TC_TII"):
        node.torque_converter_torque_in_in = FizLineUtil.get_float(kv, "TC_TII")
    if kv.has("TC_TIO"):
        node.torque_converter_torque_in_out = FizLineUtil.get_float(kv, "TC_TIO")
    if kv.has("TC_TOO"):
        node.torque_converter_torque_out_out = FizLineUtil.get_float(kv, "TC_TOO")
    if kv.has("TC_LS"):
        node.torque_converter_lockup_speed = FizLineUtil.get_float(kv, "TC_LS")
    if kv.has("TC_ULS"):
        node.torque_converter_unlock_speed = FizLineUtil.get_float(kv, "TC_ULS")
    if kv.has("MaxVelANS"):
        node.torque_converter_unlock_velocity = FizLineUtil.get_float(kv, "MaxVelANS")
    node.retarder_present = FizLineUtil.get_bool(kv, "IsRetarder")
    if not node.retarder_present:
        return
    if kv.has("R_Place"):
        node.retarder_placement = FizLineUtil.get_int(kv, "R_Place") as RailVehicleDieselEngine.RetarderPlacement
    if kv.has("R_TII"):
        node.retarder_torque_in_in = FizLineUtil.get_float(kv, "R_TII")
    if kv.has("R_MT"):
        node.retarder_max_torque = FizLineUtil.get_float(kv, "R_MT")
    if kv.has("R_MP"):
        node.retarder_max_power = FizLineUtil.get_float(kv, "R_MP")
    if kv.has("R_FRI"):
        node.retarder_fill_rate_increase = FizLineUtil.get_float(kv, "R_FRI")
    if kv.has("R_FRD"):
        node.retarder_fill_rate_decrease = FizLineUtil.get_float(kv, "R_FRD")
    if kv.has("R_MinVel"):
        node.retarder_min_velocity = FizLineUtil.get_float(kv, "R_MinVel")
    if kv.has("R_EngageVel"):
        node.retarder_engage_velocity = FizLineUtil.get_float(kv, "R_EngageVel")
    if kv.has("R_ClutchSpeed"):
        node.retarder_clutch_speed = FizLineUtil.get_float(kv, "R_ClutchSpeed")
    node.retarder_clutch = FizLineUtil.get_bool(kv, "R_IsClutch")
    node.retarder_with_individual = FizLineUtil.get_bool(kv, "R_WithIndividual")


func _get_node(context: FizImportContext) -> RailVehicleDieselEngine:
    var node: VehicleComponent = context.get_part("RailVehicleEngine")
    return node as RailVehicleDieselEngine


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    match prefix:
        "DList:":
            _active_table = "DList"
            _throttle_rows = []
            var kv: Dictionary = FizLineUtil.read_key_values(p)
            var node := _get_node(context)
            if node:
                if kv.has("Mmax"):
                    node.throttle_table_max_torque = FizLineUtil.get_float(kv, "Mmax")
                if kv.has("nMmax"):
                    node.throttle_table_max_torque_rpm = FizLineUtil.get_float(kv, "nMmax")
                if kv.has("nmax"):
                    node.mechanical_max_rpm = FizLineUtil.get_float(kv, "nmax")
                if kv.has("Mnmax"):
                    node.throttle_table_max_rpm_torque = FizLineUtil.get_float(kv, "Mnmax")
                if kv.has("nominalfill"):
                    node.throttle_table_nominal_fuel_dose = FizLineUtil.get_float(kv, "nominalfill")
                if kv.has("Mstand"):
                    node.throttle_table_resistance_torque = FizLineUtil.get_float(kv, "Mstand")
                if kv.has("NomFuelConsRate"):
                    node.throttle_table_nominal_fuel_consumption_rate = FizLineUtil.get_float(kv, "NomFuelConsRate")
        "DMList:":
            _active_table = "DMList"
            _torque_rows = []
        "HTCList:":
            _active_table = "HTCList"
            _tc_rows = []
        "V2NList:":
            _active_table = "V2NList"
            _v2n_rows = []
        _:
            _active_table = ""


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    match _active_table:
        "DList": _parse_throttle_row(p)
        "DMList": _parse_curve_row(p, _torque_rows)
        "HTCList": _parse_curve_row(p, _tc_rows)
        "V2NList": _parse_curve_row(p, _v2n_rows)


func _parse_throttle_row(p: MaszynaParser) -> void:
    var tokens: Array = p.get_tokens(3)
    if tokens.size() < 3:
        return
    var item := RailVehicleThrottlePositionItem.new()
    item.throttle_position = int(tokens[0])
    item.fuel_dose = float(tokens[1])
    item.clutch_behavior = int(tokens[2])
    _throttle_rows.append(item)


func _parse_curve_row(p: MaszynaParser, rows: Array[VehicleCurvePointItem]) -> void:
    var tokens: Array = p.get_tokens(2)
    if tokens.size() < 2:
        return
    var item := VehicleCurvePointItem.new()
    item.x = float(tokens[0])
    item.y = float(tokens[1])
    rows.append(item)


func end_table(context: FizImportContext) -> void:
    var node := _get_node(context)
    if node:
        match _active_table:
            "DList":
                if _throttle_rows:
                    node.throttle_table_positions = _throttle_rows
            "DMList":
                if _torque_rows:
                    node.torque_table = _torque_rows
            "HTCList":
                if _tc_rows:
                    node.torque_converter_table = _tc_rows
            "V2NList":
                if _v2n_rows:
                    node.vel2nmax_table = _v2n_rows
    _throttle_rows = []
    _torque_rows = []
    _tc_rows = []
    _v2n_rows = []
    _active_table = ""
