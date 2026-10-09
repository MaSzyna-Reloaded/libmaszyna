@tool
extends RefCounted
class_name FizTrainEngineParser

## Engine: section dispatcher. Decodes EngineType, creates the matching concrete RailVehicleEngine
## subclass, applies the fields common to every engine (FizTrainEngineCommon) plus the
## stashed Cntrl./Power: subsets, then delegates the remaining type-specific fields to the
## matching concrete engine parser. LoadFIZ_Engine: Mover.cpp:11119.
##
## WheelsDriven, Dumb and Steam engines are not built (the TODO branch below); every other type
## has its own field-mapping parser.

## An EZT's automatic start thresholds (LoadFIZ_Param, Mover.cpp:10303-10304)
const EZT_IMIN_LOW:int = 1
const EZT_IMIN_HIGH:int = 2

var electric_series_parser: FizTrainElectricSeriesEngineParser = FizTrainElectricSeriesEngineParser.new()
var diesel_electric_parser: FizTrainDieselElectricEngineParser = FizTrainDieselElectricEngineParser.new()
var electric_induction_parser: FizTrainElectricInductionEngineParser = FizTrainElectricInductionEngineParser.new()
var diesel_parser: FizTrainDieselEngineParser = FizTrainDieselEngineParser.new()
## The parser of the MotorParamTable: being read - its rows' layout is the engine type's (readMPT,
## Mover.cpp:9120): a series motor's own, a diesel-electric's or a plain diesel's gears
var _motor_param_parser: RefCounted = null


func parse(p: MaszynaParser, context: FizImportContext, prefix: String = "") -> void:
    if prefix == "MotorParamTable:":
        _motor_param_parser = electric_series_parser \
                if context.engine_type == RailVehicleEngine.ELECTRIC_SERIES_MOTOR else diesel_electric_parser
        _motor_param_parser.parse(p, context, prefix)
        return
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var engine_type: int = FizTrainEngineCommon.parse_engine_type(FizLineUtil.get_string(kv, "EngineType"))
    context.engine_type = engine_type

    var node: RailVehicleEngine
    match engine_type:
        RailVehicleEngine.ELECTRIC_SERIES_MOTOR:
            node = electric_series_parser.create_node()
            context.add_part("RailVehicleEngine", node)
            FizTrainEngineCommon.apply_engine_common(node, kv, context)
            FizTrainEngineCommon.apply_cntrl_engine_subset(node, context.cntrl_kv)
            electric_series_parser.apply_engine_fields(kv, node)
        RailVehicleEngine.DIESEL_ELECTRIC:
            node = diesel_electric_parser.create_node()
            context.add_part("RailVehicleEngine", node)
            FizTrainEngineCommon.apply_engine_common(node, kv, context)
            FizTrainEngineCommon.apply_cntrl_engine_subset(node, context.cntrl_kv)
            diesel_electric_parser.apply_engine_fields(kv, node)
            FizTrainDieselEngineParser.apply_diesel_common(kv, node as RailVehicleDieselEngine)
        RailVehicleEngine.ELECTRIC_INDUCTION_MOTOR:
            node = electric_induction_parser.create_node()
            context.add_part("RailVehicleEngine", node)
            FizTrainEngineCommon.apply_engine_common(node, kv, context)
            FizTrainEngineCommon.apply_cntrl_engine_subset(node, context.cntrl_kv)
            electric_induction_parser.apply_engine_fields(kv, node)
        RailVehicleEngine.DIESEL:
            node = MoverRailVehicleDieselEngine.new()
            context.add_part("RailVehicleEngine", node)
            FizTrainEngineCommon.apply_engine_common(node, kv, context)
            FizTrainEngineCommon.apply_cntrl_engine_subset(node, context.cntrl_kv)
            diesel_parser.apply_engine_fields(kv, node as RailVehicleDieselEngine)
        RailVehicleEngine.WHEELS_DRIVEN, RailVehicleEngine.DUMB, RailVehicleEngine.STEAM:
            # TODO: no wrapper engine class for these yet
            push_warning(
                    "FIZ Engine:EngineType=%s: no engine class for it yet." % FizLineUtil.get_string(kv, "EngineType"))
        _:
            push_warning("FIZ Engine:EngineType=%s: unrecognized or unsupported." % FizLineUtil.get_string(kv, "EngineType"))
    # an EZT's automatic start thresholds before Circuit: overrides them (LoadFIZ_Param, Mover.cpp:10300-10305)
    var electric_engine: RailVehicleElectricEngine = node as RailVehicleElectricEngine
    if electric_engine and context.train_type == RailVehicleController.TRAIN_TYPE_EZT:
        electric_engine.circuit_imin_low = EZT_IMIN_LOW
        electric_engine.circuit_imin_high = EZT_IMIN_HIGH


func parse_row(p: MaszynaParser, context: FizImportContext) -> void:
    _motor_param_parser.parse_row(p, context)


func end_table(context: FizImportContext) -> void:
    _motor_param_parser.end_table(context)
