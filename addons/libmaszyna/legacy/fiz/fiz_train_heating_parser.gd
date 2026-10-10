@tool
extends RefCounted
class_name FizTrainHeatingParser

## Clima: section -> RailVehicleHeating. LoadFIZ_Clima: Mover.cpp:11049 (delegates to the shared
## LoadFIZ_PowerParamsDecode, Mover.cpp:11547, with prefix "H"/"AlterH").


func parse(p: MaszynaParser, context: FizImportContext, _prefix: String = "") -> void:
    var kv: Dictionary = FizLineUtil.read_key_values(p)
    var node := MoverRailVehicleHeating.new()

    if kv.has("Heating"):
        node.heating_source = FizTrainControllerParser.parse_power_source(FizLineUtil.get_string(kv, "Heating"))
    if kv.has("HGeneratorEngine"):
        node.heating_generator_engine = FizTrainEngineCommon.parse_engine_type(FizLineUtil.get_string(kv, "HGeneratorEngine"))
    if kv.has("HGeneratorMinRPM"):
        node.heating_generator_min_rpm = FizLineUtil.get_float(kv, "HGeneratorMinRPM")
    if kv.has("HGeneratorMaxRPM"):
        node.heating_generator_max_rpm = FizLineUtil.get_float(kv, "HGeneratorMaxRPM")
    if kv.has("HGeneratorMinVoltage"):
        node.heating_generator_min_voltage = FizLineUtil.get_float(kv, "HGeneratorMinVoltage")
    if kv.has("HGeneratorMaxVoltage"):
        node.heating_generator_max_voltage = FizLineUtil.get_float(kv, "HGeneratorMaxVoltage")
    if kv.has("HMaxVoltage"):
        node.heating_max_voltage = FizLineUtil.get_float(kv, "HMaxVoltage")
    if kv.has("HPowerTrans"):
        node.heating_power_cable_type = FizTrainControllerParser.parse_power_type(FizLineUtil.get_string(kv, "HPowerTrans"))

    context.add_part("RailVehicleHeating", node)
